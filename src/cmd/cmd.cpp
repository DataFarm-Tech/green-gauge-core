#include <Arduino.h>
#include "cmd.h"
#include "msg_queue.h"
#include "th/th_handler.h"
#include "utils.h"
#include <WiFi.h>
#include <ESP32Ping.h>
#include "ntp/ntp.h"
#include "interrupts.h"
#include <queue>
#include <mutex>
#include <Arduino.h>
#include "config.h"
#include "eeprom/eeprom.h"
#include "mh/mutex_h.h"
#include "lora/lora_listener.h"
#include "http/https_comms.h"
#include "main_app/main_app.h"

/**
 * @brief A command to show the help text
 */
void cmd_help() 
{
    cli_printf("Available commands:\n");
    cli_printf("  help    - Show this help message\n");
    cli_printf("  exit    - Exit the CLI\n");
    cli_printf("  reboot  - Reboot this device\n");
    cli_printf("  queue   - Print contents of internal message queue\n");
    cli_printf("  ping [host]  - Ping a host\n");
    cli_printf("  clear - Clears the screen\n");
    cli_printf(" threads - Shows the active threads\n");
    cli_printf(" ipconfig - Shows the network config\n");
    cli_printf(" time - Shows the current NTP time\n");
    cli_printf(" teardown - Removes all threads and clears queue\n");
    cli_printf(" q_add - Adds an element to the queue\n");
    cli_printf(" key - Shows the active http key used\n");
    cli_printf(" clear-config - Clears the current config.\n");
    cli_printf(" list - Lists all the nodes belonging to itself.\n");
    cli_printf(" cache - Lists items in cache.\n");
    cli_printf(" stop_thread [lora_listener_th, main_app_th, http_th] - stops a particular thread.\n");
    cli_printf(" start_thread [lora_listener_th, main_app_th, http_th] - stops a particular thread.\n");
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
 * @brief A command to show the current sys time.
 * This time is the NTP time, being retrieved by the NTP client.
 */
void cmd_time()
{
    uint32_t currentTime;

    if (!get_sys_time(&currentTime))
    {
        return;
    }

    printf("time: %d\n", currentTime);
    
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
    // Print IPv4 Configuration
    IPAddress ip = WiFi.localIP();
    IPAddress subnet = WiFi.subnetMask();
    IPAddress gateway = WiFi.gatewayIP();
    IPAddress dns = WiFi.dnsIP();

    printf("=== IPv4 Configuration ===\n");
    printf("IP Address: %s\n", ip.toString().c_str());
    printf("Subnet Mask: %s\n", subnet.toString().c_str());
    printf("Gateway Address: %s\n", gateway.toString().c_str());
    printf("DNS Server: %s\n", dns.toString().c_str());
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
    if (xSemaphoreTake(msg_queue_mh, portMAX_DELAY) == pdTRUE) {
        cli_printf("Queue size: %zu\n", internal_msg_q.size());
        if (internal_msg_q.empty()) 
        {
            cli_print("Queue is empty.\n");
            xSemaphoreGive(msg_queue_mh);
            return;
        }
        int index = 0;
        size_t size = internal_msg_q.size();
    
        for (size_t i = 0; i < size; ++i) 
        {
            msg m = internal_msg_q.front();
            cli_printf("  [%d] src_node: %s, des_node: %s\n", index++, m.src_node.c_str(), m.des_node.c_str());
    
            internal_msg_q.pop();
            internal_msg_q.push(m);  // Rotate to preserve order
        }
    
        xSemaphoreGive(msg_queue_mh);
    }
}

/**
 * @brief The following command adds an element to the q.
 */
void cmd_add_queue()
{
    msg new_message;
    new_message.src_node = "tnode1";
    new_message.des_node = "controller";
    
    // Generate some sensor data (you can replace with real sensor readings)
    new_message.data = {
        .rs485_humidity = uint8_t(random(30, 60)),
        .rs485_temp = uint8_t(random(20, 30)),
        .rs485_con = uint8_t(random(20, 40)),
        .rs485_ph = uint8_t(random(5, 9)),
        .rs485_nit = uint8_t(random(40, 60)),
        .rs485_phos = uint8_t(random(30, 50)),
        .rs485_pot = uint8_t(random(25, 45))
    };

    if (xSemaphoreTake(msg_queue_mh, portMAX_DELAY) == pdTRUE)
    {
        internal_msg_q.push(new_message); 
        xSemaphoreGive(msg_queue_mh);
    }
    
}


void cmd_key()
{
    cli_printf("Key: %s\n", config.api_key);
}

void cmd_clear_config()
{
    printf("Clearing config\n");
    clear_config();
}

void cmd_node_list()
{
    cli_printf("Node IDs:\n");
    for (int i = 0; i < node_count; i++) 
    {
        cli_printf("%s ", (node_list)[i]);
    }
    cli_printf("\n");
}

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