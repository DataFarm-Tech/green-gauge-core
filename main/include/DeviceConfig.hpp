#pragma once

#include <cstdint>
#include <string>

#include "MeasurementType.hpp"

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
    MANF_entry_t sim_mod_sn;
    MANF_entry_t sim_card_sn;
    MANF_entry_t chassis_ver;
} MANF_info_t;

typedef struct {
    bool has_activated;
    std::string gps_coord;
    uint32_t main_app_delay;
    uint64_t session_count;
    uint8_t secretKey[32];
    MANF_info_t manf_info;
    NPK_Calib_t calib;
} DeviceConfig;

extern DeviceConfig g_device_config;
