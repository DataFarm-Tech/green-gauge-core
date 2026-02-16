#include "EEPROMConfig.hpp"
#include <cstring>
#include <cstdlib>

EEPROMConfig eeprom;

void EEPROMConfig::init_nvs() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

bool EEPROMConfig::begin() {
    init_nvs();
    
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

// Helper function implementations
bool EEPROMConfig::readString(const char* key, char* dest, size_t max_len) {
    size_t required_size = max_len;
    esp_err_t err = nvs_get_str(handle, key, dest, &required_size);
    return (err == ESP_OK);
}

bool EEPROMConfig::readBool(const char* key, bool* dest) {
    uint8_t value;
    esp_err_t err = nvs_get_u8(handle, key, &value);
    if (err == ESP_OK) {
        *dest = (value != 0);
        return true;
    }
    return false;
}

bool EEPROMConfig::readU8(const char* key, uint8_t* dest) {
    return (nvs_get_u8(handle, key, dest) == ESP_OK);
}

bool EEPROMConfig::readU32(const char* key, uint32_t* dest) {
    return (nvs_get_u32(handle, key, dest) == ESP_OK);
}

bool EEPROMConfig::readU64(const char* key, uint64_t* dest) {
    return (nvs_get_u64(handle, key, dest) == ESP_OK);
}

bool EEPROMConfig::readFloat(const char* key, float* dest) {
    char str_value[16];
    if (readString(key, str_value, sizeof(str_value))) {
        *dest = static_cast<float>(atof(str_value));
        return true;
    }
    return false;
}

bool EEPROMConfig::writeFloat(const char* key, float value) {
    char str_value[16];
    snprintf(str_value, sizeof(str_value), "%.2f", static_cast<double>(value));
    return writeString(key, str_value);
}

bool EEPROMConfig::writeString(const char* key, const char* value) {
    return (nvs_set_str(handle, key, value) == ESP_OK);
}

bool EEPROMConfig::writeBool(const char* key, bool value) {
    return (nvs_set_u8(handle, key, value ? 1 : 0) == ESP_OK);
}

bool EEPROMConfig::writeU8(const char* key, uint8_t value) {
    return (nvs_set_u8(handle, key, value) == ESP_OK);
}

bool EEPROMConfig::writeU32(const char* key, uint32_t value) {
    return (nvs_set_u32(handle, key, value) == ESP_OK);
}

bool EEPROMConfig::writeU64(const char* key, uint64_t value) {
    return (nvs_set_u64(handle, key, value) == ESP_OK);
}

bool EEPROMConfig::loadConfig(DeviceConfig& config) {
    if (handle == 0) return false;

    // Load manufacturing info
    readString("hw_ver", config.manf_info.hw_ver.value, MANF_MAX_LEN);
    
    readString("hw_var", config.manf_info.hw_var.value, MANF_MAX_LEN);
    
    readString("fw_ver", config.manf_info.fw_ver.value, MANF_MAX_LEN);
    
    readString("node_id", config.manf_info.nodeId.value, MANF_MAX_LEN);
    
    readString("secret_key", config.manf_info.secretkey.value, MANF_MAX_LEN);
    
    readString("p_code", config.manf_info.p_code.value, MANF_MAX_LEN);
    
    readString("sim_mod_sn", config.manf_info.sim_mod_sn.value, MANF_MAX_LEN);
    
    readString("sim_card_sn", config.manf_info.sim_card_sn.value, MANF_MAX_LEN);
    
    // Load activation status
    readBool("has_activated", &config.has_activated);

    readU32("main_app_delay", &config.main_app_delay);
    readU64("session_count", &config.session_count);
    
    // Load calibration data
    readFloat("cal_n_offset", &config.calib.calib_list[0].offset);
    readFloat("cal_n_gain", &config.calib.calib_list[0].gain);
    config.calib.calib_list[0].m_type = MeasurementType::Nitrogen;
    
    readFloat("cal_p_offset", &config.calib.calib_list[1].offset);
    readFloat("cal_p_gain", &config.calib.calib_list[1].gain);
    config.calib.calib_list[1].m_type = MeasurementType::Phosphorus;
    
    readFloat("cal_k_offset", &config.calib.calib_list[2].offset);
    readFloat("cal_k_gain", &config.calib.calib_list[2].gain);
    config.calib.calib_list[2].m_type = MeasurementType::Potassium;
    
    readFloat("cal_m_offset", &config.calib.calib_list[3].offset);
    readFloat("cal_m_gain", &config.calib.calib_list[3].gain);
    config.calib.calib_list[3].m_type = MeasurementType::Moisture;
    
    readFloat("cal_ph_offset", &config.calib.calib_list[4].offset);
    readFloat("cal_ph_gain", &config.calib.calib_list[4].gain);
    config.calib.calib_list[4].m_type = MeasurementType::PH;
    
    readFloat("cal_t_offset", &config.calib.calib_list[5].offset);
    readFloat("cal_t_gain", &config.calib.calib_list[5].gain);
    config.calib.calib_list[5].m_type = MeasurementType::Temperature;
    
    readU32("last_cal_ts", &config.calib.last_cal_ts);
    
    ESP_LOGI(TAG, "Configuration loaded from NVS (Node ID: %s)", config.manf_info.nodeId.value);
    return true;
}

bool EEPROMConfig::saveConfig(const DeviceConfig& config) {
    if (handle == 0) return false;

    // Save manufacturing info
    writeString("hw_ver", config.manf_info.hw_ver.value);
    
    writeString("hw_var", config.manf_info.hw_var.value);
    
    writeString("fw_ver", config.manf_info.fw_ver.value);
    
    writeString("node_id", config.manf_info.nodeId.value);
    
    writeString("secret_key", config.manf_info.secretkey.value);
    
    writeString("p_code", config.manf_info.p_code.value);

    writeString("sim_mod_sn", config.manf_info.sim_mod_sn.value);
    writeString("sim_card_sn", config.manf_info.sim_card_sn.value);
    
    // Save activation status
    writeBool("has_activated", config.has_activated);
    writeU32("main_app_delay", config.main_app_delay);
    writeU64("session_count", config.session_count);
    
    // Save calibration data
    writeFloat("cal_n_offset", config.calib.calib_list[0].offset);
    writeFloat("cal_n_gain", config.calib.calib_list[0].gain);
    
    writeFloat("cal_p_offset", config.calib.calib_list[1].offset);
    writeFloat("cal_p_gain", config.calib.calib_list[1].gain);
    
    writeFloat("cal_k_offset", config.calib.calib_list[2].offset);
    writeFloat("cal_k_gain", config.calib.calib_list[2].gain);
    
    writeFloat("cal_m_offset", config.calib.calib_list[3].offset);
    writeFloat("cal_m_gain", config.calib.calib_list[3].gain);
    
    writeFloat("cal_ph_offset", config.calib.calib_list[4].offset);
    writeFloat("cal_ph_gain", config.calib.calib_list[4].gain);
    
    writeFloat("cal_t_offset", config.calib.calib_list[5].offset);
    writeFloat("cal_t_gain", config.calib.calib_list[5].gain);
    
    writeU32("last_cal_ts", config.calib.last_cal_ts);
    
    esp_err_t err = nvs_commit(handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Commit failed: %s", esp_err_to_name(err));
        return false;
    }

    ESP_LOGI(TAG, "Configuration saved to NVS");
    return true;
}

bool EEPROMConfig::eraseConfig() {
    if (handle == 0) return false;

    esp_err_t err = nvs_erase_all(handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to erase config: %s", esp_err_to_name(err));
        return false;
    }

    nvs_commit(handle);
    ESP_LOGI(TAG, "Configuration erased");
    return true;
}