#pragma once

#include <string>
#include "esp_err.h"

/**
 * @class Logger
 * @brief Handles appending text to files on a LittleFS filesystem.
 *
 * The Logger class provides methods to mount LittleFS, append text
 * lines to files, and unmount the filesystem.
 */
class Logger {
public:
    /**
     * @brief Construct a new Logger object.
     * @param basePath Base path where LittleFS is mounted. Default is "/rootfs".
     */
    Logger(const char* basePath = "/rootfs");

    /**
     * @brief Mount the LittleFS filesystem.
     *
     * This must be called before any file operations.
     *
     * @return ESP_OK if mounted successfully.
     * @return esp_err_t Error code if mounting fails.
     */
    esp_err_t open();

    /**
     * @brief Append a line of text to a specific file.
     *
     * Creates the file if it does not exist. A newline is automatically added.
     *
     * @param filename Name of the file to append to.
     * @param text Text to append.
     * @return ESP_OK if append was successful.
     * @return esp_err_t Error code if appending fails.
     */
    esp_err_t log(const char* filename, const std::string& text);

    /**
     * @brief Unmount the LittleFS filesystem.
     */
    void close();

private:
    const char* basePath; /**< Base path where LittleFS is mounted */
    bool mounted;         /**< True if filesystem is mounted */
};
