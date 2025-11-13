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
    // // --- Battery Test Code ---
    // ESP_LOGI("MAIN", "Starting battery percentage test...");
    // BatteryPacket battery(NODE_ID, BATT_URI, 0, 0, BATT_TAG);

    // while (1) {
    //     if (!battery.readFromBMS()) {
    //         ESP_LOGE("MAIN", "Failed to read battery from BMS");
    //     }
    //     // The battery level is logged inside readFromBMS()
    //     vTaskDelay(pdMS_TO_TICKS(5000)); // Wait 5 seconds
    // }


    // --- Original Application Code ---
    EEPROMConfig eeprom;

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
        ESP_ERROR_CHECK(esp_netif_init());
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        
        Communication comm(ConnectionType::WIFI);

        if (comm.connect())
        {
            if (!g_device_config.has_activated) 
            {
                ESP_LOGI("MAIN", "Sending activation packet...");
                ActivatePacket activate(NODE_ID, ACT_URI, "activate");
                activate.sendPacket();

                g_device_config.has_activated = true; // Mark as activated and save it
                eeprom.saveConfig(g_device_config);
            } 
            else 
            {
                ESP_LOGI("MAIN", "Already activated — skipping activation packet.");
            }
        
            ReadingPacket readings(NODE_ID, DATA_URI, DATA_TAG); // Send readings
            ESP_LOGI("MAIN", "Collecting sensor readings...");
            
            readings.readSensor();
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

            // Close connection
            if (comm.isConnected()) 
            {
                comm.disconnect();
            }
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