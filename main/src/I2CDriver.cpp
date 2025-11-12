#include "I2CDriver.hpp"
#include "esp_log.h"

constexpr const char *TAG = "I2CDriver";
constexpr TickType_t I2C_TIMEOUT_TICKS = pdMS_TO_TICKS(100);

I2CDriver::I2CDriver(i2c_port_t port, gpio_num_t sda, gpio_num_t scl, uint32_t freqHz)
    : port(port), sda(sda), scl(scl), freqHz(freqHz), initialized(false) {}

bool I2CDriver::init() {
    if (initialized) return true;

    i2c_config_t config = {}; // <-- declare it locally here
    config.mode = I2C_MODE_MASTER;
    config.sda_io_num = sda;
    config.scl_io_num = scl;
    config.sda_pullup_en = GPIO_PULLUP_ENABLE;
    config.scl_pullup_en = GPIO_PULLUP_ENABLE;
    config.master.clk_speed = freqHz;
    config.clk_flags = 0;

    esp_err_t err = i2c_param_config(port, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C param config failed: %s", esp_err_to_name(err));
        return false;
    }

    err = i2c_driver_install(port, config.mode, 0, 0, 0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "I2C driver install failed: %s", esp_err_to_name(err));
        return false;
    }

    initialized = true;
    return true;
}

bool I2CDriver::writeRead(uint8_t deviceAddr, const uint8_t* writeData, size_t writeLen,
                          uint8_t* readData, size_t readLen) {
    if (!init()) return false;
    esp_err_t err = i2c_master_write_read_device(
        port, deviceAddr, writeData, writeLen, readData, readLen, I2C_TIMEOUT_TICKS);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "writeRead failed: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}

bool I2CDriver::write(uint8_t deviceAddr, const uint8_t* data, size_t length) {
    if (!init()) return false;
    esp_err_t err = i2c_master_write_to_device(port, deviceAddr, data, length, I2C_TIMEOUT_TICKS);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "write failed: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}

bool I2CDriver::read(uint8_t deviceAddr, uint8_t* buffer, size_t length) {
    if (!init()) return false;
    esp_err_t err = i2c_master_read_from_device(port, deviceAddr, buffer, length, I2C_TIMEOUT_TICKS);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "read failed: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}
