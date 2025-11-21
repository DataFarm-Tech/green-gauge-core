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

Node nodeId;
DeviceConfig g_device_config = { false, nodeId };

// #define CLI_UART UART_NUM_0
// #define BUF_SIZE 128
// #define MAX_ARGS 4

// // Command handler function type
// typedef void (*cmd_handler_t)(int argc, char** argv);

// // Command structure
// typedef struct {
//     const char* name;
//     const char* help;
//     cmd_handler_t handler;
//     int min_args;  // Minimum number of arguments (excluding command name)
// } cli_command_t;

// // OTA Update Handler
// class OTAUpdater {
// private:
//     static esp_err_t http_event_handler(esp_http_client_event_t *evt) {
//         switch (evt->event_id) {
//             case HTTP_EVENT_ERROR:
//                 ESP_LOGE("OTA", "HTTP_EVENT_ERROR");
//                 break;
//             case HTTP_EVENT_ON_CONNECTED:
//                 ESP_LOGI("OTA", "HTTP_EVENT_ON_CONNECTED");
//                 break;
//             case HTTP_EVENT_HEADER_SENT:
//                 ESP_LOGI("OTA", "HTTP_EVENT_HEADER_SENT");
//                 break;
//             case HTTP_EVENT_ON_HEADER:
//                 ESP_LOGD("OTA", "HTTP_EVENT_ON_HEADER, key=%s, value=%s", 
//                         evt->header_key, evt->header_value);
//                 break;
//             case HTTP_EVENT_ON_DATA:
//                 ESP_LOGD("OTA", "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
//                 break;
//             case HTTP_EVENT_ON_FINISH:
//                 ESP_LOGI("OTA", "HTTP_EVENT_ON_FINISH");
//                 break;
//             case HTTP_EVENT_DISCONNECTED:
//                 ESP_LOGI("OTA", "HTTP_EVENT_DISCONNECTED");
//                 break;
//             default:
//                 break;
//         }
//         return ESP_OK;
//     }

// public:
//     bool performUpdate(const char* url) {
//         ESP_LOGI("OTA", "Starting OTA update from URL: %s", url);

//         esp_http_client_config_t config = {};
//         config.url = url;
//         config.event_handler = http_event_handler;
//         config.timeout_ms = 30000;
//         config.keep_alive_enable = true;
//         config.buffer_size = 1024;
//         config.buffer_size_tx = 1024;
        
//         // Check if URL is HTTP or HTTPS
//         bool is_https = (strncmp(url, "https://", 8) == 0);
        
//         if (!is_https) {
//             // For plain HTTP, we don't need certificate verification
//             // Set cert_pem to empty string to bypass the check
//             config.cert_pem = "";
//         }

//         esp_https_ota_config_t ota_config = {};
//         ota_config.http_config = &config;

//         ESP_LOGI("OTA", "Attempting to download update...");
//         esp_err_t ret = esp_https_ota(&ota_config);

//         if (ret == ESP_OK) {
//             ESP_LOGI("OTA", "OTA update successful!");
//             return true;
//         } else {
//             ESP_LOGE("OTA", "OTA update failed! Error: %s (0x%x)", 
//                     esp_err_to_name(ret), ret);
//             return false;
//         }
//     }

//     // Get running partition information
//     static void printPartitionInfo() {
//         const esp_partition_t *running = esp_ota_get_running_partition();
//         esp_ota_img_states_t ota_state;
        
//         if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
//             ESP_LOGI("OTA", "Running partition: %s at offset 0x%lx", 
//                     running->label, running->address);
//             ESP_LOGI("OTA", "OTA state: %d", ota_state);
//         }
//     }
// };

// // Forward declarations of command handlers
// void cmd_status(int argc, char** argv);
// void cmd_reset(int argc, char** argv);
// void cmd_help(int argc, char** argv);
// void cmd_install(int argc, char** argv);
// void cmd_partition(int argc, char** argv);

// // Command table
// static const cli_command_t commands[] = {
//     {"status",    "Show device status", cmd_status, 0},
//     {"reset",     "Reboot the device", cmd_reset, 0},
//     {"help",      "Show available commands", cmd_help, 0},
//     {"install",   "Install firmware: install <ip_addr> <file_path>", cmd_install, 2},
//     {"partition", "Show partition information", cmd_partition, 0},
//     {NULL, NULL, NULL, 0}  // Terminator
// };

// // Helper function to write to UART
// void cli_print(const char* str) {
//     uart_write_bytes(CLI_UART, str, strlen(str));
// }

// void cli_printf(const char* format, ...) {
//     char buffer[256];
//     va_list args;
//     va_start(args, format);
//     vsnprintf(buffer, sizeof(buffer), format, args);
//     va_end(args);
//     cli_print(buffer);
// }

// // Command handler implementations
// void cmd_status(int argc, char** argv) {
//     cli_print("Device Status:\r\n");
//     cli_printf("  Node ID: %s\r\n", g_device_config.nodeId.getNodeID());
//     cli_printf("  Activated: %s\r\n", g_device_config.has_activated ? "Yes" : "No");
//     cli_printf("  Free Heap: %lu bytes\r\n", esp_get_free_heap_size());
//     cli_print("  Status: OK\r\n");
// }

// void cmd_reset(int argc, char** argv) {
//     cli_print("Rebooting device...\r\n");
//     vTaskDelay(200 / portTICK_PERIOD_MS);
//     esp_restart();
// }

// void cmd_help(int argc, char** argv) {
//     cli_print("Available commands:\r\n");
//     for (int i = 0; commands[i].name != NULL; i++) {
//         cli_printf("  %-10s - %s\r\n", commands[i].name, commands[i].help);
//     }
// }

// void cmd_install(int argc, char** argv) {
//     if (argc < 3) {
//         cli_print("Usage: install <ip_addr> <file_path>\r\n");
//         cli_print("Example: install 192.168.1.100 /firmware.bin\r\n");
//         return;
//     }

//     const char* ip_addr = argv[1];
//     const char* file_path = argv[2];

//     char url[256];
//     snprintf(url, sizeof(url), "http://%s%s", ip_addr, file_path);
    
//     cli_printf("Starting OTA update from: %s\r\n", url);
//     cli_print("This may take several minutes...\r\n");
    
//     // Initialize network if not already done
//     // esp_err_t ret = esp_netif_init();
//     // if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
//     //     cli_printf("Failed to initialize network: %s\r\n", esp_err_to_name(ret));
//     //     return;
//     // }

//     // ret = esp_event_loop_create_default();
//     // if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
//     //     cli_printf("Failed to create event loop: %s\r\n", esp_err_to_name(ret));
//     //     return;
//     // }

//     OTAUpdater updater;
//     if (updater.performUpdate(url)) {
//         cli_print("OTA update successful! Rebooting in 3 seconds...\r\n");
//         vTaskDelay(3000 / portTICK_PERIOD_MS);
//         esp_restart();
//     } else {
//         cli_print("OTA update failed! Check the URL and network connection.\r\n");
//     }
// }

// void cmd_partition(int argc, char** argv) {
//     const esp_partition_t *running = esp_ota_get_running_partition();
//     const esp_partition_t *boot = esp_ota_get_boot_partition();
    
//     cli_print("Partition Information:\r\n");
//     cli_printf("  Running partition: %s (0x%lx)\r\n", running->label, running->address);
//     cli_printf("  Boot partition: %s (0x%lx)\r\n", boot->label, boot->address);
    
//     esp_ota_img_states_t ota_state;
//     if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
//         const char* state_str;
//         switch (ota_state) {
//             case ESP_OTA_IMG_NEW: state_str = "NEW"; break;
//             case ESP_OTA_IMG_PENDING_VERIFY: state_str = "PENDING_VERIFY"; break;
//             case ESP_OTA_IMG_VALID: state_str = "VALID"; break;
//             case ESP_OTA_IMG_INVALID: state_str = "INVALID"; break;
//             case ESP_OTA_IMG_ABORTED: state_str = "ABORTED"; break;
//             default: state_str = "UNKNOWN"; break;
//         }
//         cli_printf("  OTA state: %s\r\n", state_str);
//     }
    
//     // List all OTA partitions
//     esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_APP, 
//                                                       ESP_PARTITION_SUBTYPE_ANY, NULL);
//     cli_print("  Available partitions:\r\n");
//     while (it != NULL) {
//         const esp_partition_t *part = esp_partition_get(it);
//         cli_printf("    - %s: 0x%lx (size: %lu bytes)\r\n", 
//                   part->label, part->address, part->size);
//         it = esp_partition_next(it);
//     }
//     esp_partition_iterator_release(it);
// }

// // Parse command line into argc/argv
// int parse_command(char* line, char** argv, int max_args) {
//     int argc = 0;
//     char* token = strtok(line, " \t");
    
//     while (token != NULL && argc < max_args) {
//         argv[argc++] = token;
//         token = strtok(NULL, " \t");
//     }
    
//     return argc;
// }

// // Execute command
// void execute_command(char* line) {
//     char* argv[MAX_ARGS];
//     int argc = parse_command(line, argv, MAX_ARGS);
    
//     if (argc == 0) {
//         return;
//     }
    
//     // Find and execute command
//     for (int i = 0; commands[i].name != NULL; i++) {
//         if (strcmp(argv[0], commands[i].name) == 0) {
//             if (argc - 1 < commands[i].min_args) {
//                 cli_printf("Error: '%s' requires at least %d argument(s)\r\n", 
//                           commands[i].name, commands[i].min_args);
//                 cli_printf("Usage: %s\r\n", commands[i].help);
//                 return;
//             }
//             commands[i].handler(argc, argv);
//             return;
//         }
//     }
    
//     cli_printf("Unknown command: '%s'. Type 'help' for available commands.\r\n", argv[0]);
// }

// extern "C" void cli_task(void *pvParameters)
// {
//     uint8_t buf[BUF_SIZE];
//     char line[BUF_SIZE * 2];
//     int idx = 0;

//     ESP_LOGI("CLI", "CLI task started.");
//     cli_print("\r\nCLI Ready. Type 'help' for commands.\r\n> ");

//     while (true)
//     {
//         int len = uart_read_bytes(CLI_UART, buf, 1, 10 / portTICK_PERIOD_MS);

//         if (len > 0)
//         {
//             char c = buf[0];

//             if (c == '\r' || c == '\n')
//             {
//                 line[idx] = '\0';
//                 cli_print("\r\n");

//                 if (idx > 0) {
//                     execute_command(line);
//                 }

//                 idx = 0;
//                 cli_print("> ");
//             }
//             else if (c == 0x7F || c == 0x08)  // Backspace or DEL
//             {
//                 if (idx > 0) {
//                     idx--;
//                     cli_print("\b \b");
//                 }
//             }
//             else if (c >= 0x20 && c < 0x7F)  // Printable characters
//             {
//                 if (idx < sizeof(line) - 1) {
//                     line[idx++] = c;
//                     uart_write_bytes(CLI_UART, &c, 1);  // Echo
//                 }
//             }
//         }
//     }
// }

extern "C" void app_main(void)
{
    printf("ENDING KERNEL STARTUP...\n");
    // Start CLI task
    UARTConsole::init(115200);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    CLI::start();

    // Original application code (commented out for CLI-only mode)
    
    vTaskDelay(pdMS_TO_TICKS(500));
    
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

    while (1) {

        Communication comm(ConnectionType::WIFI);

        if (comm.connect()) {
            for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
            if (!g_device_config.has_activated) {
                ESP_LOGI("MAIN", "Sending activation packet...");
                ActivatePacket activate(g_device_config.nodeId.getNodeID(), ACT_URI, ACT_TAG);
                activate.sendPacket();
                g_device_config.has_activated = true;
                eeprom.saveConfig(g_device_config);
            } else {
                ESP_LOGI("MAIN", "Already activated.");
            }

            ReadingPacket readings(g_device_config.nodeId.getNodeID(), DATA_URI, DATA_TAG);
            ESP_LOGI("MAIN", "Collecting sensor readings...");
            readings.readSensor();
            readings.sendPacket();

            BatteryPacket battery(g_device_config.nodeId.getNodeID(), BATT_URI, 0, 0, BATT_TAG);
            if (!battery.readFromBMS()) {
                ESP_LOGE("MAIN", "Failed to read battery.");
            } else {
                ESP_LOGI("MAIN", "Sending battery packet...");
                battery.sendPacket();
            }

            // if (comm.isConnected())
            //     comm.disconnect();
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