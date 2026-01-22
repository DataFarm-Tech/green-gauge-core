#include <iostream>
#include "Communication.hpp"
#include "esp_log.h"

extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_sleep.h"
#include "driver/uart.h"
}

#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include <string.h>
#include "BatteryPacket.hpp"
#include "ReadingPacket.hpp"
#include "ActivatePacket.hpp"
#include "Config.hpp"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "EEPROMConfig.hpp"
#include "Node.hpp"
#include "UARTDriver.hpp"
#include "CLI.hpp"
#include "OTAUpdater.hpp"
#include "Logger.hpp"
#include "NPK.hpp"

/**
 * @brief Global device configuration stored in EEPROM.
 *
 * This structure holds persistent device information such as:
 * - Activation status
 * - Node ID
 * - Hardware version
 * - Firmware version
 */
DeviceConfig g_device_config = {
    .has_activated = false,
    .manf_info = {
        .hw_ver = "",
        .fw_ver = "",
        .nodeId = ""
    },
    .calib = {
        .calib_list = {
            { .offset = 0.0f, .gain = 1.0f, .m_type = MeasurementType::Nitrogen },
            { .offset = 0.0f, .gain = 1.0f, .m_type = MeasurementType::Phosphorus },
            { .offset = 0.0f, .gain = 1.0f, .m_type = MeasurementType::Potassium },
            { .offset = 0.0f, .gain = 1.0f, .m_type = MeasurementType::Moisture },
            { .offset = 0.0f, .gain = 1.0f, .m_type = MeasurementType::PH },
            { .offset = 0.0f, .gain = 1.0f, .m_type = MeasurementType::Temperature}
        },
        .last_cal_ts = 0
    }
};


#if CLI_EN == 1
/** UART console instance used for CLI interaction */
static UARTDriver serialConsole(UART_NUM_0);
#endif

extern "C" void app_main(void)
{
    NPK npk_obj;
    g_logger.init();
    g_logger.info("System booting...");

#if OTA_EN == 1
    esp_reset_reason_t reset_reason = esp_reset_reason();
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

    if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) {
        g_logger.info("Woke from deep sleep timer");
    } else {
        g_logger.info("Power-on or hard reset");
    }
#endif

    // Initialize EEPROM/NVS
    if (!eeprom.begin()) {
        g_logger.error("Failed to open EEPROM config");
        return;
    }

    // Load or initialize configuration
    if (!eeprom.loadConfig(g_device_config)) {
        g_logger.info("No previous config found, initializing defaults");

        Node nodeId;
        strncpy(g_device_config.manf_info.nodeId, nodeId.getNodeID(), sizeof(g_device_config.manf_info.nodeId) - 1);
        g_device_config.manf_info.nodeId[sizeof(g_device_config.manf_info.nodeId) - 1] = '\0';

        g_device_config.has_activated = false;

        if (!eeprom.saveConfig(g_device_config)) {
            g_logger.error("Failed to save initial config");
        } else {
            g_logger.info("Initial config saved with Node ID: %s", g_device_config.manf_info.nodeId);
        }
    } else {
        g_logger.info("Loaded config from EEPROM. Node ID: %s, Activated: %s", 
                     g_device_config.manf_info.nodeId, 
                     g_device_config.has_activated ? "Yes" : "No");
    }

    // Initialize network stack BEFORE any network operations
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Communication & packets
    Communication comm(ConnectionType::WIFI);
    ActivatePacket activate(std::string(g_device_config.manf_info.nodeId), ACT_URI, ACT_TAG);
    ReadingPacket readings(std::string(g_device_config.manf_info.nodeId), DATA_URI, POTASSIUM, DATA_TAG);

#if CLI_EN == 1
    serialConsole.init(115200);
    CLI::start(serialConsole);
#endif

    if (comm.connect())
    {
        g_logger.info("Device connected to WLAN0");

        if (!g_device_config.has_activated)
        {
            g_logger.info("Sending activation packet for node: %s", g_device_config.manf_info.nodeId);
            activate.sendPacket();

            vTaskDelay(pdMS_TO_TICKS(3000));

            g_device_config.has_activated = true;
            if (eeprom.saveConfig(g_device_config)) {
                g_logger.info("Activation complete, config saved to EEPROM");
            } else {
                g_logger.error("Failed to save activation status to EEPROM");
            }
        }
        else
        {
            g_logger.info("Device already activated (node: %s)", g_device_config.manf_info.nodeId);
        }

        npk_obj.npk_collect(readings);

#if OTA_EN == 1
        if (reset_reason == ESP_RST_POWERON && wakeup_reason != ESP_SLEEP_WAKEUP_TIMER) {
            OTAUpdater ota;
            const char* url = "http://45.79.118.187:8080/release/latest/cn1.bin";

            g_logger.info("Power-on detected, checking for OTA update");

            if (ota.update(url)) {
                g_logger.info("OTA successful, rebooting...");
                vTaskDelay(pdMS_TO_TICKS(2000));
                esp_restart();
            } else {
                g_logger.warning("OTA failed or no update needed");
            }
        }
#endif

#if CLI_EN == 1
        // In CLI mode, keep WiFi connected for debugging
        g_logger.info("CLI mode: keeping WiFi connection active");
#else
        // In production mode, disconnect to save power
        if (comm.isConnected()) {
            g_logger.info("Disconnecting from WiFi");
            comm.disconnect();
        }
#endif
    }
    else
    {
        g_logger.error("Unable to connect to network");
    }

#if DEEP_SLEEP_EN == 1
    g_logger.info("Entering deep sleep for %d seconds", sleep_time_sec);
    esp_sleep_enable_timer_wakeup(sleep_time_sec * 1000000ULL);
    vTaskDelay(pdMS_TO_TICKS(100));
    esp_deep_sleep_start();
#else
#if CLI_EN == 1
    g_logger.info("CLI mode active, device will not sleep");
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
#else
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(sleep_time_sec * 1000));

        if (comm.connect())
        {
            npk_obj.npk_collect(readings);
            
            if (comm.isConnected())
                comm.disconnect();
        }
    }
#endif
#endif
}