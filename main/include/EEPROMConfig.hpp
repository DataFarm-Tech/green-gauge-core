#pragma once

#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "Node.hpp"

// Define the device configuration structure
typedef struct {
    bool has_activated;
    Node nodeId;    
} DeviceConfig;

/**
 * @brief Handles saving, loading, and erasing configuration data
 *        from the ESP32 NVS (EEPROM-like) storage.
 */
class EEPROMConfig {
private:
    nvs_handle_t handle;
    const char* TAG = "EEPROMConfig";
    const char* NAMESPACE = "config_ns";
    const char* KEY = "device_cfg";

    void init_nvs();

public:
    // Inline constructor
    EEPROMConfig() : handle(0) {}

    bool begin();
    void close();
    
    bool saveConfig(const DeviceConfig& config);
    bool loadConfig(DeviceConfig& config);
    bool eraseConfig();
};

// Global config variable (declared extern here, defined in main.cpp)
extern DeviceConfig g_device_config;
