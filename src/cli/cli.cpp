#include <Arduino.h>
#include <mutex>

#include "cli/cli.h"
#include "config.h"
#include "eeprom/eeprom.h"
#include "th/th_handler.h"
#include "utils.h"
#include "msg_queue.h"
#include "cmd/cmd.h"
#include "interrupts.h"

char cli_buffer[BUFFER_SIZE];
uint8_t cli_pos = 0;

/**
 * @brief The following function takes a string
 * and returns a cli_cmd type dependent on the str
 */
cli_cmd get_best_enum(const char* token) 
{
    if (strncmp(token, "help", sizeof(token)) == 0) return CMD_HELP;
    if (strncmp(token, "exit", sizeof(token)) == 0) return CMD_EXIT;
    if (strncmp(token, "reboot", sizeof(token)) == 0) return CMD_REBOOT;
    if (strncmp(token, "queue", sizeof(token)) == 0) return CMD_QUEUE;
    if (strncmp(token, "ping", sizeof(token)) == 0) return CMD_PING;
    if (strncmp(token, "threads", sizeof(token)) == 0) return CMD_THREADS;
    if (strncmp(token, "time", sizeof(token)) == 0) return CMD_TIME;
    if (strncmp(token, "teardown", sizeof(token)) == 0) return CMD_TEARDOWN;
    if (strncmp(token, "ipconfig", sizeof(token)) == 0) return CMD_IPCONFIG;
    if (strncmp(token, "q_add", sizeof(token)) == 0) return CMD_QADD;
    if (strncmp(token, "apply", sizeof(token)) == 0) return CMD_APPLY;
    if (strncmp(token, "key", sizeof(token)) == 0) return CMD_KEY;
    if (strncmp(token, "clear-config", sizeof(token)) == 0) return CMD_CLEAR_CONFIG;
    if (strncmp(token, "clear", sizeof(token)) == 0) return CMD_CLEAR;
    if (strncmp(token, "list", sizeof(token)) == 0) return CMD_LIST;
    if (strncmp(token, "stop_thread", sizeof(token)) == 0) return CMD_STOP_THREAD;
    return CMD_UNKNOWN;
}

void trim_newline(char* str) {
    size_t len = strlen(str);
    while (len > 0 && (str[len - 1] == '\r' || str[len - 1] == '\n')) {
        str[--len] = '\0';
    }
}

void print_prompt() {
    cli_printf("%s> ", ID);
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
    
    cli_printf("\n");
    if (arg) trim_newline(arg);
    
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
        case CMD_TIME:
            cmd_time();
            break;
        case CMD_TEARDOWN:
            cmd_teardown();
            break;
        case CMD_IPCONFIG:
            cmd_ipconfig();
            break;
        case CMD_QADD:
            cmd_add_queue();
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
        case CMD_STOP_THREAD:
            cmd_stop_thread(arg);
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

/**
 * @brief The following function prints the MOTD.
 */
void print_motd() 
{
    cli_printf("Welcome to the DataFarm CLI!\n\n");
}
