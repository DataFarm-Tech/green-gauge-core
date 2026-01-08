#include "EthernetConnection.hpp"
#include "esp_log.h"

static const char* TAG = "ETH";

extern "C" {
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
}


bool EthernetConnection::connect() 
{
    ESP_LOGI(TAG, "Connecting via ETH (stub)...");

    // TODO: Send AT commands via UART to initialize the modem and connect
    // This would use UART APIs and maybe PPPoS if using esp_modem

    vTaskDelay(pdMS_TO_TICKS(3000));
    ESP_LOGI(TAG, "SIM connected (simulated)");
    return false;
}

bool EthernetConnection::isConnected() 
{
    return false; // Simulated
}

void EthernetConnection::disconnect() 
{
    ESP_LOGI(TAG, "Disconnecting ETH (stub)...");
    // TODO: Send AT command to disconnect or power down modem
    vTaskDelay(pdMS_TO_TICKS(1000)); // Simulate
}
