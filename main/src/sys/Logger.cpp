#include "Logger.hpp"
#include <cstdio>
#include "esp_log.h"
#include "esp_littlefs.h"

static const char* TAG = "Logger";

Logger::Logger(const char* basePath)
    : basePath(basePath), mounted(false) {}

esp_err_t Logger::open() {
    esp_vfs_littlefs_conf_t conf = {};  // zero-initialize

    conf.base_path = basePath;
    conf.partition_label = "littlefs";
    conf.format_if_mount_failed = true;
    conf.dont_mount = false;
    conf.read_only = false;
    conf.grow_on_mount = true;
    conf.partition = nullptr;

    esp_err_t err = esp_vfs_littlefs_register(&conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount LittleFS (%s)", esp_err_to_name(err));
        return err;
    }

    mounted = true;
    ESP_LOGI(TAG, "LittleFS mounted at %s", basePath);
    return ESP_OK;
}

esp_err_t Logger::log(const char* filename, const std::string& text) {
    if (!mounted) {
        ESP_LOGE(TAG, "Filesystem not mounted. Cannot append to file %s", filename);
        return ESP_ERR_INVALID_STATE;
    }

    char path[256];
    snprintf(path, sizeof(path), "%s/%s", basePath, filename);

    FILE* f = fopen(path, "a");  // Open for append
    if (!f) {
        ESP_LOGE(TAG, "Failed to open file %s for appending", path);
        return ESP_FAIL;
    }

    std::string line = text + "\n";  // Add newline automatically
    size_t written = fwrite(line.c_str(), 1, line.size(), f);
    fclose(f);

    if (written != line.size()) {
        ESP_LOGE(TAG, "Failed to append full data to %s", path);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Appended %zu bytes to %s", written, path);
    return ESP_OK;
}

void Logger::close() {
    if (mounted) {
        esp_vfs_littlefs_unregister("littlefs");
        mounted = false;
    }
}
