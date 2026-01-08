#include "SimConnection.hpp"
#include "esp_log.h"

static const char* TAG = "SIM";

extern "C" {
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
}

enum class SimStatus : uint8_t {
    DISCONNECTED,      ///< Wi-Fi connection
    UNREGISTERED
};

bool SimConnection::connect() 
{
    ESP_LOGI(TAG, "Connecting via SIM (stub)...");

    // TODO: Send AT commands via UART to initialize the modem and connect
    // This would use UART APIs and maybe PPPoS if using esp_modem

    vTaskDelay(pdMS_TO_TICKS(3000)); // Simulate connection time
    ESP_LOGI(TAG, "SIM connected (simulated)");
    return false;
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
