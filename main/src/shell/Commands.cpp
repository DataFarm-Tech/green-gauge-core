#include "CLI.hpp"
#include "UARTConsole.hpp"
#include "OTAUpdater.hpp"
#include "EEPROMConfig.hpp"
#include "Logger.hpp"
#include "esp_system.h"
#include "esp_app_desc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>


/**
 * @brief Dispatches a subcommand from a command table.
 * Searches the provided command table for a matching subcommand
 * and invokes its handler if found.
 */
extern void dispatch_subcommand(
    const Command* table,
    int argc,
    char** argv,
    const char* usage
);

/**
 * @brief Command handler for 'eeprom clean' subcommand.
 * Erases the stored EEPROM configuration.
 */
static void cmd_eeprom_clean(int, char**);

/**
 * @brief Command handler for 'eeprom get' subcommand.
 * Retrieves and displays the stored EEPROM configuration.
 */
static void cmd_eeprom_get(int, char**);



/**
 * @brief EEPRROM configuration subcommands.
 * Defines the 'clean' and 'get' subcommands for the 'eeprom' command.
 */
static const Command eeprom_subcommands[] = {
    {"clean", "Erase EEPROM configuration", cmd_eeprom_clean, 0},
    {"get",   "Get EEPROM configuration",   cmd_eeprom_get,   0},
    {nullptr, nullptr, nullptr, 0}
};



/**
 * @brief Command handler for 'help' command.
 * Displays a list of available commands.
 */
static void cmd_help(int, char**);

/**
 * @brief Command handler for 'reset' command.
 * Reboots the device.
 */
static void cmd_reset(int, char**);

/**
 * @brief Command handler for 'install' command.
 * Performs an OTA update from the specified IP and file.
 */
static void cmd_install(int, char**);

/**
 * @brief Command handler for 'eeprom' command.
 * Manages EEPROM configuration (clean or get).
 */
static void cmd_eeprom(int, char**);

/**
 * @brief Command handler for 'log' command.
 * Displays the system log contents.
 */
static void cmd_log(int, char**);

/**
 * @brief Command handler for 'history' command.
 * Displays the command history.
 */
static void cmd_history(int, char**);

/**
 * @brief Command handler for 'version' command.
 * Displays firmware version information.
 */
static void cmd_version(int, char**);

/**
 * @brief Global command table.
 * Defines all available CLI commands.
 * Each entry includes the command name, help string, handler function, and minimum args.
 * The table is null-terminated.
 */
const Command commands[] = {
    {"help",    "Show commands",           cmd_help,    0},
    {"reset",   "Reboot device",           cmd_reset,   0},
    {"install", "install <ip> <file>",     cmd_install, 2},
    {"eeprom",  "eeprom <clean|get>",      cmd_eeprom,  1},
    {"log",     "Show system log",          cmd_log,     0},
    {"history", "Show command history",    cmd_history, 0},
    {"version", "Show firmware version",   cmd_version, 0},
    {nullptr, nullptr, nullptr, 0}
};

static void cmd_help(int, char**) {
    UARTConsole::write("Commands:\r\n");
    for (int i = 0; commands[i].name; i++) {
        UARTConsole::writef("  %-10s %s\r\n", commands[i].name, commands[i].help);
    }
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

static void cmd_eeprom(int argc, char** argv) {
    dispatch_subcommand(eeprom_subcommands, argc, argv, "eeprom <clean|get>");
}

static void cmd_eeprom_clean(int, char**) {
    eeprom.begin();
    eeprom.eraseConfig();
    UARTConsole::write("EEPROM configuration erased\r\n");
}

static void cmd_eeprom_get(int, char**) {
    DeviceConfig config;
    eeprom.begin();
    if (eeprom.loadConfig(config)) {
        UARTConsole::writef("Activated: %s\r\n",
            config.has_activated ? "Yes" : "No"
        );
        UARTConsole::writef("Node ID: %s\r\n", config.nodeId.getNodeID());

    } else {
        UARTConsole::write("No EEPROM configuration found\r\n");
    }
}

static void cmd_log(int, char**) {
    std::string contents;
    if (g_logger.read("system.log", contents) == ESP_OK) {
        // Simple find-replace: \n -> \r\n
        for (size_t i = 0; i < contents.length(); i++) {
            if (contents[i] == '\n' && (i == 0 || contents[i-1] != '\r')) {
                contents.insert(i, 1, '\r');
                i++; // Skip the inserted \r
            }
        }
        UARTConsole::write(contents.c_str());
        UARTConsole::write("\r\n");
    } else {
        UARTConsole::write("No log file\r\n");
    }
}

static void cmd_history(int, char**) {
    UARTConsole::write("History shown inline\r\n");
}

static void cmd_version(int, char**) {
    const esp_app_desc_t* a = esp_app_get_description();
    UARTConsole::writef(
        "Project: %s\r\nVersion: %s\r\nBuilt: %s %s\r\nIDF: %s\r\n",
        a->project_name, a->version, a->date, a->time, a->idf_ver
    );
}
