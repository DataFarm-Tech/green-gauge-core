#include "EEPROMConfig.hpp"

bool EEPROMConfig::begin() {
    esp_err_t err = nvs_open(NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS namespace: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}

void EEPROMConfig::close() {
    if (handle != 0) {
        nvs_close(handle);
        handle = 0;
    }
}

bool EEPROMConfig::saveConfig(const DeviceConfig& config) {
    if (handle == 0) return false;

    esp_err_t err = nvs_set_blob(handle, KEY, &config, sizeof(DeviceConfig));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write config blob: %s", esp_err_to_name(err));
        return false;
    }

    err = nvs_commit(handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Commit failed: %s", esp_err_to_name(err));
        return false;
    }

    ESP_LOGI(TAG, "Configuration saved to NVS.");
    return true;
}

bool EEPROMConfig::loadConfig(DeviceConfig& config) {
    if (handle == 0) return false;

    size_t size = sizeof(DeviceConfig);
    esp_err_t err = nvs_get_blob(handle, KEY, &config, &size);

    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "No saved configuration found in NVS.");
        return false;
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to load config blob: %s", esp_err_to_name(err));
        return false;
    }

    ESP_LOGI(TAG, "Configuration loaded from NVS.");
    return true;
}

bool EEPROMConfig::eraseConfig() {
    if (handle == 0) return false;

    esp_err_t err = nvs_erase_key(handle, KEY);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to erase config: %s", esp_err_to_name(err));
        return false;
    }

    nvs_commit(handle);
    ESP_LOGI(TAG, "Configuration erased.");
    return true;
}
