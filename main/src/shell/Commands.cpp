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
 * @brief Command handler for 'provision hwver' subcommand.
 * Sets the hardware version/ID.
 */
static void cmd_provision_hwver(int, char**);

/**
 * @brief Command handler for 'provision fwver' subcommand.
 * Sets the firmware version.
 */
static void cmd_provision_nodeid(int, char**);


/**
 * @brief Command handler for 'provision secretkey' subcommand.
 * Sets the secretkey.
 */
static void cmd_provision_secretkey(int, char**);

/**
 * @brief Command handler for 'manf-set p_code' subcommand.
 * Sets the p_code.
 */
static void cmd_provision_p_code(int, char**);

/**
 * @brief TODO
 */
static void cmd_provision(int, char**);

static void cmd_provision_hwvar(int argc, char ** argv);

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
    {"manf-set", "manf-set <hwver|nodeId|secretkey|p_code|hw_var> <value>", cmd_provision, 1},
    {nullptr, nullptr, nullptr, 0}
};

/**
 * @brief Provisioning subcommands.
 * Defines the 'hwver' and 'fwver' subcommands for the 'provision' command.
 */
static const Command provision_subcommands[] = {
    {"hwver",   "hwver <hardware_version>", cmd_provision_hwver, 1},
    {"nodeId",  "nodeId <node_id>",         cmd_provision_nodeid, 1},
    {"secretkey", "secretkey <secretkey>", cmd_provision_secretkey, 1},
    {"p_code", "p_code <p_code>", cmd_provision_p_code, 1},
    {"hw_var", "hw_var <hw_var>", cmd_provision_hwvar, 1},
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
        console->writef("Node ID: %s\r\n", config.manf_info.nodeId.value);
        console->writef("Hardware Version: %s\r\n", config.manf_info.hw_ver.value);
        console->writef("Firmware Version: %s\r\n", config.manf_info.fw_ver.value);
        console->writef("Secret Key: %s\n", config.manf_info.secretkey.value);
        console->writef("Hardware Variant: %s\r\n", config.manf_info.hw_var.value);
        console->writef("Product Code: %s\r\n", config.manf_info.p_code.value);

        console->writef("Calibration\n");
        for (size_t i = 0; i < 6; i++) {
            const char* type_name = "UNKNOWN";
            for (const auto& entry : NPK::MEASUREMENT_TABLE) {
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
    dispatch_subcommand(provision_subcommands, argc, argv, "manf-set <hwver|nodeId|secretkey> <value>");
}

static void cmd_provision_hwver(int argc, char** argv) {
    UARTDriver* console = CLI::getConsole();
    if (!console) return;

    const char * hw_ver = argv[1];
    console->writef("Setting hardware version to: %s\r\n", hw_ver);

    strncpy(g_device_config.manf_info.hw_ver.value, hw_ver,
            sizeof(g_device_config.manf_info.hw_ver.value) - 1);
    // g_device_config.manf_info.hw_ver.has_provision = true;

    eeprom.saveConfig(g_device_config);
}

static void cmd_provision_nodeid(int argc, char** argv) {
    UARTDriver* console = CLI::getConsole();
    if (!console) return;

    const char * nodeId = argv[1];
    console->writef("Setting nodeId to: %s\r\n", nodeId);

    strncpy(g_device_config.manf_info.nodeId.value, nodeId,
            sizeof(g_device_config.manf_info.nodeId.value) - 1);
    // g_device_config.manf_info.nodeId.has_provision = true;

    eeprom.saveConfig(g_device_config);
}

static void cmd_provision_secretkey(int argc, char** argv) {
    UARTDriver* console = CLI::getConsole();
    if (!console) return;

    const char * secretkey = argv[1];
    console->writef("Setting secretkey to: %s\r\n", secretkey);
    strncpy(g_device_config.manf_info.secretkey.value, secretkey,
        sizeof(g_device_config.manf_info.secretkey.value) - 1);
    // g_device_config.manf_info.secretkey.has_provision = true;

    eeprom.saveConfig(g_device_config);
}

static void cmd_provision_p_code(int argc, char** argv) {
    UARTDriver* console = CLI::getConsole();
    if (!console) return;

    const char * p_code = argv[1];
    console->writef("Setting pcode to: %s\r\n", p_code);
    
    strncpy(g_device_config.manf_info.p_code.value, p_code,
        sizeof(g_device_config.manf_info.p_code.value) - 1);
    // g_device_config.manf_info.p_code.has_provision = true;

    eeprom.saveConfig(g_device_config);
}

static void cmd_provision_hwvar(int argc, char ** argv) {
    UARTDriver* console = CLI::getConsole();
    if (!console) return;

    const char * hwvar = argv[1];
    console->writef("Setting hwvar to: %s\r\n", hwvar);
    
    strncpy(g_device_config.manf_info.hw_var.value, hwvar,
        sizeof(g_device_config.manf_info.hw_var.value) - 1);
    // g_device_config.manf_info.hw_var.has_provision = true;

    eeprom.saveConfig(g_device_config);
}


static void cmd_ping(int argc, char** argv) {
    printf("NOT SUPPORTED\n");
}