#pragma once

#include <cstdint>
#include <memory>

#include "Communication.hpp"
#include "DeviceConfig.hpp"
#include "EEPROMConfig.hpp"
#include "GPS.hpp"

extern "C" {
#include "esp_system.h"
}

extern uint32_t wakeup_causes;
extern esp_reset_reason_t reset_reason;
extern std::unique_ptr<Communication> g_comm;
extern ConnectionType g_hw_var;
extern GPS m_gps;

/**
 * @brief Enters deep sleep mode for a specified duration in seconds.
 * If the duration is 0, it will sleep indefinitely until an external wakeup event occurs
 */
void enter_deep_sleep();

/**
 * @brief Initializes hardware peripherals and resources based on the selected connection type.
 * This includes setting up UART interfaces, network stacks, and any other necessary hardware features.
 * The specific initialization steps are determined by the global g_hw_var, which is set based on the device's hardware version.
 * For example, if g_hw_var is ConnectionType::WIFI, it will initialize the Wi-Fi network interface and event loop.
 * If g_hw_var is ConnectionType::SIM, it will initialize the UART interface for the cellular modem.
 * This function must be called before attempting to use any communication features or peripherals.
 */
void init_hw();

/**
 * @brief Loads the device configuration from NVS or creates a default configuration if none exists.
 * This function attempts to read the DeviceConfig structure from NVS using the EEPROMConfig class.
 * If a valid configuration is found, it populates the global g_device_config variable and returns
 * true. If no valid configuration is found, it initializes g_device_config with default values,
 * attempts to save it to NVS, and returns true if the save was successful. If
 */
bool load_create_config();

/**
 * @brief Determines the hardware features and connection type based on the device's hardware version.
 * This function reads the hardware version from the global g_device_config.manf_info.hw_ver.value
 * and matches it against known hardware versions to set the global g_hw_var accordingly.
 * It then initializes the global g_comm unique pointer with a new Communication instance
 * configured for the selected connection type. This function must be called before init_hw() to ensure
 * that the correct hardware features and communication interfaces are initialized.
 */
void hw_features();

/**
 * @brief Main application task that handles provisioning, GPS reading, packet sending, and other operational logic.
 * This function is designed to be run as a FreeRTOS task and will execute the main
 * application logic, including connecting to the network, checking for OTA updates, reading GPS coordinates,
 * handling device activation, sending updates, and managing the device's operational flow.
 * The function will attempt to connect to the network and, if successful, will proceed with the main application logic.
 * If the network connection fails at any point, it will enter deep sleep mode to conserve power and retry on the next wakeup.
 * The function also includes logic to handle OTA updates if enabled, and will reboot into the updated firmware if an update is successfully applied.
 * After completing its operations, the function will enter deep sleep mode to wait for the next cycle, and will delete itself from the FreeRTOS scheduler before sleeping.
 * @param arg Unused parameter for FreeRTOS task compatibility.
 */
void start_app(void* arg);
