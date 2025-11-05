#include <iostream>
#include "comm/Communication.hpp"
#include "esp_log.h"
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
#include "packet/BatteryPacket.hpp"
#include "packet/ReadingPacket.hpp"
#include "Config.hpp"
#include "ota/OTAUpdater.hpp"

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
    while (1)
    {
        init_nvs();
        ESP_ERROR_CHECK(esp_netif_init());
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        
        Communication comm(ConnectionType::WIFI);

        if (comm.connect())
        {
            ReadingPacket readings(NODE_ID, DATA_URI, "ReadingPacket");

            ESP_LOGI("MAIN", "Collecting sensor readings...");
            readings.readSensor();

            ESP_LOGI("MAIN", "Sending readings packet...");
            readings.sendPacket();
            
            
            BatteryPacket battery(NODE_ID, BATT_URI, 0, 0, BATT_TAG);
            
            if (!battery.readFromBMS())
            {
                ESP_LOGE("MAIN", "Failed to read battery from BMS");
            } 
            else 
            {
                ESP_LOGI("MAIN", "Sending battery packet...");
                battery.sendPacket();
            }

            //Close connection
            if (comm.isConnected()) 
            {
                comm.disconnect();
            }
        }
        
        #if DEEP_SLEEP_EN == 1
            break;
        #else
            vTaskDelay(pdMS_TO_TICKS(sleep_time_sec * 1000));
        #endif
    }
    
    #if DEEP_SLEEP_EN == 1
        ESP_LOGI("MAIN", "Going into deep sleep for %d seconds...", sleep_time_sec);
        esp_sleep_enable_timer_wakeup(sleep_time_sec * 1000000ULL);  // Convert to microseconds
        esp_deep_sleep_start();
    #endif
}