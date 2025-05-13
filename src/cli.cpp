#include <Arduino.h>
#include <mutex>

#include "cli.h"
#include "config.h"
#include "eeprom.h"
#include "th_handler.h"
#include "utils.h"
#include "msg_queue.h"
#include "cmd.h"
#include "interrupts.h"

char cli_buffer[BUFFER_SIZE];
uint8_t cli_pos = 0;

/**
 * @brief The following function takes a string
 * and returns a cli_cmd type dependent on the str
 */
cli_cmd get_best_enum(const char* token) 
{
    if (strncmp(token, "help", strlen(token)) == 0) return CMD_HELP;
    if (strncmp(token, "exit", strlen(token)) == 0) return CMD_EXIT;
    if (strncmp(token, "reboot", strlen(token)) == 0) return CMD_REBOOT;
    if (strncmp(token, "queue", strlen(token)) == 0) return CMD_QUEUE;
    if (strncmp(token, "ping", strlen(token)) == 0) return CMD_PING;
    if (strncmp(token, "threads", strlen(token)) == 0) return CMD_THREADS;
    if (strncmp(token, "teardown", strlen(token)) == 0) return CMD_TEARDOWN;
    if (strncmp(token, "ipconfig", strlen(token)) == 0) return CMD_IPCONFIG;
    if (strncmp(token, "q_add", strlen(token)) == 0) return CMD_QADD;
    if (strncmp(token, "apply", strlen(token)) == 0) return CMD_APPLY;
    if (strncmp(token, "key", strlen(token)) == 0) return CMD_KEY;
    if (strncmp(token, "clear-config", strlen(token)) == 0) return CMD_CLEAR_CONFIG;
    if (strncmp(token, "clear", strlen(token)) == 0) return CMD_CLEAR;
    if (strncmp(token, "list", strlen(token)) == 0) return CMD_LIST;
    if (strncmp(token, "state", strlen(token)) == 0) return CMD_STATE;
    if (strncmp(token, "cache", strlen(token)) == 0) return CMD_CACHE;
    if (strncmp(token, "stop_thread", strlen(token)) == 0) return CMD_STOP_THREAD;
    if (strncmp(token, "start_thread", strlen(token)) == 0) return CMD_START_THREAD;
    if (strncmp(token, "disconnect_wifi", strlen(token)) == 0) return CMD_DISCONNECT_WIFI;
    if (strncmp(token, "connect_wifi", strlen(token)) == 0) return CMD_CONNECT_WIFI;
    return CMD_UNKNOWN;
}



/**
 * @brief The following function takes a string.
 * And calls get_best_enum on the string, which returns a cli_cmd
 * enum. Then calls respective function.
 */
void handle_cmd(char* cmd) 
{
    char* token = strtok(cmd, " ");
    if (token == nullptr) return;
    
    trim_newline(token);
    
    cli_cmd cmd_input = get_best_enum(token);
    
    char* arg = strtok(nullptr, " ");
    char* arg2 = strtok(nullptr, " ");

    if (arg) trim_newline(arg);
    if (arg2) trim_newline(arg2);
    
    cli_printf("\n");

 
    
    switch (cmd_input) 
    {
        case CMD_HELP:
            cmd_help();
            break;
        case CMD_EXIT:
            cmd_exit();
            break;
        case CMD_REBOOT:
            cmd_reboot();
            break;
        case CMD_QUEUE:
            cmd_queue();
            break;
        case CMD_PING:
            cmd_ping(arg);
            break;
        case CMD_CLEAR:
            cmd_clear();
            break;
        case CMD_THREADS:
            cmd_threads();
            break;
        case CMD_TEARDOWN:
            cmd_teardown();
            break;
        case CMD_IPCONFIG:
            cmd_ipconfig();
            break;
        case CMD_QADD:
            cmd_add_queue(arg, arg2);
            break;
        case CMD_APPLY:
            save_config();
            break;
        case CMD_KEY:
            cmd_key();
            break;
        case CMD_CLEAR_CONFIG:
            clear_config();
            break;
        case CMD_LIST:
            cmd_node_list();
            break;
        case CMD_CACHE:
            cmd_cache();
            break;
        case CMD_STOP_THREAD:
            cmd_stop_thread(arg);
            break;
        case CMD_START_THREAD:
            cmd_start_thread(arg);
            break;
        case CMD_STATE:
            cmd_check_state();
            break;
        case CMD_DISCONNECT_WIFI:
            cmd_disconnect_wifi(arg);
            break;
        case CMD_CONNECT_WIFI:
            cmd_connect_wifi();
            break;
        case CMD_UNKNOWN:
        default:
            cli_printf("Unknown command: %s\n", token);
            break;
    }
}


/**
 * @brief The following function is an active thread that
 * retrieves data via the Serial console. Once a carraige return
 * is provided the thread calls handle_cmd.
 */
void read_serial_cli(void* param) 
{
    print_prompt();

    while (true) 
    {
        bool processedCommand = false;

        while (Serial.available()) 
        {
            char c = Serial.read();

            if (c == '\b' || c == 127) 
            {
                if (cli_pos > 0) 
                {
                    cli_pos--;
                    cli_buffer[cli_pos] = '\0';
                    cli_print("\b \b");
                }
                continue;
            }

            if (c == '\r' || c == '\n') 
            {
                cli_buffer[cli_pos] = '\0';
                if (cli_pos > 0) 
                {
                    handle_cmd(cli_buffer);
                    cli_pos = 0;
                    processedCommand = true;
                }
                continue;
            }

            if (cli_pos < BUFFER_SIZE - 1 && isprint(c)) 
            {
                cli_buffer[cli_pos++] = c;
                cli_print(c);
            }
        }

        if (processedCommand) 
        {
            print_prompt();
            processedCommand = false;
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}