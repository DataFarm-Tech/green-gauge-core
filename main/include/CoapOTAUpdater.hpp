#pragma once

#include <string>
#ifdef __cplusplus
extern "C" {
#endif
#include "esp_http_client.h"
#include "esp_ota_ops.h"
#ifdef __cplusplus
}
#endif
#include "Communication.hpp"

class CoapOTAUpdater {
public:
    /**
     * @brief Constructs a CoapOTAUpdater with the given Communication instance and current firmware version.
     * @param communication Reference to the Communication instance used for network operations.
     * @param current_firmware_version C-string representing the current firmware version of the device.
     * If current_firmware_version is nullptr, it will be treated as an empty string.
     * The Communication instance is used to send CoAP requests to check for firmware updates and to
     */
    CoapOTAUpdater(Communication& communication, const char* current_firmware_version);

    /**
     * @brief Checks if a new firmware version is available on the server.
     * Sends a CoAP request to the firmware version endpoint and compares it with the current version.
     * @return true if a newer firmware version is available, false otherwise.
     * The available version string is stored internally for use during the update process if this returns true.
     * Note: This function does not perform any OTA update actions, it only checks for the availability of an update.
     */
    bool isFirmwareAvailable();
    
    /**
     * @brief Executes the OTA update process.
     * This function should only be called if isFirmwareAvailable() returns true.
     * It sends a CoAP request to download the firmware binary and writes it to the OTA
     * partition. After successful completion, it returns true, indicating that the device should reboot to apply the update.
     * @return true if the OTA update was successfully downloaded and written, false otherwise.
     * Note: This function does not perform the reboot itself, it only handles the download and
     * writing of the firmware to the OTA partition.
     **/
    bool executeUpdate();

private:
    bool streamFirmwareFromHttpsToOta(const std::string& firmware_url, esp_ota_handle_t ota_handle, size_t& total_written);

    Communication& comm;
    std::string current_version;
    std::string available_version;
    esp_http_client_config_t http_config;
};
