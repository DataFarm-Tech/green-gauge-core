#include <Arduino.h>
#include "cmd.h"
#include "msg_queue.h"
#include "th_handler.h"
#include "utils.h"
#include <WiFi.h>
#include <ESP32Ping.h>


void cmd_help() {
    cli_printf("Available commands:\n");
    cli_printf("  help    - Show this help message\n");
    cli_printf("  exit    - Exit the CLI\n");
    cli_printf("  reboot  - Reboot this device\n");
    cli_printf("  queue   - Print contents of internal message queue\n");
    cli_printf("  ping [host]  - Ping a host\n");
    cli_printf("  clear - Clears the screen\n");
    cli_printf(" threads - Shows the active threads\n");
}

void cmd_exit() {
    cli_print("Exiting CLI...\n");
    delay(1000);
    delete_th(read_serial_cli_th);
    // ESP.restart(); // if needed
}


void cmd_clear() {
    for (int i = 0; i < 50; i++) {
        cli_print("\n");
    }
}

void cmd_threads()
{
        cli_printf("=== Thread Status ===\n");
    
        if (read_serial_cli_th != NULL) {
            cli_printf("read_serial_cli_th: Running\n");
        }
    
        if (process_state_ch_th != NULL) {
            cli_printf("process_state_ch_th: Running\n");
        }
    
        if (lora_listener_th != NULL) {
            cli_printf("lora_listener_th: Running\n");
        }
    
        if (main_app_th != NULL) {
            cli_printf("main_app_th: Running\n");
        }
    
        if (http_th != NULL) {
            cli_printf("http_th: Running\n");
        }
    
}


void cmd_ping(const char* host)
{

    if (WiFi.status() != WL_CONNECTED)
    {
        printf("Not connected to Wifi...\n");
        return;
    }
    
    
    printf("\n[PING]: Pinging %s...\n", host);

    if (Ping.ping(host), 5) 
    {
        printf("[PING] Success! Avg time: %d ms\n", Ping.averageTime());
    } 
    else 
    {
        printf("[PING] Failed.\n");
    }
}

void cmd_reboot() {
    cli_print("CLI Triggered Reboot\n");
    delay(1000);
    ESP.restart();
}

void cmd_queue() {
    queue_mutex.lock();
    
    cli_printf("Queue size: %zu\n", internal_msg_q.size());

    if (internal_msg_q.empty()) {
        cli_print("Queue is empty.\n");
        queue_mutex.unlock();
        return;
    }

    int index = 0;
    size_t size = internal_msg_q.size();

    for (size_t i = 0; i < size; ++i) {
        msg m = internal_msg_q.front();
        cli_printf("  [%d] src_node: %s, des_node: %s\n", index++, m.src_node.c_str(), m.des_node.c_str());

        internal_msg_q.pop();
        internal_msg_q.push(m);  // Rotate to preserve order
    }

    queue_mutex.unlock();
}
