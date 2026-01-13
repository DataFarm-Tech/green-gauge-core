#pragma once

#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "Node.hpp"

// Define the device configuration structure
/**
 * @struct DeviceConfig
 * @brief  Represents stored configuration for this device.
 *
 * This structure contains persistent fields stored in NVS, including
 * activation state and the device's assigned Node ID.
 */
typedef struct {
    bool has_activated;   ///< True if the device has already completed activation
    Node nodeId;          ///< Unique Node identifier for the device
    char hw_ver[32];     ///< Hardware version string (e.g., "0.0.1"
    char fw_ver[32];     ///< Firmware version string (e.g., "0.0.1"
} DeviceConfig;

/**
 * @brief Provides high-level APIs for saving, loading, and erasing
 *        configuration data in ESP32 NVS (EEPROM-like flash storage).
 *
 * This class wraps the ESP-IDF NVS functions, offering a convenient and
 * type-safe interface for storing the `DeviceConfig` structure.  
 *
 * Usage example:
 * @code
 *     EEPROMConfig cfg;
 *     cfg.begin();
 *     cfg.saveConfig(g_device_config);
 * @endcode
 */
class EEPROMConfig {
private:
    nvs_handle_t handle;          ///< Internal NVS handle used after opening
    const char* TAG = "EEPROMConfig"; ///< Logging tag
    const char* NAMESPACE = "config_ns"; ///< NVS namespace name
    const char* KEY = "device_cfg"; ///< Key under which DeviceConfig is stored

    /**
     * @brief Initialize the NVS flash subsystem (nvs_flash_init / erase if needed).
     *
     * This is called internally from `begin()` and ensures that the NVS partition
     * is ready for use. Handles recovery from truncated or inconsistent NVS states.
     */
    void init_nvs();

public:
    /**
     * @brief Construct an uninitialized EEPROMConfig object.
     *
     * An explicit call to `begin()` is required before calling any other method.
     */
    EEPROMConfig() : handle(0) {}

    /**
     * @brief Initialize the NVS namespace and open storage handle.
     *
     * Ensures the NVS system is initialized and opens the class-specific namespace.
     *
     * @return true  Initialization successful  
     * @return false Failed to open or initialize NVS
     */
    bool begin();

    /**
     * @brief Close the opened NVS handle.
     *
     * Safe to call even if `begin()` failed or no handle is open.
     */
    void close();

    /**
     * @brief Save a `DeviceConfig` instance to flash.
     *
     * The structure is serialized into a binary blob and written to NVS.
     * A commit operation is performed before returning.
     *
     * @param config Reference to the struct to store.
     * @return true  Save successful  
     * @return false Write or commit failed
     */
    bool saveConfig(const DeviceConfig& config);

    /**
     * @brief Load a `DeviceConfig` instance from flash.
     *
     * Attempts to read the stored binary blob and reconstruct the original
     * structure. If the key does not exist, the function returns false.
     *
     * @param config Reference where the loaded data is stored.
     * @return true  Successfully loaded  
     * @return false No data found or invalid length
     */
    bool loadConfig(DeviceConfig& config);

    /**
     * @brief Erase the saved configuration from NVS.
     *
     * Removes the key from the namespace and commits the change.
     *
     * @return true  Erased successfully  
     * @return false Key does not exist or erase failed
     */
    bool eraseConfig();
};

// Global config variable (declared extern here, defined in main.cpp)
extern DeviceConfig g_device_config;

extern EEPROMConfig eeprom;
