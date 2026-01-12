#include "Logger.hpp"
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <ctime>
#include <sys/stat.h>
#include "esp_log.h"
#include "esp_littlefs.h"

static const char* TAG = "Logger";

Logger g_logger; // Global instance

Logger::Logger(const char* basePath)
    : basePath(basePath), initialized(false) {}

esp_err_t Logger::init() {
    if (initialized) return ESP_OK;

    esp_vfs_littlefs_conf_t conf = {};  // zero-initialize

    conf.base_path = basePath;
    conf.partition_label = "littlefs";
    conf.format_if_mount_failed = true;
    conf.dont_mount = false;
    conf.read_only = false;
    conf.grow_on_mount = true;
    conf.partition = nullptr;

    esp_err_t ret = esp_vfs_littlefs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount LittleFS: %s", esp_err_to_name(ret));
        return ret;
    }

    initialized = true;
    ESP_LOGI(TAG, "Logger initialized at %s", basePath);
    return ESP_OK;
}

void Logger::log(const char* filename, const char* format, ...) {
    if (!initialized && init() != ESP_OK) return;

    char buffer[480];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    // Add [LOG] prefix
    std::string prefixed = "[LOG] ";
    prefixed += buffer;
    writeLog("system.log", prefixed);  // Changed to system.log
}

void Logger::info(const char* format, ...) {
    char buffer[480];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    ESP_LOGI(TAG, "%s", buffer);
    
    // Add [INFO] prefix and write to system.log
    std::string prefixed = "[INFO] ";
    prefixed += buffer;
    writeLog("system.log", prefixed);
}

void Logger::error(const char* format, ...) {
    char buffer[480];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    ESP_LOGE(TAG, "%s", buffer);
    
    // Add [ERROR] prefix and write to system.log
    std::string prefixed = "[ERROR] ";
    prefixed += buffer;
    writeLog("system.log", prefixed);  // Changed from error.log to system.log
}

void Logger::warning(const char* format, ...) {
    char buffer[480];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    ESP_LOGW(TAG, "%s", buffer);
    
    // Add [WARN] prefix and write to system.log
    std::string prefixed = "[WARN] ";
    prefixed += buffer;
    writeLog("system.log", prefixed);
}

bool Logger::hasSpace(size_t bytes) {
    size_t total = 0, used = 0;
    if (esp_littlefs_info("littlefs", &total, &used) != ESP_OK) {
        return false;
    }
    return (total - used) >= (bytes + MIN_FREE_SPACE);
}

esp_err_t Logger::rotateIfNeeded(const char* filename, size_t newSize) {
    char path[256];
    snprintf(path, sizeof(path), "%s/%s", basePath, filename);

    struct stat st;
    if (stat(path, &st) == 0 && st.st_size + newSize > MAX_LOG_SIZE) {
        // Rotate old backups
        for (int i = MAX_LOG_BACKUPS - 1; i > 0; i--) {
            char old[256], newer[256];
            snprintf(old, sizeof(old), "%s/%s.%d", basePath, filename, i);
            snprintf(newer, sizeof(newer), "%s/%s.%d", basePath, filename, i + 1);
            rename(old, newer); // Ignore errors if file doesn't exist
        }
        
        // Move current to .1
        char backup[256];
        snprintf(backup, sizeof(backup), "%s/%s.1", basePath, filename);
        rename(path, backup);
        
        ESP_LOGI(TAG, "Rotated %s", filename);
    }
    return ESP_OK;
}

esp_err_t Logger::writeLog(const char* filename, const std::string& text) {
    if (!initialized) return ESP_ERR_INVALID_STATE;

    std::string line = text + "\n";

    if (!hasSpace(line.size())) {
        ESP_LOGE(TAG, "Insufficient space for log");
        return ESP_ERR_NO_MEM;
    }

    rotateIfNeeded(filename, line.size());

    char path[256];
    snprintf(path, sizeof(path), "%s/%s", basePath, filename);

    FILE* f = fopen(path, "a");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open %s", path);
        return ESP_FAIL;
    }

    fwrite(line.c_str(), 1, line.size(), f);
    fclose(f);
    return ESP_OK;
}

esp_err_t Logger::read(const char* filename, std::string& out) {
    if (!initialized) return ESP_ERR_INVALID_STATE;

    char path[256];
    snprintf(path, sizeof(path), "%s/%s", basePath, filename);

    FILE* f = fopen(path, "r");
    if (!f) return ESP_FAIL;

    char buffer[256];
    out.clear();
    while (fgets(buffer, sizeof(buffer), f)) {
        out += buffer;
    }
    fclose(f);
    return ESP_OK;
}

void Logger::deinit() {
    if (initialized) {
        esp_vfs_littlefs_unregister("littlefs");
        initialized = false;
    }
}