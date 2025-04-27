#include <Arduino.h>
#include <mutex>

#include "cli_th.h"
#include "config.h"
#include "th_handler.h"
#include "utils.h"
#include "msg_queue.h"
#include "cmd/cmd.h"


// 1. Define the enum
typedef enum {
    CMD_HELP,
    CMD_EXIT,
    CMD_REBOOT,
    CMD_QUEUE,
    CMD_PING,
    CMD_CLEAR,
    CMD_THREADS,
    CMD_UNKNOWN
} cli_cmd;





char cli_buffer[BUFFER_SIZE];
uint8_t cli_pos = 0;

cli_cmd parse_command(const char* token) 
{
    if (strncmp(token, "help", sizeof(token)) == 0) return CMD_HELP;
    if (strncmp(token, "exit", sizeof(token)) == 0) return CMD_EXIT;
    if (strncmp(token, "reboot", sizeof(token)) == 0) return CMD_REBOOT;
    if (strncmp(token, "queue", sizeof(token)) == 0) return CMD_QUEUE;
    if (strncmp(token, "ping", sizeof(token)) == 0) return CMD_PING;
    if (strncmp(token, "threads", sizeof(token)) == 0) return CMD_THREADS;
    if (strncmp(token, "clear", sizeof(token)) == 0) return CMD_CLEAR;
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

void handle_cmd(const char* cmd) {
    cli_printf("\n");

    char cmd_copy[BUFFER_SIZE];
    strncpy(cmd_copy, cmd, BUFFER_SIZE);
    cmd_copy[BUFFER_SIZE - 1] = '\0';

    char* token = strtok(cmd_copy, " ");
    if (token == nullptr) return;

    trim_newline(token);

    cli_cmd cmd_input = parse_command(token);

    // Get the second argument (e.g., for "ping google.com", this will be "google.com")
    char* arg = strtok(nullptr, " ");
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
            if (arg == NULL) {
                cli_printf("Error: ping requires a host argument.\n");
            } else {
                cmd_ping(arg);
            }
            break;
        case CMD_CLEAR:
            cmd_clear();
            break;
        case CMD_THREADS:
            cmd_threads();
            break;
            
        case CMD_UNKNOWN:
        default:
            cli_printf("Unknown command: %s\n", token);
            break;
    }
}



void read_serial_cli(void* param) {
    print_prompt();

    while (true) {
        bool processedCommand = false;

        while (Serial.available()) {
            char c = Serial.read();

            if (c == '\b' || c == 127) {
                if (cli_pos > 0) {
                    cli_pos--;
                    cli_buffer[cli_pos] = '\0';
                    cli_print("\b \b");
                }
                continue;
            }

            if (c == '\r' || c == '\n') {
                cli_buffer[cli_pos] = '\0';
                if (cli_pos > 0) {
                    handle_cmd(cli_buffer);
                    cli_pos = 0;
                    processedCommand = true;
                }
                continue;
            }

            if (cli_pos < BUFFER_SIZE - 1 && isprint(c)) {
                cli_buffer[cli_pos++] = c;
                cli_print(c);
            }
        }

        if (processedCommand) {
            print_prompt();
            processedCommand = false;
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void print_motd() {
    cli_printf("Welcome to the DataFarm CLI!\n\n");
}
