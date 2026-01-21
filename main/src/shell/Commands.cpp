#include "CLI.hpp"
#include "UARTDriver.hpp"
#include "OTAUpdater.hpp"
#include "EEPROMConfig.hpp"
#include "Logger.hpp"
#include "esp_system.h"
#include "esp_app_desc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>


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
 * @brief Command handler for 'provision hwver' subcommand.
 * Sets the hardware version/ID.
 */
static void cmd_provision_hwver(int, char**);

/**
 * @brief Command handler for 'provision fwver' subcommand.
 * Sets the firmware version.
 */
static void cmd_provision_fwver(int, char**);


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
 * @brief Command handler for 'provision' command.
 * Manages device provisioning (hardware version, firmware version).
 */
static void cmd_provision(int, char**);

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
    {"log",     "Show system log",         cmd_log,     0},
    {"history", "Show command history",    cmd_history, 0},
    {"version", "Show firmware version",   cmd_version, 0},
    {"provision", "provision <subcommand>", cmd_provision, 1},
    {nullptr, nullptr, nullptr, 0}
};

/**
 * @brief EEPROM configuration subcommands.
 * Defines the 'clean' and 'get' subcommands for the 'eeprom' command.
 */
static const Command eeprom_subcommands[] = {
    {"clean", "Erase EEPROM configuration", cmd_eeprom_clean, 0},
    {"get",   "Get EEPROM configuration",   cmd_eeprom_get,   0},
    {nullptr, nullptr, nullptr, 0}
};

/**
 * @brief Provisioning subcommands.
 * Defines the 'hwver' and 'fwver' subcommands for the 'provision' command.
 */
static const Command provision_subcommands[] = {
    {"hwver", "hwver <hardware_version> - Set hardware version", cmd_provision_hwver, 1},
    {"fwver", "fwver - Set firmware version", cmd_provision_fwver, 0},
    {nullptr, nullptr, nullptr, 0}
};

static void cmd_help(int, char**) {
    UARTDriver* console = CLI::getConsole();
    if (!console) return;

    console->write("Commands:\r\n");
    for (int i = 0; commands[i].name; i++) {
        console->writef("  %-10s %s\r\n", commands[i].name, commands[i].help);
    }
}

static void cmd_reset(int, char**) {
    UARTDriver* console = CLI::getConsole();
    if (!console) return;

    console->write("Rebooting...\r\n");
    vTaskDelay(pdMS_TO_TICKS(300));
    esp_restart();
}

static void cmd_install(int argc, char** argv) {
    UARTDriver* console = CLI::getConsole();
    if (!console) return;

    char url[256];
    snprintf(url, sizeof(url), "http://%s%s", argv[1], argv[2]);

    console->writef("OTA from %s\r\n", url);

    OTAUpdater ota;
    if (ota.update(url)) {
        console->write("OTA OK. Rebooting...\r\n");
        vTaskDelay(pdMS_TO_TICKS(2000));
        esp_restart();
    } else {
        console->write("OTA FAILED\r\n");
    }
}

static void cmd_eeprom(int argc, char** argv) {
    dispatch_subcommand(eeprom_subcommands, argc, argv, "eeprom <clean|get>");
}

static void cmd_eeprom_clean(int, char**) {
    UARTDriver* console = CLI::getConsole();
    if (!console) return;

    eeprom.begin();
    eeprom.eraseConfig();
    console->write("EEPROM configuration erased\r\n");
}

static void cmd_eeprom_get(int, char**) {
    UARTDriver* console = CLI::getConsole();
    if (!console) return;

    DeviceConfig config;
    eeprom.begin();
    if (eeprom.loadConfig(config)) {
        console->writef("Activated: %s\r\n",
            config.has_activated ? "Yes" : "No"
        );
        console->writef("Node ID: %s\r\n", config.manf_info.nodeId);
        console->writef("Hardware Version: %s\r\n", config.manf_info.hw_ver);
        console->writef("Firmware Version: %s\r\n", config.manf_info.fw_ver);

        console->writef("Calibration\n");

        for (size_t i = 0; i < 5; i++) {
            
            const char* type_name = "UNKNOWN";
            
            for (const auto& entry : MEASUREMENT_TABLE) {
                if (entry.type == config.calib.calib_list[i].m_type) {
                    type_name = entry.name;
                    break;
                }
            }
            
            console->writef("Sensor %d (%s):\r\n", (int)i, type_name);
            console->writef("  Offset: %.4f\r\n", static_cast<double>(config.calib.calib_list[i].offset));
            console->writef("  Gain:   %.4f\r\n", static_cast<double>(config.calib.calib_list[i].gain));
        }
        

    } else {
        console->write("No EEPROM configuration found\r\n");
    }
}

static void cmd_log(int, char**) {
    UARTDriver* console = CLI::getConsole();
    if (!console) return;

    std::string contents;
    if (g_logger.read("system.log", contents) == ESP_OK) {
        // Simple find-replace: \n -> \r\n
        for (size_t i = 0; i < contents.length(); i++) {
            if (contents[i] == '\n' && (i == 0 || contents[i-1] != '\r')) {
                contents.insert(i, 1, '\r');
                i++; // Skip the inserted \r
            }
        }
        console->write(contents.c_str());
        console->write("\r\n");
    } else {
        console->write("No log file\r\n");
    }
}

static void cmd_history(int, char**) {
    UARTDriver* console = CLI::getConsole();
    if (!console) return;

    console->write("History shown inline\r\n");
}

static void cmd_version(int, char**) {
    UARTDriver* console = CLI::getConsole();
    if (!console) return;

    const esp_app_desc_t* a = esp_app_get_description();
    console->writef(
        "Project: %s\r\nVersion: %s\r\nBuilt: %s %s\r\nIDF: %s\r\n",
        a->project_name, a->version, a->date, a->time, a->idf_ver
    );
}

static void cmd_provision(int argc, char** argv) {
    dispatch_subcommand(provision_subcommands, argc, argv, "provision <hwver|fwver> <value>");
}

static void cmd_provision_hwver(int argc, char** argv) {
    UARTDriver* console = CLI::getConsole();
    if (!console) return;

    const char * hw_ver = argv[1];

    console->writef("Setting hardware version to: %s\r\n", hw_ver);

    strncpy(g_device_config.manf_info.hw_ver, hw_ver, sizeof(g_device_config.manf_info.hw_ver)-1);

    eeprom.saveConfig(g_device_config);
}

static void cmd_provision_fwver(int argc, char** argv) {
    UARTDriver* console = CLI::getConsole();
    if (!console) return;
    
    const esp_app_desc_t* a = esp_app_get_description();

    console->writef("Setting firmware version to: %s\r\n", a->version);

    strncpy(g_device_config.manf_info.fw_ver, a->version, sizeof(g_device_config.manf_info.fw_ver)-1);
    eeprom.saveConfig(g_device_config);
}