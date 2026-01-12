#pragma once
#include <stdbool.h>

/**
 * @class OTAUpdater
 * @brief Handles performing OTA (Over-The-Air) firmware updates.
 *
 * This class provides a minimal API for fetching a firmware binary from a
 * remote HTTP server and initiating the ESP-IDF OTA update process.
 * 
 * The updater downloads the firmware from the provided URL, writes it into an
 * OTA partition, verifies the image, and then marks it as the next bootable
 * image. The caller typically triggers a reboot after a successful update.
 *
 * Example:
 * @code
 *     OTAUpdater ota;
 *     if (ota.update("http://192.168.1.10/cn1.bin")) {
 *         ESP_LOGI("OTA", "Update OK, rebooting...");
 *         esp_restart();
 *     }
 * @endcode
 */
class OTAUpdater {
public:
    /**
     * @brief Perform an OTA firmware update from the specified URL.
     *
     * Downloads a firmware binary via HTTP and writes it to the selected OTA
     * partition. This function manages the entire update flow including:
     *  - HTTP fetch of the image
     *  - Writing to OTA partition
     *  - Validation and completion of the update
     *
     * On success, the firmware is ready to boot on the next restart.
     *
     * @param url HTTP URL pointing to a binary firmware file.
     *            Example: "http://192.168.1.10/cn1.bin"
     *
     * @return true  The update succeeded and the image is ready to boot.  
     * @return false The update failed (network error, validation failure, etc.)
     */
    bool update(const char* url);

    /**
     * @brief Print OTA-related system and partition information.
     *
     * Displays details such as:
     *  - Current running partition
     *  - Next boot partition
     *  - OTA partition table layout
     *
     * Useful for diagnostics, debugging OTA flows, and confirming partition
     * selection behavior.
     */
    static void printInfo();
};
