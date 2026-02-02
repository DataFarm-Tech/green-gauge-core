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
#include "CLI.hpp"
#include "OTAUpdater.hpp"
#include "Logger.hpp"
#include "NPK.hpp"
#include "HwTypes.hpp"
#include "ActivatePkt.hpp"
#include "ReadingPkt.hpp"
#include <memory>

/**
 * @brief Global device configuration stored in EEPROM.
 */
DeviceConfig g_device_config = {
    .has_activated = false,
    .manf_info = {
        .hw_ver = {.value = ""},
        .hw_var = {.value = ""},
        .fw_ver = {.value = ""},
        .nodeId = {.value = ""},
        .secretkey = {.value = ""},
        .p_code = {.value = ""}},
    .calib = {.calib_list = {{.offset = 0.0f, .gain = 1.0f, .m_type = MeasurementType::Nitrogen}, {.offset = 0.0f, .gain = 1.0f, .m_type = MeasurementType::Phosphorus}, {.offset = 0.0f, .gain = 1.0f, .m_type = MeasurementType::Potassium}, {.offset = 0.0f, .gain = 1.0f, .m_type = MeasurementType::Moisture}, {.offset = 0.0f, .gain = 1.0f, .m_type = MeasurementType::PH}, {.offset = 0.0f, .gain = 1.0f, .m_type = MeasurementType::Temperature}}, .last_cal_ts = 0}};

uint32_t wakeup_causes = 0;
esp_reset_reason_t reset_reason;

std::unique_ptr<Communication> g_comm;
ConnectionType g_hw_var = ConnectionType::SIM;

bool send_rs485_msg() {
    // Modbus RTU request to read 7 registers (Humidity through Potassium)
    uint8_t modbus_request[8];
    modbus_request[0] = 0x01;  // Slave address
    modbus_request[1] = 0x03;  // Function code (read holding registers)
    modbus_request[2] = 0x00;  // Start address high
    modbus_request[3] = 0x00;  // Start address low
    modbus_request[4] = 0x00;  // Number of registers high
    modbus_request[5] = 0x07;  // Number of registers low (7 registers)
    
    // Calculate CRC16 inline
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < 6; i++) {
        crc ^= modbus_request[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    modbus_request[6] = static_cast<uint8_t>(crc & 0xFF);        // CRC low byte
    modbus_request[7] = static_cast<uint8_t>((crc >> 8) & 0xFF); // CRC high byte
    
    uint8_t rx_buffer[32];
    int len = 0;

    // Flush any existing data
    uart_flush(UART_NUM_2);
    
    // Send request byte by byte
    for (int i = 0; i < sizeof(modbus_request); i++) {
        rs485_uart.writeByte(modbus_request[i]);
    }
    
    g_logger.info("Sent NPK sensor read request");
    
    // Wait for response
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Read response using non-blocking readByte with timeout
    uint32_t start_time = xTaskGetTickCount();
    uint32_t timeout_ticks = pdMS_TO_TICKS(500);
    
    while (len < static_cast<int>(sizeof(rx_buffer))) {
        if (rs485_uart.readByte(rx_buffer[len])) {
            len++;
            // Reset timeout on successful read
            start_time = xTaskGetTickCount();
        } else {
            // Check timeout
            if ((xTaskGetTickCount() - start_time) > timeout_ticks) {
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(1)); // Small delay to avoid busy-waiting
        }
        
        // Stop reading if we got a complete response (19 bytes expected)
        if (len >= 5 && len == (3 + rx_buffer[2] + 2)) {
            break;
        }
    }
    
    if (len > 0) {
        g_logger.info("RS485 read %d bytes from NPK sensor", len);
        
        // Verify minimum length
        if (len < 5) {
            g_logger.warning("Response too short: %d bytes", len);
            return false;
        }
        
        // Check slave address and function code
        if (rx_buffer[0] != 0x01 || rx_buffer[1] != 0x03) {
            g_logger.warning("Invalid response header: addr=0x%02X func=0x%02X", 
                           rx_buffer[0], rx_buffer[1]);
            return false;
        }
        
        // Get byte count and verify length
        uint8_t byte_count = rx_buffer[2];
        int expected_len = 3 + byte_count + 2; // header + data + CRC
        
        if (len != expected_len) {
            g_logger.warning("Expected %d bytes, got %d", expected_len, len);
            return false;
        }
        
        // Verify CRC
        crc = 0xFFFF;
        for (int i = 0; i < len - 2; i++) {
            crc ^= rx_buffer[i];
            for (int j = 0; j < 8; j++) {
                if (crc & 0x0001) {
                    crc = (crc >> 1) ^ 0xA001;
                } else {
                    crc >>= 1;
                }
            }
        }
        uint16_t received_crc = static_cast<uint16_t>(rx_buffer[len-2] | (rx_buffer[len-1] << 8));
        
        if (received_crc != crc) {
            g_logger.warning("CRC mismatch: received=0x%04X calculated=0x%04X", 
                           received_crc, crc);
            return false;
        }
        
        // Parse data (14 bytes = 7 registers × 2 bytes each)
        uint16_t humidity = static_cast<uint16_t>((rx_buffer[3] << 8) | rx_buffer[4]);      // 0.1%RH
        uint16_t temperature = static_cast<uint16_t>((rx_buffer[5] << 8) | rx_buffer[6]);   // 0.1°C
        uint16_t conductivity = static_cast<uint16_t>((rx_buffer[7] << 8) | rx_buffer[8]);  // us/cm
        uint16_t ph = static_cast<uint16_t>((rx_buffer[9] << 8) | rx_buffer[10]);           // 0.1 pH
        uint16_t nitrogen = static_cast<uint16_t>((rx_buffer[11] << 8) | rx_buffer[12]);    // mg/kg
        uint16_t phosphorus = static_cast<uint16_t>((rx_buffer[13] << 8) | rx_buffer[14]);  // mg/kg
        uint16_t potassium = static_cast<uint16_t>((rx_buffer[15] << 8) | rx_buffer[16]);   // mg/kg
        
        g_logger.info("Soil Data:");
        g_logger.info("  Humidity: %.1f %%", humidity / 10.0);
        g_logger.info("  Temperature: %.1f °C", temperature / 10.0);
        g_logger.info("  Conductivity: %u us/cm", conductivity);
        g_logger.info("  pH: %.1f", ph / 10.0);
        g_logger.info("  Nitrogen (N): %u mg/kg", nitrogen);
        g_logger.info("  Phosphorus (P): %u mg/kg", phosphorus);
        g_logger.info("  Potassium (K): %u mg/kg", potassium);
        
        return true;
    }
    else {
        g_logger.warning("No data received from RS485 NPK sensor");
        return false;
    }
}

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
        GPIO_NUM_37,        // TXD0 (IO37) - connects to UART2_TXD
        GPIO_NUM_36,        // RXD0 (IO36) - connects to UART2_RXD
        UART_PIN_NO_CHANGE, /* rts_pin */
        UART_PIN_NO_CHANGE, /* cts_pin */
        GEN_BUFFER_SIZE,    /* rx_buffer_size */
        GEN_BUFFER_SIZE     // /* tx_buffer_size */ RS485 benefits from TX buffer
    );

    send_rs485_msg();

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

/**
 * @brief Handle device activation process.
 */
void handle_activation()
{

    ActivatePkt activatePkt( PktType::Activate, std::string(g_device_config.manf_info.nodeId.value), 
    std::string(ACT_URI), std::string(g_device_config.manf_info.secretkey.value));

    const uint8_t *pkt_1 = activatePkt.toBuffer();
    const size_t buffer_len = activatePkt.getBufferLength();

    if (g_device_config.has_activated)
    {
        g_logger.info("Device already activated (node: %s)", g_device_config.manf_info.nodeId.value);
        return;
    }

    g_logger.info("Sending activation packet for node: %s", g_device_config.manf_info.nodeId.value);

    if (!g_comm->sendPacket(pkt_1, buffer_len, PktType::Activate))
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
    uint8_t reading[NPK_COLLECT_SIZE];

    for (const NPK::MeasurementEntry &m_entry : NPK::MEASUREMENT_TABLE)
    {
        memset(reading, 0, NPK_COLLECT_SIZE);

        NPK::npk_collect(m_entry, reading);


        ReadingPkt readingPkt(PktType::Reading, std::string(g_device_config.manf_info.nodeId.value), std::string(DATA_URI), reading,  m_entry.type);

        const uint8_t * cbor_buffer = readingPkt.toBuffer(); //the CBOR payload
        const size_t cbor_buffer_len = readingPkt.getBufferLength(); // the CBOR payload len
        
        //TAkes the cbor data.
        if (!g_comm->sendPacket(cbor_buffer, cbor_buffer_len, PktType::Reading))
        {
            g_logger.error("Sending activation packet failed for node: %s", g_device_config.manf_info.nodeId.value);
            return;
        }
    }
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

    if (!g_comm->connect())
    {
        g_logger.error("Unable to connect to network");
        vTaskDelete(nullptr);
        return;
    }

    g_logger.info("Device connected to network");

    if (g_comm->isConnected() && !g_device_config.has_activated)
    {
        g_logger.info("Activating UNIT\n");
        ESP_LOGI("UNIT", "ACTIVATING UNIT\n");
        handle_activation();
    }

#if OTA_EN == 1
    if (g_comm->isConnected())
    {

        bool should_run_ota = !(wakeup_causes & ESP_SLEEP_WAKEUP_TIMER) && reset_reason == ESP_RST_POWERON;

        if (should_run_ota)
        {
            g_logger.info("Power-on or hard reset");
            checkAndPerformOTA();
        }
        else
        {
            g_logger.info("Good morning!!");
        }
    }
#endif

    /** TODO
     * 1. Read ALL DATA FROM NPK sensor. 50 N, 50 P.... 50 PH, returns a uint8_t buffer
     * 2. Call Communications send method -> See send method.
     */
    collect_reading();

    g_logger.info("Entering deep sleep for %d seconds", sleep_time_sec);
    esp_sleep_enable_timer_wakeup(sleep_time_sec * 1000000ULL);
    vTaskDelay(pdMS_TO_TICKS(100));
    esp_deep_sleep_start();

    vTaskDelete(nullptr);
}

/**
 * @brief Main application entry point.
 */
extern "C" void app_main(void)
{

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