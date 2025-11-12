#pragma once

#include <cstdint>
#include <cstddef>
#include "driver/i2c.h"
#include "esp_err.h"

/**
 * @class I2CDriver
 * @brief Lightweight wrapper around ESP-IDF's I2C master interface.
 *
 * This class simplifies initialization and communication with I2C devices
 * using the ESP-IDF driver API. It supports common operations such as
 * read, write, and write-then-read (for register-based devices).
 *
 * Example usage:
 * @code
 * I2CDriver i2c(I2C_NUM_0, GPIO_NUM_3, GPIO_NUM_2, 400000);
 * if (i2c.init()) {
 *     uint8_t reg = 0x02;
 *     uint8_t data[2];
 *     i2c.writeRead(0x36, &reg, 1, data, 2);
 * }
 * @endcode
 */
class I2CDriver {
public:
    /**
     * @brief Construct a new I2CDriver instance.
     *
     * @param port     The I2C port number (e.g., I2C_NUM_0 or I2C_NUM_1).
     * @param sda      The GPIO pin used for SDA.
     * @param scl      The GPIO pin used for SCL.
     * @param freqHz   The I2C clock frequency in hertz (default: 400kHz).
     */
    I2CDriver(i2c_port_t port, gpio_num_t sda, gpio_num_t scl, uint32_t freqHz = 400000);

    /**
     * @brief Initialize the I2C hardware bus.
     *
     * Configures the SDA/SCL pins, sets the bus frequency, and installs the I2C driver.
     * This method is idempotent — calling it multiple times will have no effect after
     * successful initialization.
     *
     * @return true if the bus was successfully initialized or already active.
     * @return false if initialization failed (check logs for details).
     */
    bool init();

    /**
     * @brief Perform a combined write/read transaction.
     *
     * This function writes a sequence of bytes (typically a register address)
     * to the device, then immediately reads data back from it in a single
     * transaction.
     *
     * Equivalent to calling:
     *   - i2c_master_write_read_device() in ESP-IDF.
     *
     * @param deviceAddr  7-bit I2C device address.
     * @param writeData   Pointer to the data to write (e.g., register address).
     * @param writeLen    Number of bytes to write.
     * @param readData    Pointer to buffer where read data will be stored.
     * @param readLen     Number of bytes to read.
     * @return true if the transaction succeeded, false otherwise.
     */
    bool writeRead(uint8_t deviceAddr, const uint8_t* writeData, size_t writeLen,
                   uint8_t* readData, size_t readLen);

    /**
     * @brief Write a sequence of bytes to a device.
     *
     * Sends raw data over the I2C bus. Often used to set device registers
     * or send configuration data.
     *
     * Equivalent to i2c_master_write_to_device() in ESP-IDF.
     *
     * @param deviceAddr  7-bit I2C device address.
     * @param data        Pointer to the buffer of bytes to send.
     * @param length      Number of bytes to send.
     * @return true if the operation succeeded, false otherwise.
     */
    bool write(uint8_t deviceAddr, const uint8_t* data, size_t length);

    /**
     * @brief Read a sequence of bytes from a device.
     *
     * Requests `length` bytes from the specified I2C address.
     * Typically used when a device supports direct reading
     * without prior register addressing.
     *
     * Equivalent to i2c_master_read_from_device() in ESP-IDF.
     *
     * @param deviceAddr  7-bit I2C device address.
     * @param buffer      Pointer to the destination buffer.
     * @param length      Number of bytes to read.
     * @return true if the operation succeeded, false otherwise.
     */
    bool read(uint8_t deviceAddr, uint8_t* buffer, size_t length);

private:
    /** @brief ESP-IDF I2C port identifier (I2C_NUM_0 or I2C_NUM_1). */
    i2c_port_t port;

    /** @brief GPIO pin used for the I2C SDA line. */
    gpio_num_t sda;

    /** @brief GPIO pin used for the I2C SCL line. */
    gpio_num_t scl;

    /** @brief I2C bus frequency in hertz (typically 100kHz or 400kHz). */
    uint32_t freqHz;

    /** @brief Flag indicating whether the driver is already initialized. */
    bool initialized;
};
