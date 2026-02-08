#pragma once
#include <string>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>


#define MAX_LOG_SIZE    (512 * 1024) // 512 KB per log file
#define MAX_LOG_BACKUPS 3            // Keep 3 rotated logs
#define MIN_FREE_SPACE  (10 * 1024)  // 10 KB safety buffer

/**
 * @class Logger
 * @brief Simple file logger with automatic rotation on LittleFS
 * 
 * Provides convenient logging to files stored on the LittleFS partition.
 * Automatically handles filesystem mounting, log rotation when files exceed
 * MAX_LOG_SIZE, and space management to prevent filesystem overflow.
 */
class Logger {
public:
    /**
     * @brief Construct a new Logger object
     * @param basePath Base mount path for LittleFS (default: "/rootfs")
     */
    Logger(const char* basePath = "/rootfs");
    
    /**
     * @brief Initialize the LittleFS filesystem
     * 
     * Mounts the "littlefs" partition at the base path. This should be called
     * once at startup, though log methods will auto-initialize if needed.
     * 
     * @return ESP_OK on success
     * @return ESP error code on failure
     */
    esp_err_t init();
    
    /**
     * @brief Write formatted text to a specific log file
     * 
     * Auto-initializes filesystem if not already mounted. Supports printf-style
     * formatting. Automatically rotates the log if it exceeds MAX_LOG_SIZE.
     * 
     * @param filename Name of the log file (e.g., "custom.log")
     * @param format Printf-style format string
     * @param ... Variable arguments for format string
     * 
     * @note Does not output to console - file only
     * 
     * Example:
     * @code
     * g_logger.log("network.log", "Connected to %s with IP %s", ssid, ip);
     * @endcode
     */
    void log(const char* filename, const char* format, ...);
    
    /**
     * @brief Log informational message to console and system.log
     * 
     * Outputs to both ESP_LOGI console and appends to "system.log" file.
     * Auto-initializes filesystem if needed.
     * 
     * @param format Printf-style format string
     * @param ... Variable arguments for format string
     * 
     * Example:
     * @code
     * g_logger.info("System started, version %s", version);
     * @endcode
     */
    void info(const char* format, ...);
    
    /**
     * @brief Log error message to console and error.log
     * 
     * Outputs to both ESP_LOGE console and appends to "error.log" file.
     * Auto-initializes filesystem if needed.
     * 
     * @param format Printf-style format string
     * @param ... Variable arguments for format string
     * 
     * Example:
     * @code
     * g_logger.error("Failed to connect: error code %d", err);
     * @endcode
     */
    void error(const char* format, ...);
    
    /**
     * @brief Log warning message to console and system.log
     * 
     * Outputs to both ESP_LOGW console and appends to "system.log" file.
     * Auto-initializes filesystem if needed.
     * 
     * @param format Printf-style format string
     * @param ... Variable arguments for format string
     * 
     * Example:
     * @code
     * g_logger.warning("Battery low: %d%%", battery_pct);
     * @endcode
     */
    void warning(const char* format, ...);
    
    /**
     * @brief Read entire contents of a log file
     * 
     * Reads the complete contents of the specified log file into a string.
     * Useful for CLI commands or uploading logs.
     * 
     * @param filename Name of the log file to read
     * @param out String to store the file contents
     * @return ESP_OK if file was read successfully
     * @return ESP_FAIL if file doesn't exist or read failed
     * @return ESP_ERR_INVALID_STATE if filesystem not initialized
     * 
     * Example:
     * @code
     * std::string contents;
     * if (g_logger.read("system.log", contents) == ESP_OK) {
     *     printf("%s", contents.c_str());
     * }
     * @endcode
     */
    esp_err_t read(const char* filename, std::string& out);
    
    /**
     * @brief Unmount the LittleFS filesystem
     * 
     * Should be called before deep sleep or system shutdown. Not typically
     * needed for normal operation.
     */
    void deinit();

private:
    const char* basePath;  ///< Mount path for LittleFS
    bool initialized;      ///< True if filesystem is mounted
    SemaphoreHandle_t mutex;  // ADD THIS

    /**
     * @brief Internal method to write text to a log file
     * @param filename Target log file
     * @param text Text to append (newline will be added)
     * @return ESP_OK on success, error code on failure
     */
    esp_err_t writeLog(const char* filename, const std::string& text);
    
    /**
     * @brief Rotate log file if it exceeds MAX_LOG_SIZE
     * @param filename Log file to check
     * @param newSize Size of data about to be appended
     * @return ESP_OK on success
     */
    esp_err_t rotateIfNeeded(const char* filename, size_t newSize);
    
    /**
     * @brief Check if filesystem has enough free space
     * @param bytes Number of bytes to write
     * @return true if sufficient space available (with MIN_FREE_SPACE buffer)
     * @return false if insufficient space
     */
    bool hasSpace(size_t bytes);
};

/**
 * @brief Global logger instance
 * 
 * Use this instance throughout your application:
 * - g_logger.info("message") - Info to console + system.log
 * - g_logger.error("error") - Error to console + error.log  
 * - g_logger.log("file.log", "msg") - Custom file
 */
extern Logger g_logger;