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
#include "freertos/FreeRTOS.h"
#include "I2CDriver.hpp"

constexpr uint8_t MAX17048_I2C_ADDRESS = 0x36;
constexpr uint8_t MAX17048_REG_VCELL = 0x02;
constexpr uint8_t MAX17048_REG_SOC = 0x04;

constexpr i2c_port_t BATT_I2C_PORT = I2C_NUM_0;
constexpr gpio_num_t BATT_I2C_SDA = GPIO_NUM_3;  // SDA per Waveshare schematic
constexpr gpio_num_t BATT_I2C_SCL = GPIO_NUM_2;  // SCL per Waveshare schematic
constexpr uint32_t BATT_I2C_FREQ_HZ = 400000;

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
    // Static I2C driver instance - initialized once and reused
    static I2CDriver i2c(BATT_I2C_PORT, BATT_I2C_SDA, BATT_I2C_SCL, BATT_I2C_FREQ_HZ);

    //Reading voltage into rawVcell
    uint8_t rawVcell[2] = {0};
    uint8_t regVcell = MAX17048_REG_VCELL;
    if (!i2c.writeRead(MAX17048_I2C_ADDRESS, &regVcell, 1, rawVcell, sizeof(rawVcell))) {
        ESP_LOGE(TAG.c_str(), "Failed to read MAX17048 VCELL register");
        return false;
    }

    uint16_t rawVoltage = (static_cast<uint16_t>(rawVcell[0]) << 8) | rawVcell[1];
    float voltage = static_cast<float>(rawVoltage >> 4) * 0.00125f; // Each LSB = 1.25 mV
    
    //Reading SOC into rawSOC
    uint8_t rawSoc[2] = {0};
    uint8_t regSoc = MAX17048_REG_SOC;
    if (!i2c.writeRead(MAX17048_I2C_ADDRESS, &regSoc, 1, rawSoc, sizeof(rawSoc))) {
        ESP_LOGE(TAG.c_str(), "Failed to read MAX17048 SOC register");
        return false;
    }

    uint16_t socRaw = (static_cast<uint16_t>(rawSoc[0]) << 8) | rawSoc[1];
    float socPercent = static_cast<float>(socRaw) / 256.0f; // 1/256 % per LSB

    int roundedSoc = static_cast<int>(std::roundf(socPercent));
    roundedSoc = std::clamp(roundedSoc, 0, 100);

    level = static_cast<uint8_t>(roundedSoc);
    health = 100; // No direct health metric from MAX17048

    ESP_LOGI(TAG.c_str(),
             "MAX17048: voltage=%.3f V, SoC=%.2f%% (stored %u%%)",
             voltage, socPercent, static_cast<unsigned>(level));

    return true;
}
