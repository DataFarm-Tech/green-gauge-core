#include <cbor.h>
#include <cstring>
#include <esp_log.h>
#include <cbor.h>
#include <sys/socket.h>
#include "esp_log.h"
#include <cstdint>
#include <cstddef>
#include <algorithm>
#include <cmath>
#include "packet/BatteryPacket.hpp"
#include "Config.hpp"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"

namespace {
constexpr const char * BATTERY_LOG_TAG = "BatteryPacket";
constexpr uint8_t MAX17048_I2C_ADDRESS = 0x36;
constexpr uint8_t MAX17048_REG_VCELL = 0x02;
constexpr uint8_t MAX17048_REG_SOC = 0x04;

constexpr i2c_port_t BATT_I2C_PORT = I2C_NUM_0;
constexpr gpio_num_t BATT_I2C_SDA = GPIO_NUM_3;  // SDA per Waveshare schematic
constexpr gpio_num_t BATT_I2C_SCL = GPIO_NUM_2;  // SCL per Waveshare schematic
constexpr uint32_t BATT_I2C_FREQ_HZ = 400000;

constexpr TickType_t I2C_TIMEOUT_TICKS = pdMS_TO_TICKS(100);

bool ensure_i2c_bus()
{
    static bool initialized = false;
    if (initialized)
    {
        return true;
    }

    i2c_config_t config = {};
    config.mode = I2C_MODE_MASTER;
    config.sda_io_num = BATT_I2C_SDA;
    config.scl_io_num = BATT_I2C_SCL;
    config.sda_pullup_en = GPIO_PULLUP_ENABLE;
    config.scl_pullup_en = GPIO_PULLUP_ENABLE;
    config.master.clk_speed = BATT_I2C_FREQ_HZ;
    config.clk_flags = 0;

    esp_err_t err = i2c_param_config(BATT_I2C_PORT, &config);
    if (err != ESP_OK)
    {
        ESP_LOGE(BATTERY_LOG_TAG, "I2C param config failed: %s", esp_err_to_name(err));
        return false;
    }

    err = i2c_driver_install(BATT_I2C_PORT, config.mode, 0, 0, 0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGE(BATTERY_LOG_TAG, "I2C driver install failed: %s", esp_err_to_name(err));
        return false;
    }

    initialized = true;
    return true;
}

bool read_register(uint8_t reg, uint8_t * buffer, size_t length)
{
    if (!ensure_i2c_bus())
    {
        return false;
    }

    esp_err_t err = i2c_master_write_read_device(
        BATT_I2C_PORT,
        MAX17048_I2C_ADDRESS,
        &reg,
        1,
        buffer,
        length,
        I2C_TIMEOUT_TICKS
    );

    if (err != ESP_OK)
    {
        ESP_LOGE(BATTERY_LOG_TAG, "I2C read failed for reg 0x%02X: %s", reg, esp_err_to_name(err));
        return false;
    }

    return true;
}
} // namespace

const uint8_t * BatteryPacket::toBuffer()
{
    CborEncoder encoder, mapEncoder, arrayEncoder, batteryEncoder;
    cbor_encoder_init(&encoder, buffer, BUFFER_SIZE, 0);  // use member buffer

    if (cbor_encoder_create_map(&encoder, &mapEncoder, 2) != CborNoError) 
    {
        ESP_LOGE(TAG.c_str(), "Failed to create root map");
        return nullptr;
    }

    if (level == 0 && health == 0)
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
    uint8_t rawVcell[2] = {0};
    if (!read_register(MAX17048_REG_VCELL, rawVcell, sizeof(rawVcell)))
    {
        ESP_LOGE(TAG.c_str(), "Failed to read MAX17048 VCELL register");
        return false;
    }

    uint16_t rawVoltage = (static_cast<uint16_t>(rawVcell[0]) << 8) | rawVcell[1];
    float voltage = static_cast<float>(rawVoltage >> 4) * 0.00125f; // Each LSB equals 1.25 mV

    uint8_t rawSoc[2] = {0};
    if (!read_register(MAX17048_REG_SOC, rawSoc, sizeof(rawSoc)))
    {
        ESP_LOGE(TAG.c_str(), "Failed to read MAX17048 SOC register");
        return false;
    }

    uint16_t socRaw = (static_cast<uint16_t>(rawSoc[0]) << 8) | rawSoc[1];
    float socPercent = static_cast<float>(socRaw) / 256.0f; // 1/256 % per LSB

    int roundedSoc = static_cast<int>(std::roundf(socPercent));
    roundedSoc = std::clamp(roundedSoc, 0, 100);

    level = static_cast<uint8_t>(roundedSoc);
    health = 100; // No direct health metric available from MAX17048

    ESP_LOGI(TAG.c_str(), "MAX17048: voltage=%.3f V, SoC=%.2f%% (stored %u%%)", voltage, socPercent, static_cast<unsigned>(level));

    return true;
}
