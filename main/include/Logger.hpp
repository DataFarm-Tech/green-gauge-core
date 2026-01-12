#pragma once
#include <string>
#include "esp_err.h"

#define MAX_LOG_SIZE    (512 * 1024) // 512 KB
#define MAX_LOG_BACKUPS 3             // Keep 3 rotated logs
#define MIN_FREE_SPACE  (10 * 1024)  // 10 KB buffer

/**
 * @class Logger
 * @brief Handles appending text to files on a LittleFS filesystem with log rotation.
 *
 * The Logger class provides methods to mount LittleFS, append log lines to files,
 * read file contents, and unmount the filesystem. It supports automatic log rotation
 * when a log exceeds MAX_LOG_SIZE and checks free space before writing.
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
     * Must be called before any file operations.
     *
     * @return ESP_OK if mounted successfully.
     * @return esp_err_t Error code if mounting fails.
     */
    esp_err_t open();

    /**
     * @brief Append a line of text to a log file.
     *
     * Automatically adds a newline at the end, checks free space, and rotates
     * the log if it exceeds MAX_LOG_SIZE.
     *
     * @param filename Name of the log file.
     * @param text Text to append.
     * @return ESP_OK if the append was successful.
     * @return esp_err_t Error code if appending fails or insufficient space.
     */
    esp_err_t log(const char* filename, const std::string& text);

    /**
     * @brief Read the entire contents of a file.
     *
     * @param filename Name of the file to read.
     * @param out String to store the file contents.
     * @return ESP_OK if the read was successful.
     * @return esp_err_t Error code if reading fails.
     */
    esp_err_t readAll(const char* filename, std::string& out);

    /**
     * @brief Unmount the LittleFS filesystem.
     *
     * Should be called when done logging.
     */
    void close();

private:
    const char* basePath; /**< Base path where LittleFS is mounted */
    bool mounted;         /**< True if filesystem is mounted */

    /**
     * @brief Rotate log files if the current log exceeds MAX_LOG_SIZE.
     *
     * Old log files are shifted and the current file is renamed with a numeric
     * suffix. Only MAX_LOG_BACKUPS backups are kept.
     *
     * @param filename Name of the log file.
     * @param newLineSize Size of the new log line to append.
     * @return ESP_OK on success.
     * @return esp_err_t Error code on failure.
     */
    esp_err_t rotateIfNeeded(const char* filename, size_t newLineSize);

    /**
     * @brief Check if there is enough free space on the filesystem.
     *
     * Ensures that MIN_FREE_SPACE bytes are left free after writing.
     *
     * @param bytes Number of bytes intended to write.
     * @return true if there is enough space.
     * @return false if not enough space.
     */
    bool hasEnoughSpace(size_t bytes);
};

extern Logger logger;