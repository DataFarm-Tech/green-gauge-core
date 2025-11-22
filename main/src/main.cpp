#include <iostream>
#include "comm/Communication.hpp"
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
#include "packet/BatteryPacket.hpp"
#include "packet/ReadingPacket.hpp"
#include "packet/ActivatePacket.hpp"
#include "Config.hpp"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "EEPROMConfig.hpp"
#include "Node.hpp"
#include "UARTConsole.hpp"
#include "CLI.hpp"
#include "ota/OTAUpdater.hpp"

Node nodeId;
DeviceConfig g_device_config = { false, nodeId };


extern "C" void app_main(void)
{   
    esp_sleep_wakeup_cause_t wakeup_reason;
    esp_reset_reason_t reset_reason;
    EEPROMConfig eeprom;
    Communication comm(ConnectionType::WIFI);
    ActivatePacket activate(g_device_config.nodeId.getNodeID(), ACT_URI, ACT_TAG);
    ReadingPacket readings(g_device_config.nodeId.getNodeID(), DATA_URI, DATA_TAG);

    wakeup_reason = esp_sleep_get_wakeup_cause();
    reset_reason = esp_reset_reason();

    // Start CLI task
    #if CLI_EN == 1
        UARTConsole::init(115200);
        ESP_ERROR_CHECK(esp_netif_init());
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        CLI::start();
    #endif

    vTaskDelay(pdMS_TO_TICKS(500));
    
    if (!eeprom.begin()) 
    {
        ESP_LOGE("MAIN", "Failed to open EEPROM config");
        return;
    }

    if (!eeprom.loadConfig(g_device_config)) 
    {
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
                g_device_config.has_activated = true;
                eeprom.saveConfig(g_device_config);
            } 
            else 
            {
                ESP_LOGI("MAIN", "Already activated.");
            }

            ESP_LOGI("MAIN", "Collecting sensor readings...");
            readings.readSensor();
            readings.sendPacket();
            
            // BatteryPacket battery(g_device_config.nodeId.getNodeID(), BATT_URI, 0, 0, BATT_TAG);
            // if (!battery.readFromBMS()) {
            //     ESP_LOGE("MAIN", "Failed to read battery.");
            // } else {
            //     ESP_LOGI("MAIN", "Sending battery packet...");
            //     battery.sendPacket();
            // }

            // Only run OTA update on power-on (not on reboot or deep sleep wakeup)

            #if OTA_EN == 1
                {
                    OTAUpdater ota;

                    if (reset_reason == ESP_RST_POWERON) 
                    {
                        const char * url = "http://45.79.118.187:8080/release/latest/cn1.bin";
                        printf("Power-on detected. Checking for OTA update...\n");

                        if (ota.update(url)) 
                        {
                            printf("OTA OK. Rebooting...\n");
                            vTaskDelay(pdMS_TO_TICKS(2000));
                            esp_restart();
                        } 
                        else 
                        {
                            printf("OTA FAILED or no update needed\n");
                        }
                    }
                }
            #endif

            /**
             * Existing bug: If CLI is enabled, after executing battery section 
             * CLI is non-responsive. May be becuase of the UART pins. Battery could
             * be using UART0 for communication with BMS???
             */
            #if CLI_EN == 1
                for (;;) //WILL REMOVE THIS LATER...
                {
                    vTaskDelay(pdMS_TO_TICKS(1000));
                }
            #endif

            
            if (comm.isConnected())
                comm.disconnect();
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
        ESP_LOGI("MAIN", "Going into deep sleep...");
        esp_sleep_enable_timer_wakeup(sleep_time_sec * 1000000ULL);
        esp_deep_sleep_start();
    #endif
}