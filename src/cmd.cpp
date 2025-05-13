#include <Arduino.h>
#include "cmd.h"
#include "msg_queue.h"
#include "th_handler.h"
#include "utils.h"
#include <WiFi.h>
#include <ESP32Ping.h>
#include "interrupts.h"
#include <queue>
#include <mutex>
#include <Arduino.h>
#include "config.h"
#include "eeprom.h"
#include "mutex_h.h"
#include "lora_listener.h"
#include "https_comms.h"
#include "hw.h"
#include "main_app.h"
#include "pack_def.h"
#include "err_handle.h"
/**
 * @brief A command to show the help text
 */
void cmd_help() 
{
    switch (current_state)
    {
        case UNDEFINED_STATE:
            printf("Device is in UNDEFINED state.\n");
            break;
        case SENSOR_STATE:
            printf("Available commands:\n");
            printf("  help    - Show this help message\n");
            printf("  exit    - Exit the CLI\n");
            printf("  state   - Shows the current state of the device\n");
            printf("  reboot  - Reboot this device\n");
            printf("  threads - Show active threads\n");
            printf("  teardown - Removes all threads and clears queue\n");
            printf("  clear-config - Clears the current config.\n");
            printf("  cache - Lists items in cache.\n");
            printf("  clear - Clears the screen\n");
            printf("  stop_thread [lora_listener_th, main_app_th, http_th] - stops a particular thread.\n");
            printf("  start_thread [lora_listener_th, main_app_th, http_th] - stops a particular thread.\n");
            printf("  apply - Saves the current config.\n");
            break;
        case CONTROLLER_STATE:
            printf("Available commands:\n");
            printf("  help    - Show this help message\n");
            printf("  exit    - Exit the CLI\n");
            printf("  reboot  - Reboot this device\n");
            printf("  threads - Show active threads\n");
            printf("  teardown - Removes all threads and clears queue\n");
            printf("  clear-config - Clears the current config.\n");
            printf("  cache - Lists items in cache.\n");
            printf("  stop_thread [lora_listener_th, main_app_th, http_th] - stops a particular thread.\n");
            printf("  start_thread [lora_listener_th, main_app_th, http_th] - stops a particular thread.\n");
            printf("  list    - Lists all the nodes belonging to itself.\n");
            printf("  ping [host]  - Ping a host\n");
            printf("  key     - Shows the active http key used\n");
            printf("  disconnect_wifi - Disconnects from WiFi\n");
            printf("  disconnect_wifi erase - Disconnects from WiFi and erases credentials\n");
            printf("  ipconfig - Shows the network config\n");
            printf("  queue   - Print contents of internal message queue\n");
            printf("  q_add   - Adds an element to the queue\n");
            printf("  clear   - Clears the screen\n");
            printf("  state   - Shows the current state of the device\n");
            printf("  apply - Saves the current config.\n");

            break;
        
        default:
            break;
    }
}

/**
 * @brief A command to exit the CLI (deleting the CLI thread)
 */
void cmd_exit() 
{
    cli_print("Exiting CLI...\n");
    delay(1000);
    delete_th(&read_serial_cli_th);
}

/**
 * @brief A command to clear the screen. A bit of a hack
 * Only does 50 carraige returns.
 */
void cmd_clear() 
{
    for (int i = 0; i < 50; i++) 
    {
        cli_print("\n");
    }
}

/**
 * @brief The following cmd shows all the active threads
 */
void cmd_threads()
{
    cli_printf("=== Thread Status ===\n");

    if (read_serial_cli_th != NULL) 
    {
        cli_printf("read_serial_cli_th: Running\n");
    }

    if (process_state_ch_th != NULL) 
    {
        cli_printf("process_state_ch_th: Running\n");
    }

    if (lora_listener_th != NULL) 
    {
        cli_printf("lora_listener_th: Running\n");
    }

    if (main_app_th != NULL) 
    {
        cli_printf("main_app_th: Running\n");
    }

    if (http_th != NULL) 
    {
        cli_printf("http_th: Running\n");
    }

    return;
}

/**
 * @brief the following command pings a given host
 */
void cmd_ping(const char* host)
{
    if (host == NULL) 
    {
        printf("Error: ping requires a host argument.\n");
        return;
    }

    if (WiFi.status() != WL_CONNECTED)
    {
        printf("Not connected to Wifi...\n");
        err_led_state(WIFI, INT_STATE_ERROR);
        return;
    }

    // Try to resolve the host name to an IP address
    IPAddress resolved_ip;
    if (!WiFi.hostByName(host, resolved_ip))
    {
        printf("[PING] DNS Failed for %s\n", host);
        return;
    }

    printf("\n[PING]: Pinging %s (%s)...\n", host, resolved_ip.toString().c_str());

    // Try pinging the resolved IP address 5 times
    if (Ping.ping(resolved_ip, 5)) 
    {
        printf("[PING] Success! Avg time: %f ms\n", Ping.averageTime());
    } 
    else 
    {
        printf("[PING] Failed to reach %s.\n", host);
    }
}

/**
 * @brief The following command prints the networking information
 */
void cmd_ipconfig()
{
    // Get the IPv4 configuration
    IPAddress ip = WiFi.localIP();
    IPAddress subnet = WiFi.subnetMask();
    IPAddress gateway = WiFi.gatewayIP();
    IPAddress dns = WiFi.dnsIP();
    
    // Check if all IP components are 0.0.0.0
    if (ip == IPAddress(0, 0, 0, 0) && subnet == IPAddress(0, 0, 0, 0) && gateway == IPAddress(0, 0, 0, 0) && dns == IPAddress(0, 0, 0, 0)) {
        printf("Not connected to WiFi\n");
    } else {
        // Print IPv4 Configuration
        printf("=== IPv4 Configuration ===\n");
        printf("IP Address: %s\n", ip.toString().c_str());
        printf("Subnet Mask: %s\n", subnet.toString().c_str());
        printf("Gateway Address: %s\n", gateway.toString().c_str());
        printf("DNS Server: %s\n", dns.toString().c_str());
    }
}



/**
 * @brief This command reboots the esp32
 */
void cmd_reboot() 
{
    cli_print("CLI Triggered Reboot\n");
    delay(1000);
    ESP.restart();
}

/**
 * @brief This command reboots the esp32
 */
void cmd_teardown() 
{
    cli_print("Cleaning up threads and Q\n");
    tear_down();
}

/**
 * @brief This command shows the size of the queue and the elements
 * in it.
 */
void cmd_queue() 
{
    sn001_suc_rsp m;
    
    switch (current_state)
    {
        case UNDEFINED_STATE:
        case SENSOR_STATE:
            printf("Cannot show queue in the current state.\n");
            break;
        case CONTROLLER_STATE:
            if (msg_queue_mh != NULL) 
            {
                if (xSemaphoreTake(msg_queue_mh, portMAX_DELAY) == pdTRUE) 
                    {
                        cli_printf("Queue size: %zu\n", internal_msg_q.size());
                        
                        if (internal_msg_q.empty()) 
                        {
                            cli_print("Queue is empty.\n");
                            xSemaphoreGive(msg_queue_mh);
                            return;
                        }
                        
                        size_t size = internal_msg_q.size();
                        int index = 0;
                    
                        for (size_t i = 0; i < size; ++i) 
                        {
                            m = internal_msg_q.front();
                            cli_printf("  [%d] src_node: %s, des_node: %s\n", index++, m.src_node.c_str(), m.des_node.c_str());
                    
                            internal_msg_q.pop();
                            internal_msg_q.push(m);  // Rotate to preserve order
                        }
                    
                        xSemaphoreGive(msg_queue_mh);
                    }    
            }
            else
            {
                printf("Error: msg_queue_mh is NULL.\n");
                break;
            }
            
            break;
    default:
        break;
    }
}

/**
 * @brief The following command adds an element to the q.
 */
void cmd_add_queue(const char * src_node, const char * des_node)
{
    sn001_suc_rsp new_message;

    switch (current_state)
    {
        case UNDEFINED_STATE:
        case SENSOR_STATE:
            printf("Cannot add to queue in the current state.\n");
            break;
        case CONTROLLER_STATE:

            if (src_node == NULL || strlen(src_node) != ADDRESS_SIZE)
            {
                src_node = "sn0001";
            }

            if (des_node == NULL || strlen(des_node) != ADDRESS_SIZE)
            {
                des_node = "cn0001";
            }
            
            new_message.src_node = src_node;
            new_message.des_node = des_node;
            
            new_message.data = {
                .rs485_humidity = uint8_t(random(30, 60)),
                .rs485_temp = uint8_t(random(20, 30)),
                .rs485_con = uint8_t(random(20, 40)),
                .rs485_ph = uint8_t(random(5, 9)),
                .rs485_nit = uint8_t(random(40, 60)),
                .rs485_phos = uint8_t(random(30, 50)),
                .rs485_pot = uint8_t(random(25, 45))
            };

            if (msg_queue_mh != NULL) 
            {
                if (xSemaphoreTake(msg_queue_mh, portMAX_DELAY) == pdTRUE)
                {
                    internal_msg_q.push(new_message); 
                    xSemaphoreGive(msg_queue_mh);
                }
                break;
            }
            else
            {
                printf("Error: msg_queue_mh is NULL.\n");
                break;
            }
    default:
        break;
    }
}

/**
 * @brief This command shows the current API key
 * @return None
 */
void cmd_key()
{
    switch (current_state)
    {
        case SENSOR_STATE:
        case UNDEFINED_STATE:
            printf("Cannot show API key in the current state.\n");
            break;
        case CONTROLLER_STATE:
            if (strlen(config.api_key) == 0)
            {
                printf("No API key set.\n");
                break;
            }
            cli_printf("API Key: %s\n", config.api_key);
            break;
        default:
            break;
        }
}

/**
 * @brief This command lists all the nodes
 * in the node list.
 * @return None
 */
void cmd_node_list()
{
    switch (current_state)
    {
        case UNDEFINED_STATE:
        case SENSOR_STATE:
            printf("Cannot show node list in the current state.\n");
            break;
        case CONTROLLER_STATE:
            if (node_count > 0)
            {
                cli_printf("Node IDs:\n");
                for (int i = 0; i < node_count; i++) 
                {
                    cli_printf("%s ", (node_list)[i]);
                }
                cli_printf("\n");
                break;
            }
            else
            {
                cli_printf("No nodes in the list.\n");
                break;
            }
            
    default:
        break;
    }
}

/**
 * @brief This command prints the contents of the hash cache
 * @return None
 */
void cmd_cache()
{
    Serial.println("Hash Cache Contents:");
    for (int i = 0; i < config.cache.count; i++) {
        uint8_t index = (config.cache.head + i) % HASH_CACHE_SIZE;
        Serial.print("Entry ");
        Serial.print(i + 1);
        Serial.print(": ");
        
        // Print the hash as hexadecimal
        for (int j = 0; j < HASH_SIZE; j++) {
            Serial.print(config.cache.entries[index][j], HEX);
            if (j < HASH_SIZE - 1) {
                Serial.print(":");
            }
        }
        Serial.println();

    }
}

/**
 * @brief This command stops a thread
 * @param thread_name - The name of the thread to stop
 * @return None
 */
void cmd_stop_thread(const char* thread_name)
{
    if (thread_name == NULL) 
    {
        printf("Error: thread_stop requires a thread_name argument.\n");
        return;
    }

    if (strcmp(thread_name, "lora_listener_th") == 0) 
    {
        delete_th(&lora_listener_th);
        printf("Stopped lora_listener_th\n");
    } 
    else if (strcmp(thread_name, "main_app_th") == 0) 
    {
        delete_th(&main_app_th);
        printf("Stopped main_app_th\n");
    } 
    else if (strcmp(thread_name, "http_th") == 0) 
    {
        delete_th(&http_th);
        printf("Stopped http_th\n");
    } 
    else 
    {
        printf("Unknown thread name: %s\n", thread_name);
        return;
    }
}

/**
 * @brief This command checks the current state of the device
 * @return None
 */
void cmd_check_state()
{
    switch (current_state)
    {
        case CONTROLLER_STATE:
            printf("Current state: CONTROLLER\n");
            break;
        case SENSOR_STATE:
            printf("Current state: SENSOR\n");
            break;
        case UNDEFINED_STATE:
            printf("Current state: UNDEFINED\n");
            break;
        
    default:
        break;
    }
}

/**
 * @brief This command starts a thread
 * @param thread_name - The name of the thread to start
 * @return None
 */
void cmd_start_thread(const char * thread_name)
{
    if (thread_name == NULL) 
    {
        printf("Error: thread_start requires a thread_name argument.\n");
        return;
    }

    if (strcmp(thread_name, "lora_listener_th") == 0) 
    {
        create_th(lora_listener, "lora_listener_th", LORA_LISTENER_TH_STACK_SIZE, &lora_listener_th, 1);
        printf("Started lora_listener_th\n");
    } 
    else if (strcmp(thread_name, "main_app_th") == 0) 
    {
        create_th(main_app, "main_app_th", MAIN_APP_TH_STACK_SIZE, &main_app_th, 1);
        printf("Started main_app_th\n");
    } 
    else if (strcmp(thread_name, "http_th") == 0) 
    {
        create_th(http_send, "http_send", HTTP_TH_STACK_SIZE, &http_th, 1);
        printf("Started http_th\n");
    } 
    else 
    {
        printf("Unknown thread name: %s\n", thread_name);
        return;
    }
}

/**
 * @brief This command clears the current config
 * and erases the credentials
 * @param arg
 * @return None
 */
void cmd_disconnect_wifi(const char * arg)
{
    if (arg == NULL) 
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            wifi_disconnect(false); /** if no arg, do not erase credentials */
        }
        else
        {
            printf("Not connected to WiFi.\n");
        }
        
        return;
    }
    else if (strncmp(arg, "erase", strlen(arg)) == 0) 
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            wifi_disconnect(true);
        }
        else
        {
            printf("Not connected to WiFi.\n");
        }

        return;
    } 
    else
    {
        printf("Invalid argument. Use 'erase' to erase credentials.\n");
        return;
    }
}
/**
 * @brief This command connects to the WiFi
 * @return None
 */
void cmd_connect_wifi()
{
    if (WiFi.status() == WL_CONNECTED) 
    {
        printf("Already connected to WiFi.\n");
        return;
    }

    wifi_connect();
}