#include <Arduino.h>
#include <mutex>

#include "cli_th.h"
#include "config.h"
#include "th_handler.h"
#include "utils.h"
#include "msg_queue.h"
#include "cmd/cmd.h"

char cli_buffer[BUFFER_SIZE];
uint8_t cli_pos = 0;

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

    // Get the second argument (e.g., for "ping google.com", this will be "google.com")
    char* arg = strtok(nullptr, " ");
    if (arg) trim_newline(arg);

    if (strcmp(token, "help") == 0) return cmd_help();
    if (strcmp(token, "exit") == 0) return cmd_exit();
    if (strcmp(token, "reboot") == 0) return cmd_reboot();
    if (strcmp(token, "queue") == 0) return cmd_queue();
    if (strcmp(token, "ping") == 0) return cmd_ping(arg);

    cli_printf("Unknown command: %s\n", token);
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
