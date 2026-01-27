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
#include "UARTDriver.hpp"
#include "CLI.hpp"
#include "OTAUpdater.hpp"
#include "Logger.hpp"
#include "NPK.hpp"

/**
 * @brief Global device configuration stored in EEPROM.
 */
DeviceConfig g_device_config = {
    .has_activated = false,
    .manf_info = {
        .hw_ver = {.value="", .has_provision=false},
        .hw_var = {.value="", .has_provision=false},
        .fw_ver = {.value="", .has_provision=false},
        .nodeId = {.value="", .has_provision=false},
        .secretkey = {.value="", .has_provision=false},
        .p_code = {.value="", .has_provision=false}
    },
    .calib = {
        .calib_list = {
            { .offset = 0.0f, .gain = 1.0f, .m_type = MeasurementType::Nitrogen },
            { .offset = 0.0f, .gain = 1.0f, .m_type = MeasurementType::Phosphorus },
            { .offset = 0.0f, .gain = 1.0f, .m_type = MeasurementType::Potassium },
            { .offset = 0.0f, .gain = 1.0f, .m_type = MeasurementType::Moisture },
            { .offset = 0.0f, .gain = 1.0f, .m_type = MeasurementType::PH },
            { .offset = 0.0f, .gain = 1.0f, .m_type = MeasurementType::Temperature }
        },
        .last_cal_ts = 0
    }
};

/** UART console instance used for CLI interaction */
static UARTDriver serialConsole(UART_NUM_0);

/**
 * @brief Initialize core system components.
 */
void init_hw() {
    g_logger.init();
    g_logger.info("System booting...");

    if (!eeprom.begin()) {
        g_logger.error("Failed to open EEPROM config");
        esp_restart();
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
}

/**
 * @brief Load existing config or create default configuration.
 */
bool load_create_config() {
    const esp_app_desc_t* app_desc = esp_app_get_description();

    if (eeprom.loadConfig(g_device_config)) {
        g_logger.info("Loaded config from EEPROM. Node ID: %s, Activated: %s",
                      g_device_config.manf_info.nodeId.value,
                      g_device_config.has_activated ? "Yes" : "No");
        return true;
    }

    g_logger.info("No previous config found, initializing defaults");

    g_device_config.has_activated = false;

    strncpy(g_device_config.manf_info.fw_ver.value, app_desc->version,
            sizeof(g_device_config.manf_info.fw_ver.value) - 1);
    g_device_config.manf_info.fw_ver.has_provision = true;

    if (!eeprom.saveConfig(g_device_config)) {
        g_logger.error("Failed to save initial config");
        return false;
    }

    g_logger.info("Initial config saved.");
    return true;
}

/**
 * @brief Handle device activation process.
 */
void handle_activation(Communication& comm) {
    if (g_device_config.has_activated) {
        g_logger.info("Device already activated (node: %s)", g_device_config.manf_info.nodeId.value);
        return;
    }

    g_logger.info("Sending activation packet for node: %s", g_device_config.manf_info.nodeId.value);

    ActivatePacket activate(std::string(g_device_config.manf_info.nodeId.value), ACT_URI, ACT_TAG);
    activate.sendPacket();

    vTaskDelay(pdMS_TO_TICKS(3000));

    g_device_config.has_activated = true;
    if (eeprom.saveConfig(g_device_config)) {
        g_logger.info("Activation complete, config saved to EEPROM");
    } else {
        g_logger.error("Failed to save activation status to EEPROM");
    }
}

#if OTA_EN == 1
/**
 * @brief Check reset reason and perform OTA update if appropriate.
 */
void checkAndPerformOTA() {
    esp_reset_reason_t reset_reason = esp_reset_reason();
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

    if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) {
        g_logger.info("Woke from deep sleep timer");
        return;
    }

    g_logger.info("Power-on or hard reset");

    if (reset_reason == ESP_RST_POWERON) {
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
}
#endif

/**
 * @brief Enter deep sleep or continuous loop based on configuration.
 */
void enterSleepOrLoop(Communication& comm) {
#if CLI_EN == 1
    g_logger.info("CLI mode: keeping WiFi connection active");
#else
    if (comm.isConnected()) {
        g_logger.info("Disconnecting from WiFi");
        comm.disconnect();
    }
#endif

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
    NPK npk_obj;
    ReadingPacket readings(std::string(g_device_config.manf_info.nodeId.value),
                           DATA_URI, POTASSIUM, DATA_TAG);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(sleep_time_sec * 1000));

        if (comm.connect()) {
            npk_obj.npk_collect(readings);
            if (comm.isConnected()) comm.disconnect();
        }
    }
    #endif
#endif
}

/**
 * @brief Background task for waiting provisioning, connecting, activating, OTA, and data collection.
 */
void start_app(void * arg) {

    NPK npk_obj;
    ReadingPacket readings(std::string(g_device_config.manf_info.nodeId.value), DATA_URI, POTASSIUM, DATA_TAG);
    TickType_t lastLogTick = 0;
    constexpr TickType_t LOG_INTERVAL = pdMS_TO_TICKS(4000);
    TickType_t now;
    uint8_t hw_var_int;
    ConnectionType hw_var;


    auto is_provisioned = []() {
        const auto& m = g_device_config.manf_info;
        return  m.fw_ver.has_provision && 
                m.hw_ver.has_provision && 
                m.nodeId.has_provision && 
                m.secretkey.has_provision &&
                m.p_code.has_provision &&
                m.hw_var.has_provision;
    };

    while (!is_provisioned()) {
        now = xTaskGetTickCount();
        if (now - lastLogTick > LOG_INTERVAL) {
            g_logger.warning("Waiting for provisioning...");
            printf("Waiting for provision...\n");
            lastLogTick = now;
        }
        vTaskDelay(pdMS_TO_TICKS(500)); // small delay, keeps CLI responsive
    }

    g_logger.info("Provisioning complete!");
        
    hw_var = ConnectionType::WIFI;
    Communication* comm = new Communication(hw_var);

    if (!comm->connect()) {
        g_logger.error("Unable to connect to network");
        vTaskDelete(nullptr);
        return;
    }
    g_logger.info("Device connected to WLAN0");

    // Handle activation
    handle_activation(*comm);

#if OTA_EN == 1
    checkAndPerformOTA();
#endif

    /**
     * HW Dependent Code:
     * TODO: Check HW Ver and Hw Type,
     * depending on the revision hardware changes maybe.
     */

    // Collect and send data
    npk_obj.npk_collect(readings);

    // Enter sleep or loop
    enterSleepOrLoop(*comm);

    vTaskDelete(nullptr);
}

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

/**
 * @brief Main application entry point.
 */
extern "C" void app_main(void) {
    
    
    // Step 1: Initialize core system
    init_hw();
    
    // Step 2: Load or create configuration
    if (!load_create_config()) return;

    // Step 3: Initialize CLI
    serialConsole.init(BAUD_115200);
    CLI::start(serialConsole);

    // Step 4: Launch provisioning + operational tasks in background
    xTaskCreate(
        start_app,
        "start_app",
        8192,      // stack size
        nullptr,      // argument
        5,         // priority
        nullptr
    );

    // Keep main alive for CLI responsiveness
    while (1) {
        printf("HELLOWORLD\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}