#include "SimConnection.hpp"
#include "esp_log.h"
#include "driver/gpio.h"

#include "Types.hpp"

static const char* TAG = "SIM";

extern "C" {
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
}

bool SimConnection::sendAT(const char* cmd, const char* expect, int timeout_ms)
{
    char rx_buf[GEN_BUFFER_SIZE] = {};
    int idx = 0;
    TickType_t start = xTaskGetTickCount();

    m_modem_uart.writef("%s\r\n", cmd);

    while ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(timeout_ms)) {
        uint8_t c;
        if (m_modem_uart.readByte(c) == 1) {
            if (idx < sizeof(rx_buf) - 1) {
                rx_buf[idx++] = c;
                rx_buf[idx] = 0;
            }

            if (strstr(rx_buf, expect)) {
                ESP_LOGI(TAG, "AT OK: %s", rx_buf);
                return true;
            }

            if (strstr(rx_buf, "ERROR")) {
                ESP_LOGE(TAG, "AT ERROR: %s", rx_buf);
                return false;
            }
        }
    }

    ESP_LOGE(TAG, "AT TIMEOUT (%s)", cmd);
    return false;
}


bool SimConnection::connect()
{
    ESP_LOGI(TAG, "Initializing modem UART...");

    m_modem_uart.init(
        115200,
        GPIO_NUM_17,   // TX → Quectel RX
        GPIO_NUM_18,   // RX → Quectel TX
        GPIO_NUM_19,   // RTS (optional)
        GPIO_NUM_20    // CTS (optional)
    );

    vTaskDelay(pdMS_TO_TICKS(500));

    // Basic modem check
    if (!sendAT("AT", "OK", 1000)) return false;

    // Disable echo (important)
    sendAT("ATE0", "OK", 1000);

    // Check SIM presence
    if (!sendAT("AT+CPIN?", "READY", 2000)) {
        ESP_LOGE(TAG, "SIM not ready");
        return false;
    }

    // Check network registration (LTE)
    if (!sendAT("AT+CEREG?", "+CEREG: 0,1", 5000) &&
        !sendAT("AT+CEREG?", "+CEREG: 0,5", 5000)) {
        ESP_LOGE(TAG, "Not registered on network");
        return false;
    }

    ESP_LOGI(TAG, "SIM connected and registered");
    return true;
}


bool SimConnection::isConnected() 
{
    /**
     * Send "AT" command through UART, and if "ERROR" is not in the resulting string
     * then we are connected. return true. Otherwise return false.
     * 
     * Check network registration status with "AT+CREG?" or similar command.
     * return true if registered, false otherwise.
     */
    // TODO: Check modem status
    return false; // Simulated
}

void SimConnection::disconnect() 
{
    ESP_LOGI(TAG, "Disconnecting SIM (stub)...");
    // TODO: Send AT command to disconnect or power down modem
    vTaskDelay(pdMS_TO_TICKS(1000)); // Simulate
}
