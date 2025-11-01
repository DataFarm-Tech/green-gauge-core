#include <iostream>
#include "Communication.hpp"
#include "esp_log.h"
#include "Config.hpp"

extern "C" {
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "nvs_flash.h"
    #include "esp_sleep.h"
}

#include "esp_log.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include <string.h>
#include <vector>
#include "esp_littlefs.h"
#include <cstdint>
#include "BatteryPacket.hpp"

constexpr int sleep_time_sec = 6 * 60 * 60;

void init_nvs()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    
    ESP_ERROR_CHECK(ret);
}

extern "C" void app_main(void) 
{
    init_nvs();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    Communication comm(ConnectionType::WIFI);
    
    if (!comm.connect()) 
    {
        ESP_LOGE("MAIN", "Failed to connect.");
    }
    else
    {
        ESP_LOGI("MAIN", "Connected.");

        BatteryPacket battery("nod123");

        if (!battery.readFromBMS()) {
            ESP_LOGE("MAIN", "Failed to read battery from BMS");
        } else {
            ESP_LOGI("MAIN", "Battery Level: %d, Health: %d", battery.getLevel(), battery.getHealth());

            battery.sendPacket();
            ESP_LOGI("MAIN", "BatteryPacket sent successfully.");
        }

        if (comm.isConnected()) 
        {
            ESP_LOGI("MAIN", "Still connected. Disconnecting...");
            comm.disconnect();
        }
    }

    ESP_LOGI("MAIN", "Going into deep sleep for %d seconds...", sleep_time_sec);
    esp_sleep_enable_timer_wakeup(sleep_time_sec * 1000000ULL);  // Convert to microseconds
    esp_deep_sleep_start();
}
