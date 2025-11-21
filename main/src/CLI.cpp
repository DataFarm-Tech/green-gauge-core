#include "CLI.hpp"
#include "UARTConsole.hpp"
#include "ota/OTAUpdater.hpp"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

#define MAX_ARGS 4
#define BUF_SIZE 128

typedef void (*cmd_handler_t)(int argc, char** argv);

struct Command {
    const char* name;
    const char* help;
    cmd_handler_t handler;
    int min_args;
};


#define HISTORY_SIZE 8
static char history[HISTORY_SIZE][BUF_SIZE];
static int history_count = 0;
static int history_index = -1;



// ───────────────────────── COMMAND HANDLERS ─────────────────────────

static void cmd_help(int, char**);
static void cmd_reset(int, char**);
static void cmd_install(int argc, char** argv);

static const Command commands[] = {
    {"help",      "Show commands",           cmd_help,      0},
    {"reset",     "Reboot",                  cmd_reset,     0},
    {"install",   "install <ip> <file>",     cmd_install,   2},
    {nullptr, nullptr, nullptr, 0}
};

// ───────────────────────── IMPLEMENTATIONS ─────────────────────────

static void cmd_help(int, char**) {
    UARTConsole::write("Commands:\r\n");
    for (int i = 0; commands[i].name; i++)
        UARTConsole::writef("  %-12s %s\r\n",
                            commands[i].name, commands[i].help);
}

static void cmd_reset(int, char**) {
    UARTConsole::write("Rebooting...\r\n");
    vTaskDelay(pdMS_TO_TICKS(300));
    esp_restart();
}

static void cmd_install(int argc, char** argv) {
    char url[256];
    snprintf(url, sizeof(url), "http://%s%s", argv[1], argv[2]);

    UARTConsole::writef("OTA from %s\r\n", url);

    OTAUpdater ota;

    if (ota.update(url)) {
        UARTConsole::write("OTA OK. Rebooting...\r\n");
        vTaskDelay(pdMS_TO_TICKS(2000));
        esp_restart();
    } else {
        UARTConsole::write("OTA FAILED\r\n");
    }
}

// ───────────────────────── PARSER + LOOP ─────────────────────────

static int parse(char* line, char** argv) {
    int argc = 0;
    char* p = strtok(line, " ");

    while (p && argc < MAX_ARGS) {
        argv[argc++] = p;
        p = strtok(nullptr, " ");
    }
    return argc;
}

static void exec(char* line) {
    char* argv[MAX_ARGS];
    int argc = parse(line, argv);

    if (argc == 0) return;

    for (int i = 0; commands[i].name; i++) {
        if (strcmp(argv[0], commands[i].name) == 0) {
            if (argc - 1 < commands[i].min_args) {
                UARTConsole::write("Not enough args\r\n");
                return;
            }
            commands[i].handler(argc, argv);
            return;
        }
    }
    UARTConsole::write("Unknown command\r\n");
}

extern "C" void cli_task(void*) {
    char line[BUF_SIZE];
    int idx = 0;
    bool esc_seq = false;
    int esc_state = 0;

    UARTConsole::write("CLI Ready\r\n> ");

    while (true) {
        uint8_t c;
        if (UARTConsole::readByte(c) > 0) {

            // ───────────────── ESCAPE SEQUENCES (arrow keys) ─────────────────
            if (c == 0x1B) {  // ESC
                esc_seq = true;
                esc_state = 1;
                continue;
            }

            if (esc_seq) {
                if (esc_state == 1 && c == '[') {
                    esc_state = 2;
                    continue;
                }
                if (esc_state == 2) {
                    // UP arrow
                    if (c == 'A') {
                        if (history_count > 0 && history_index < history_count - 1) {
                            history_index++;

                            // Clear current line
                            while (idx > 0) {
                                UARTConsole::write("\b \b");
                                idx--;
                            }

                            // Load history entry
                            strcpy(line, history[history_count - 1 - history_index]);
                            idx = strlen(line);
                            UARTConsole::write(line);
                        }
                    }

                    // DOWN arrow
                    else if (c == 'B') {
                        if (history_index > 0) {
                            history_index--;

                            // Clear current line
                            while (idx > 0) {
                                UARTConsole::write("\b \b");
                                idx--;
                            }

                            // Load the next newer entry
                            strcpy(line, history[history_count - 1 - history_index]);
                            idx = strlen(line);
                            UARTConsole::write(line);
                        }
                        else if (history_index == 0) {
                            // Clear history selection — blank line
                            history_index = -1;
                            while (idx > 0) {
                                UARTConsole::write("\b \b");
                                idx--;
                            }
                        }
                    }

                    esc_seq = false;
                    continue;
                }
            }

            // ───────────────── ENTER / EXECUTE ─────────────────
            if (c == '\n' || c == '\r') {
                UARTConsole::write("\r\n");
                line[idx] = 0;

                if (idx > 0) {
                    // Save to history
                    if (history_count < HISTORY_SIZE) {
                        strcpy(history[history_count++], line);
                    } else {
                        // Shift left
                        for (int i = 1; i < HISTORY_SIZE; i++)
                            strcpy(history[i - 1], history[i]);
                        strcpy(history[HISTORY_SIZE - 1], line);
                    }
                }

                history_index = -1;

                exec(line);
                idx = 0;

                UARTConsole::write("> ");
                continue;
            }

            // ───────────────── BACKSPACE ─────────────────
            if ((c == 0x08 || c == 0x7F)) {
                if (idx > 0) {
                    idx--;
                    UARTConsole::write("\b \b");
                }
                continue;
            }

            // ───────────────── PRINTABLE ─────────────────
            if (c >= 32 && c < 127 && idx < BUF_SIZE - 1) {
                line[idx++] = c;
                uart_write_bytes(UART_NUM_0, (char*)&c, 1);
                continue;
            }
        }
    }
}



void CLI::start() {
    xTaskCreate(cli_task, "cli_task", 4096, NULL, 1, NULL);
}
