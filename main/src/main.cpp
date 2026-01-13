#include <iostream>
#include "Communication.hpp"
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
#include "BatteryPacket.hpp"
#include "ReadingPacket.hpp"
#include "ActivatePacket.hpp"
#include "Config.hpp"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "EEPROMConfig.hpp"
#include "Node.hpp"
#include "UARTConsole.hpp"
#include "CLI.hpp"
#include "OTAUpdater.hpp"
#include "Logger.hpp"

Node nodeId;
DeviceConfig g_device_config = { false, nodeId };

#if CLI_EN == 1
// Create UART console instance for serial monitor
static UARTConsole serialConsole(UART_NUM_0);
#endif

extern "C" void app_main(void)
{   
    // Initialize logger (auto-mounts filesystem)
    g_logger.init();
    g_logger.info("System booting...");

    #if OTA_EN == 1
    esp_reset_reason_t reset_reason = esp_reset_reason();
    #endif

    Communication comm(ConnectionType::WIFI);
    ActivatePacket activate(std::string(g_device_config.nodeId.getNodeID()), ACT_URI, ACT_TAG);
    ReadingPacket readings(std::string(g_device_config.nodeId.getNodeID()), DATA_URI, DATA_TAG);

    // Start CLI task
    #if CLI_EN == 1
        serialConsole.init(115200);  // Initialize UART console instance
        ESP_ERROR_CHECK(esp_netif_init());
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        CLI::start(serialConsole);  // Pass console instance to CLI
    #endif
    
    if (!eeprom.begin()) 
    {
        g_logger.error("Failed to open EEPROM config");
        return;
    }

    if (!eeprom.loadConfig(g_device_config)) 
    {
        g_logger.info("No previous config found, initializing defaults");
        g_device_config.has_activated = false;
        eeprom.saveConfig(g_device_config);
    }

    while (1) 
    {
        if (comm.connect()) 
        {
            g_logger.info("Device connected to WLAN0");
            
            if (!g_device_config.has_activated) 
            {
                g_logger.info("Sending activation packet...");
                activate.sendPacket();

                g_device_config.has_activated = true;
                g_logger.info("Activation complete, saving config");
                eeprom.saveConfig(g_device_config);
            } 
            else 
            {
                g_logger.info("Already activated");
            }

            g_logger.info("Collecting sensor readings...");
            readings.readSensor();
            readings.sendPacket();
            
            // BatteryPacket battery(g_device_config.nodeId.getNodeID(), BATT_URI, 0, 0, BATT_TAG);
            // if (!battery.readFromBMS()) {
            //     g_logger.error("Failed to read battery");
            // } else {
            //     g_logger.info("Sending battery packet...");
            //     battery.sendPacket();
            // }

            #if OTA_EN == 1
                if (reset_reason == ESP_RST_POWERON) 
                {
                    OTAUpdater ota;
                    const char * url = "http://45.79.118.187:8080/release/latest/cn1.bin";
                    
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

            #if CLI_EN == 1
                // Keep CLI running
                for (;;) 
                {
                    vTaskDelay(pdMS_TO_TICKS(1000));
                }
            #endif

            if (comm.isConnected())
                comm.disconnect();
        }
        else 
        {
            g_logger.error("Unable to connect to network");
        }

        eeprom.close();

        #if DEEP_SLEEP_EN == 1
            break;
        #else
            vTaskDelay(pdMS_TO_TICKS(sleep_time_sec * 1000));
        #endif
    }

    #if DEEP_SLEEP_EN == 1
        g_logger.info("Entering deep sleep for %d seconds", sleep_time_sec);
        esp_sleep_enable_timer_wakeup(sleep_time_sec * 1000000ULL);
        esp_deep_sleep_start();
    #endif
}