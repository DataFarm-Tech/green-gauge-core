#include "CLI.hpp"
#include "UARTDriver.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <string.h>

#define MAX_ARGS     6
#define BUF_SIZE     128
#define HISTORY_SIZE 8

// Static console pointer
UARTDriver* CLI::s_console = nullptr;

static char history[HISTORY_SIZE][BUF_SIZE];
static int history_count = 0;
static int history_index = -1;

// ───────────────────────── PUBLIC INTERFACE ─────────────────────────

UARTDriver* CLI::getConsole() {
    return s_console;
}

// ───────────────────────── PARSER ─────────────────────────

static int parse(char* line, char** argv) {
    int argc = 0;
    char* p = strtok(line, " ");
    while (p && argc < MAX_ARGS) {
        argv[argc++] = p;
        p = strtok(nullptr, " ");
    }
    return argc;
}

// ───────────────────────── SUBCOMMAND DISPATCHER ─────────────────────────

void dispatch_subcommand(
    const Command* table,
    int argc,
    char** argv,
    const char* usage
) {
    UARTDriver* console = CLI::getConsole();
    if (!console) return;

    if (argc < 2) {
        console->writef("Usage: %s\r\n", usage);
        return;
    }

    for (int i = 0; table[i].name; i++) {
        if (strcmp(argv[1], table[i].name) == 0) {
            if (argc - 2 < table[i].min_args) {
                console->writef("Usage: %s\r\n", table[i].help);
                return;
            }
            table[i].handler(argc - 1, &argv[1]);
            return;
        }
    }

    console->write("Unknown subcommand\r\n");
}

// ───────────────────────── EXEC ─────────────────────────

static void exec(char* line) {
    UARTDriver* console = CLI::getConsole();
    if (!console) return;

    char* argv[MAX_ARGS];
    int argc = parse(line, argv);
    if (argc == 0) return;

    for (int i = 0; commands[i].name; i++) {
        if (strcmp(argv[0], commands[i].name) == 0) {
            if (argc - 1 < commands[i].min_args) {
                console->writef("Usage: %s\r\n", commands[i].help);
                return;
            }
            commands[i].handler(argc, argv);
            return;
        }
    }

    console->write("Unknown command\r\n");
}

// ───────────────────────── CLI TASK ─────────────────────────

extern "C" void cli_task(void* param) {
    UARTDriver* console = static_cast<UARTDriver*>(param);
    if (!console) return;

    char line[BUF_SIZE] = {};
    int idx = 0;
    bool esc = false;
    int esc_state = 0;

    console->write("CLI Ready\r\n> ");

    while (true) {
        uint8_t c;
        if (console->readByte(c) <= 0) continue;

        if (c == 0x1B) { esc = true; esc_state = 1; continue; }
        if (esc) {
            if (esc_state == 1 && c == '[') { esc_state = 2; continue; }
            if (esc_state == 2) {
                if (c == 'A' && history_count && history_index < history_count - 1)
                    history_index++;
                else if (c == 'B' && history_index >= 0)
                    history_index--;

                while (idx) console->write("\b \b"), idx--;

                if (history_index >= 0) {
                    strcpy(line, history[history_count - 1 - history_index]);
                    idx = strlen(line);
                    console->write(line);
                }

                esc = false;
                continue;
            }
        }

        if (c == '\r' || c == '\n') {
            console->write("\r\n");
            line[idx] = 0;

            if (idx) {
                if (history_count < HISTORY_SIZE)
                    strcpy(history[history_count++], line);
                else {
                    for (int i = 1; i < HISTORY_SIZE; i++)
                        strcpy(history[i - 1], history[i]);
                    strcpy(history[HISTORY_SIZE - 1], line);
                }
            }

            history_index = -1;
            exec(line);
            idx = 0;
            console->write("> ");
            continue;
        }

        if ((c == 0x08 || c == 0x7F) && idx) {
            idx--;
            console->write("\b \b");
            continue;
        }

        if (c >= 32 && c < 127 && idx < BUF_SIZE - 1) {
            line[idx++] = c;
            // Write character using console instance
            char tmp[2] = {(char)c, 0};
            console->write(tmp);
        }
    }
}

void CLI::start(UARTDriver& console) {
    s_console = &console;
    xTaskCreate(cli_task, "cli_task", 4096, s_console, 1, nullptr);
}