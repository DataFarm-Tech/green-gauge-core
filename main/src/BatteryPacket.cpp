#include <cbor.h>
#include <cstring>
#include <esp_log.h>
#include <cbor.h>
#include <sys/socket.h>
#include "esp_log.h"
#include <cstdint>
#include <cstddef>
#include "packet/BatteryPacket.hpp"
#include "Config.hpp"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#define BATT_ADC_CHANNEL ADC_CHANNEL_0 // GPIO1
#define BATT_MIN_VOLTAGE 3000 // 3.0V
#define BATT_MAX_VOLTAGE 4200 // 4.2V

const uint8_t * BatteryPacket::toBuffer()
{
    CborEncoder encoder, mapEncoder, arrayEncoder, batteryEncoder;
    cbor_encoder_init(&encoder, buffer, BUFFER_SIZE, 0);  // use member buffer

    if (cbor_encoder_create_map(&encoder, &mapEncoder, 2) != CborNoError) 
    {
        ESP_LOGE(TAG.c_str(), "Failed to create root map");
        return nullptr;
    }

    if (level == 0 || health == 0)
    {
        ESP_LOGE(TAG.c_str(), "Cannot append empty battery diagnostics to buffer");
        return nullptr;
    }
    

    cbor_encode_text_stringz(&mapEncoder, "node_id");
    cbor_encode_text_stringz(&mapEncoder, nodeId.c_str());

    cbor_encode_text_stringz(&mapEncoder, "batteries");
    cbor_encoder_create_array(&mapEncoder, &arrayEncoder, 1);
    cbor_encoder_create_map(&arrayEncoder, &batteryEncoder, 2);

    cbor_encode_text_stringz(&batteryEncoder, "bat_lvl");
    cbor_encode_int(&batteryEncoder, level);

    cbor_encode_text_stringz(&batteryEncoder, "bat_hlth");
    cbor_encode_int(&batteryEncoder, health);

    cbor_encoder_close_container(&arrayEncoder, &batteryEncoder);
    cbor_encoder_close_container(&mapEncoder, &arrayEncoder);
    cbor_encoder_close_container(&encoder, &mapEncoder);

    bufferLength = cbor_encoder_get_buffer_size(&encoder, buffer);
    if (bufferLength > BUFFER_SIZE) {
        ESP_LOGE(TAG.c_str(), "CBOR buffer overflow: %d bytes (max %d)", (int)bufferLength, BUFFER_SIZE);
        return nullptr;
    }

    return buffer;
}


bool BatteryPacket::readFromBMS() 
{
    // ADC Initialization
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    if (adc_oneshot_new_unit(&init_config1, &adc1_handle) != ESP_OK) {
        ESP_LOGE(TAG.c_str(), "Failed to initialize ADC1");
        return false;
    }

    // ADC Configuration
    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    if (adc_oneshot_config_channel(adc1_handle, BATT_ADC_CHANNEL, &config) != ESP_OK) {
        ESP_LOGE(TAG.c_str(), "Failed to configure ADC channel");
        adc_oneshot_del_unit(adc1_handle);
        return false;
    }

    // ADC Calibration
    adc_cali_handle_t cali_handle = NULL;
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    if (adc_cali_create_scheme_curve_fitting(&cali_config, &cali_handle) != ESP_OK) {
        ESP_LOGW(TAG.c_str(), "ADC calibration scheme creation failed");
    }

    int adc_raw;
    int voltage_mv = 0;

    if (adc_oneshot_read(adc1_handle, BATT_ADC_CHANNEL, &adc_raw) != ESP_OK) {
        ESP_LOGE(TAG.c_str(), "ADC read failed");
    } else {
        if (cali_handle) {
            if (adc_cali_raw_to_voltage(cali_handle, adc_raw, &voltage_mv) != ESP_OK) {
                ESP_LOGW(TAG.c_str(), "ADC calibration failed, using raw value");
                voltage_mv = 0; // Indicate failure
            }
        }
    }

    // Cleanup
    adc_oneshot_del_unit(adc1_handle);
    if (cali_handle) {
        adc_cali_delete_scheme_curve_fitting(cali_handle);
    }

    if (voltage_mv > 0) {
        // The board has a 100k/100k voltage divider, so multiply by 2
        int battery_voltage = voltage_mv * 2;
        ESP_LOGI(TAG.c_str(), "Raw ADC: %d, Calibrated Voltage: %d mV, Battery Voltage: %d mV", adc_raw, voltage_mv, battery_voltage);

        // Clamp voltage to defined min/max range
        if (battery_voltage < BATT_MIN_VOLTAGE) {
            battery_voltage = BATT_MIN_VOLTAGE;
        }
        if (battery_voltage > BATT_MAX_VOLTAGE) {
            battery_voltage = BATT_MAX_VOLTAGE;
        }

        // Convert voltage to percentage
        level = ((battery_voltage - BATT_MIN_VOLTAGE) * 100) / (BATT_MAX_VOLTAGE - BATT_MIN_VOLTAGE);
    } else {
        // Fallback if calibration fails
        level = 50; // Default to a safe-but-not-full value
        ESP_LOGE(TAG.c_str(), "Could not get calibrated voltage. Using fallback value for battery level.");
    }

    // Health is not directly measurable, so we assume it's 100%
    health = 100;

    ESP_LOGI(TAG.c_str(), "Battery Level: %d%%, Health: %d%%", level, health);

    return true;
}
