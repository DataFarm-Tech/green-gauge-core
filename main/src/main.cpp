#include <iostream>
#include "comm/Communication.hpp"
#include "esp_log.h"

extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_sleep.h"
}

#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include <string.h>
#include "packet/BatteryPacket.hpp"
#include "packet/ReadingPacket.hpp"
#include "packet/ActivatePacket.hpp"
#include "Config.hpp"
#include "ota/OTAUpdater.hpp"
#include "EEPROMConfig.hpp"

DeviceConfig g_device_config = { false };

extern "C" void app_main(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    Communication comm(ConnectionType::WIFI);
    EEPROMConfig eeprom;
    ReadingPacket readings(NODE_ID, DATA_URI, DATA_TAG); // Send readings
    ActivatePacket activate(NODE_ID, ACT_URI, "activate");
    BatteryPacket battery(NODE_ID, BATT_URI, 0, 0, BATT_TAG);

    if (!eeprom.begin()) {
        ESP_LOGE("MAIN", "Failed to open EEPROM config");
        return;
    }

    if (!eeprom.loadConfig(g_device_config)) {
        ESP_LOGW("MAIN", "No previous config found, initializing default...");
        g_device_config.has_activated = false;
        eeprom.saveConfig(g_device_config);
    }
    
    while (1)
    {
        if (comm.connect())
        {
            if (!g_device_config.has_activated) 
            {
                ESP_LOGI("MAIN", "Sending activation packet...");
                activate.sendPacket();

                g_device_config.has_activated = true; // Mark as activated and save it
                eeprom.saveConfig(g_device_config);
            } 
            else 
            {
                ESP_LOGI("MAIN", "Already activated — skipping activation packet.");
            }
        
            ESP_LOGI("MAIN", "Collecting sensor readings...");
            
            readings.readSensor();
            readings.sendPacket();

            
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
        else {
            ESP_LOGI("MAIN", "Unable to connect");
            //Start the web server.
        }

        eeprom.close();

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