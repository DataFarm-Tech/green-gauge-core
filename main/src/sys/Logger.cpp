#include "Logger.hpp"
#include <cstdio>
#include <cstring>
#include <string>
#include <sstream>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>
#include "esp_log.h"
#include "esp_littlefs.h"

static const char* TAG = "Logger";
Logger logger; // Global logger instance

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

bool Logger::hasEnoughSpace(size_t bytes) {
    size_t total = 0, used = 0;
    esp_err_t err = esp_littlefs_info("littlefs", &total, &used);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get LittleFS info (%s)", esp_err_to_name(err));
        return false;
    }

    size_t free = total - used;
    return free >= (bytes + MIN_FREE_SPACE);
}


esp_err_t Logger::rotateIfNeeded(const char* filename, size_t newLineSize) {
    char path[256];
    snprintf(path, sizeof(path), "%s/%s", basePath, filename);

    struct stat st;
    if (stat(path, &st) == 0 && st.st_size + newLineSize > MAX_LOG_SIZE) {
        for (int i = MAX_LOG_BACKUPS - 1; i > 0; i--) {
            char oldPath[256], newPath[256];
            snprintf(oldPath, sizeof(oldPath), "%s/%s.%d", basePath, filename, i);
            snprintf(newPath, sizeof(newPath), "%s/%s.%d", basePath, filename, i+1);
            if (access(oldPath, F_OK) == 0) rename(oldPath, newPath);
        }
        char backup[256];
        snprintf(backup, sizeof(backup), "%s/%s.1", basePath, filename);
        rename(path, backup);
    }
    return ESP_OK;
}

esp_err_t Logger::log(const char* filename, const std::string& text) {
    if (!mounted) return ESP_ERR_INVALID_STATE;

    std::string line = text + "\n";

    if (!hasEnoughSpace(line.size())) {
        ESP_LOGE(TAG, "Not enough free space to append log (%zu bytes needed)", line.size());
        return ESP_ERR_NO_MEM;
    }

    rotateIfNeeded(filename, line.size());

    char path[256];
    snprintf(path, sizeof(path), "%s/%s", basePath, filename);

    FILE* f = fopen(path, "a");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open file %s for appending", path);
        return ESP_FAIL;
    }

    size_t written = fwrite(line.c_str(), 1, line.size(), f);
    fclose(f);

    if (written != line.size()) {
        ESP_LOGE(TAG, "Failed to append full data to %s", path);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Appended %zu bytes to %s", written, path);
    return ESP_OK;
}

esp_err_t Logger::readAll(const char* filename, std::string& out) {
    printf("mounted: %d\n", mounted);
    if (!mounted) return ESP_ERR_INVALID_STATE;

    char path[256];
    snprintf(path, sizeof(path), "%s/%s", basePath, filename);

    FILE* f = fopen(path, "r");
    if (!f) return ESP_FAIL;

    std::ostringstream ss;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), f)) ss << buffer;
    fclose(f);

    out = ss.str();
    return ESP_OK;
}

void Logger::close() {
    if (mounted) {
        esp_vfs_littlefs_unregister("littlefs");
        mounted = false;
    }
}
