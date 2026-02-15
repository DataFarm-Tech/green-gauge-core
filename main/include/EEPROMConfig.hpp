#pragma once

#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "Types.hpp"
#include "NPK.hpp"

#define MANF_MAX_LEN 32

typedef struct {
    float offset;
    float gain;
    MeasurementType m_type;
} DataCalib_t;

typedef struct {
    DataCalib_t calib_list[6]; 
    uint32_t last_cal_ts;
} NPK_Calib_t;

typedef struct {
    char value[MANF_MAX_LEN];
} MANF_entry_t;

typedef struct {
    MANF_entry_t hw_ver;
    MANF_entry_t hw_var;
    MANF_entry_t fw_ver;
    MANF_entry_t nodeId;
    MANF_entry_t secretkey;
    MANF_entry_t p_code;
    MANF_entry_t sim_sn;
} MANF_info_t;

typedef struct {
    bool has_activated;
    uint32_t main_app_delay;
    uint64_t session_count;
    MANF_info_t manf_info;
    NPK_Calib_t calib;
} DeviceConfig;

class EEPROMConfig {
private:
    nvs_handle_t handle;
    const char* TAG = "EEPROMConfig";
    const char* NAMESPACE = "config_ns";

    void init_nvs();
    
    // Helper functions for reading individual values
    bool readString(const char* key, char* dest, size_t max_len);
    bool readBool(const char* key, bool* dest);
    bool readU8(const char* key, uint8_t* dest);
    bool readU32(const char* key, uint32_t* dest);
    bool readU64(const char* key, uint64_t* dest);
    bool readFloat(const char* key, float* dest);
    
    // Helper functions for writing individual values
    bool writeString(const char* key, const char* value);
    bool writeBool(const char* key, bool value);
    bool writeU8(const char* key, uint8_t value);
    bool writeU32(const char* key, uint32_t value);
    bool writeU64(const char* key, uint64_t value);
    bool writeFloat(const char* key, float value);

public:
    EEPROMConfig() : handle(0) {}
    
    bool begin();
    void close();
    bool saveConfig(const DeviceConfig& config);
    bool loadConfig(DeviceConfig& config);
    bool eraseConfig();
};

extern DeviceConfig g_device_config;
extern EEPROMConfig eeprom;