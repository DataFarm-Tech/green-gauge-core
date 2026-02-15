#include "Communication.hpp"
#include <stdio.h>

extern "C"
{
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "driver/uart.h"
}

#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include <string.h>
#include "esp_log.h"
#include "Config.hpp"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "EEPROMConfig.hpp"
#include "UARTDriver.hpp"
// #include "CLI.hpp"
#include "cli.hpp"
#include "OTAUpdater.hpp"
#include "Logger.hpp"
#include "NPK.hpp"
#include "HwTypes.hpp"
#include "ActivatePkt.hpp"
#include "ReadingPkt.hpp"
#include <memory>
#include "cli.hpp"
#include "GpsUpdatePkt.hpp"

/**
 * @brief Global device configuration stored in EEPROM.
 */
DeviceConfig g_device_config = {
    .has_activated = false,
    .main_app_delay = 30,
    .session_count = 0,
    .manf_info = {
        .hw_ver = {.value = ""},
        .hw_var = {.value = ""},
        .fw_ver = {.value = ""},
        .nodeId = {.value = ""},
        .secretkey = {.value = ""},
        .p_code = {.value = ""},
        .sim_sn = {.value = ""}},
    .calib = {.calib_list = {{.offset = 0.0f, .gain = 1.0f, .m_type = MeasurementType::Nitrogen}, {.offset = 0.0f, .gain = 1.0f, .m_type = MeasurementType::Phosphorus}, {.offset = 0.0f, .gain = 1.0f, .m_type = MeasurementType::Potassium}, {.offset = 0.0f, .gain = 1.0f, .m_type = MeasurementType::Moisture}, {.offset = 0.0f, .gain = 1.0f, .m_type = MeasurementType::PH}, {.offset = 0.0f, .gain = 1.0f, .m_type = MeasurementType::Temperature}}, .last_cal_ts = 0}};

uint32_t wakeup_causes = 0;
esp_reset_reason_t reset_reason;

std::unique_ptr<Communication> g_comm;
ConnectionType g_hw_var = ConnectionType::SIM;
GPS m_gps;

/**
 * @brief Initialize core system components.
 */
void init_hw()
{
    g_logger.init();
    g_logger.info("System booting...");

    if (!eeprom.begin())
    {
        g_logger.error("Failed to open EEPROM config");
        esp_restart();
    }

    rs485_uart.init(
        BAUD_4800,          // Baud rate (adjust based on your RS485 device requirements)
        GPIO_NUM_38,        // TXD0 (IO37) - connects to UART2_TXD
        GPIO_NUM_37,        // RXD0 (IO36) - connects to UART2_RXD
        UART_PIN_NO_CHANGE, /* rts_pin */
        UART_PIN_NO_CHANGE, /* cts_pin */
        GEN_BUFFER_SIZE,    /* rx_buffer_size */
        GEN_BUFFER_SIZE     // /* tx_buffer_size */ RS485 benefits from TX buffer
    );

    switch (g_hw_var)
    {
    case ConnectionType::WIFI:
        ESP_ERROR_CHECK(esp_netif_init());
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        break;
    case ConnectionType::SIM:
        m_modem_uart.init(
            BAUD_115200,        // baud rate
            GPIO_NUM_17,        // TX → Quectel RX
            GPIO_NUM_18,        // RX → Quectel TX
            UART_PIN_NO_CHANGE, // RTS (not used)
            UART_PIN_NO_CHANGE, // CTS (not used)
            2048,               // RX buffer size (larger for AT responses)
            0                   // TX buffer size (0 = no buffering)
        );
        break;

    default:
        break;
    }
}

/**
 * @brief Load existing config or create default configuration.
 */
bool load_create_config()
{
    if (eeprom.loadConfig(g_device_config))
    {
        g_logger.info("Loaded config from NVS. Node ID: %s, Activated: %s",
                      g_device_config.manf_info.nodeId.value,
                      g_device_config.has_activated ? "Yes" : "No");

        return true;
    }

    g_logger.error("Failed to load config from NVS - device may not be provisioned");
    return false;
}

/**
 * @brief The following function selects the network interface.
 */
void net_select(HwVer_e hw_ver)
{
    switch (hw_ver)
    {
    case HW_VER_0_0_1_E:
        g_hw_var = ConnectionType::SIM;
        break;
    case HW_VER_0_0_2_E:
        g_hw_var = ConnectionType::SIM;
        break;

    default:
        g_hw_var = ConnectionType::WIFI;
        break;
    }

    g_comm = std::make_unique<Communication>(g_hw_var);
}

/**
 * @brief The following function finds the current HW_VER.
 * Then initialises network_interface select. Depending on HW.
 */
void hw_features(void)
{
    HwVer_e hw_ver = HW_VER_UNKNOWN_E;

    for (const auto &entry : ver)
    {
        if (entry.name == nullptr)
        {
            break;
        }

        if (strcmp(entry.name, g_device_config.manf_info.hw_ver.value) == 0)
        {
            hw_ver = entry.hw_ver;
            break;
        }
    }

    hw_ver = HW_VER_0_0_1_E;

    net_select(hw_ver);
}

std::string read_gps()
{
    std::string parsed;

    // GPS Cold Start (first fix) can take 30-60 seconds. Allow adequate time for acquisition.
    // Initial 15s delay gives modem time to begin satellite search after AT+QGPS=1 command
    g_logger.info("Waiting for GPS fix (Cold Start may take 30-60 seconds)...");
    vTaskDelay(pdMS_TO_TICKS(15000));

    if (m_gps.getCoordinates(parsed))
    {
        g_logger.info("GPS location retrieved: %s", parsed.c_str());
        return parsed;
    }
    else
    {
        std::string default_coords = "0.0,0.0"; // Define your default
        g_logger.info("GPS location not available, using default coordinates: %s", default_coords.c_str());
        return default_coords;
    }
}

void handle_gps_update() {
    std::string gps = read_gps();

    GpsUpdatePkt gpsupdatePkt(PktType::GpsUpdate, std::string(g_device_config.manf_info.nodeId.value), std::string(GPS_URI), gps);

    const uint8_t * pkt_1 = gpsupdatePkt.toBuffer();
    const size_t buffer_len = gpsupdatePkt.getBufferLength();

    if (!g_comm->sendPacket(pkt_1, buffer_len, PktType::GpsUpdate, CoapMethod::PUT))
    {
        g_logger.error("Sending activation packet failed for node: %s", g_device_config.manf_info.nodeId.value);
        return;
    }
}

/**
 * @brief Handle device activation process.
 */
void handle_activation()
{
    std::string gps = read_gps();

    ActivatePkt activatePkt(PktType::Activate, std::string(g_device_config.manf_info.nodeId.value),
                            std::string(ACT_URI), std::string(g_device_config.manf_info.secretkey.value), gps);

    const uint8_t *pkt_1 = activatePkt.toBuffer();
    const size_t buffer_len = activatePkt.getBufferLength();

    if (g_device_config.has_activated)
    {
        g_logger.info("Device already activated (node: %s)", g_device_config.manf_info.nodeId.value);
        return;
    }

    g_logger.info("Sending activation packet for node: %s", g_device_config.manf_info.nodeId.value);

    if (!g_comm->sendPacket(pkt_1, buffer_len, PktType::Activate, CoapMethod::POST))
    {
        g_logger.error("Sending activation packet failed for node: %s", g_device_config.manf_info.nodeId.value);
        return;
    }

    g_device_config.has_activated = true;

    if (eeprom.saveConfig(g_device_config))
    {
        g_logger.info("Activation complete, config saved to EEPROM");
    }
    else
    {
        g_logger.error("Failed to save activation status to EEPROM");
    }

    return;
}

#if OTA_EN == 1
/**
 * @brief Check reset reason and perform OTA update if appropriate.
 */
void checkAndPerformOTA()
{
    OTAUpdater ota;
    const char *url = "http://45.79.118.187:8080/release/latest/cn1.bin";

    g_logger.info("Power-on detected, checking for OTA update");

    if (ota.update(url))
    {
        g_logger.info("OTA successful, rebooting...");
        vTaskDelay(pdMS_TO_TICKS(2000));
        esp_restart();
    }
    else
    {
        g_logger.warning("OTA failed or no update needed");
    }
}
#endif

void collect_reading()
{
    uint16_t reading[NPK_COLLECT_SIZE];

    for (const NPK::MeasurementEntry &m_entry : NPK::MEASUREMENT_TABLE)
    {
        memset(reading, 0, NPK_COLLECT_SIZE);
        
        if (!NPK::npk_collect(m_entry, reading))
        {
            g_logger.warning("Failed to collect measurement type %d", 
                           static_cast<int>(m_entry.type));
            continue;  // Skip to next measurement
        }

        g_device_config.session_count++;
        
        ReadingPkt readingPkt(PktType::Reading, 
                             std::string(g_device_config.manf_info.nodeId.value), 
                             std::string(DATA_URI), 
                             reading, 
                             m_entry.type,
                            g_device_config.session_count);

        const uint8_t *cbor_buffer = readingPkt.toBuffer();
        const size_t cbor_buffer_len = readingPkt.getBufferLength();

        if (g_comm->sendPacket(cbor_buffer, cbor_buffer_len, PktType::Reading, CoapMethod::POST))
        {
            g_logger.info("Sent measurement type %d successfully", 
                         static_cast<int>(m_entry.type));
        }
        else
        {
            g_logger.warning("Failed to send measurement type %d (will continue)", 
                           static_cast<int>(m_entry.type));
        }
    }
    
    eeprom.saveConfig(g_device_config);

    g_logger.info("Collection complete");
}

/**
 * @brief Background task for waiting provisioning, connecting, activating, OTA, and data collection.
 */
void start_app(void *arg)
{
    if (!g_comm)
    {
        g_logger.error("Communication not initialized");
        vTaskDelete(nullptr);
        return;
    }

    // Initial connection
    if (!g_comm->connect())
    {
        g_logger.error("Unable to connect to network");
        vTaskDelete(nullptr);
        return;
    }

    g_logger.info("Device connected to network");

    // Handle activation only once
    if (g_comm->isConnected()) {
        if (!g_device_config.has_activated) {
            handle_activation();
        }
        else {
            handle_gps_update();    
        }
    }

#if OTA_EN == 1
    // OTA check only on power-on
    if (g_comm->isConnected())
    {
        bool should_run_ota = !(wakeup_causes & ESP_SLEEP_WAKEUP_TIMER) && reset_reason == ESP_RST_POWERON;

        if (should_run_ota)
        {
            g_logger.info("Power-on or hard reset");
            checkAndPerformOTA();
        }
    }
#endif

    collect_reading();

    g_logger.info("Entering deep sleep for %d seconds", g_device_config.main_app_delay);
    esp_sleep_enable_timer_wakeup(g_device_config.main_app_delay * 1000000ULL);
    vTaskDelay(pdMS_TO_TICKS(100));
    esp_deep_sleep_start();

    vTaskDelete(nullptr);
}

/**
 * @brief Main application entry point.
 */
extern "C" void app_main(void)
{
    // cli_init();

    reset_reason = esp_reset_reason();
    wakeup_causes = esp_sleep_get_wakeup_causes();

    hw_features();

    init_hw();

    // Step 2: Load or create configuration
    if (!load_create_config())
        return;

    // Step 3: Launch provisioning + operational tasks in background
    xTaskCreate(start_app, "start_app", 8192, nullptr, 5, nullptr);

    return;
}