#pragma once
#include "driver/uart.h"
#include "driver/usb_serial_jtag.h"
#include <string>
#include "Types.hpp"

#define BAUD_4800 4800
#define BAUD_115200 115200
#define BAUD_9600 9600

/**
 * @class   UARTDriver
 * @brief   Lightweight UART wrapper for console I/O with USB-Serial-JTAG support.
 *
 * This class provides simple formatted printing and byte-level input handling
 * over either the ESP-IDF UART driver or the USB-Serial-JTAG driver (for ESP32-S3 USB).
 */
class UARTDriver {
public:
    enum class DriverType {
        UART,           // Standard UART peripheral
        USB_SERIAL_JTAG // USB-Serial-JTAG peripheral (ESP32-S3 USB)
    };

    /**
     * @brief Construct a new UART Console instance.
     *
     * @param uart_num UART port number (e.g., UART_NUM_0, UART_NUM_1, UART_NUM_2)
     * @param driver_type Type of driver to use (UART or USB_SERIAL_JTAG)
     */
    UARTDriver(uart_port_t uart_num, DriverType driver_type = DriverType::UART);

    /**
     * @brief Initialize the UART console.
     *
     * For USB_SERIAL_JTAG, baud rate and pins are ignored (USB handles this).
     * For standard UART, all parameters apply normally.
     *
     * @param baud Baud rate (ignored for USB)
     * @param tx_pin TX pin number (ignored for USB)
     * @param rx_pin RX pin number (ignored for USB)
     * @param rts_pin RTS pin (ignored for USB)
     * @param cts_pin CTS pin (ignored for USB)
     * @param rx_buffer_size RX buffer size in bytes
     * @param tx_buffer_size TX buffer size in bytes
     */
    void init(int baud, 
              int tx_pin = UART_PIN_NO_CHANGE, 
              int rx_pin = UART_PIN_NO_CHANGE,
              int rts_pin = UART_PIN_NO_CHANGE,
              int cts_pin = UART_PIN_NO_CHANGE,
              int rx_buffer_size = GEN_BUFFER_SIZE,
              int tx_buffer_size = GEN_BUFFER_SIZE);

    /**
     * @brief Write a null-terminated string to the console.
     */
    void write(const char* text);

    /**
     * @brief Write formatted text to the console.
     */
    void writef(const char* fmt, ...);

    /**
     * @brief Read a single byte (non-blocking).
     *
     * @param out Reference where the read byte will be stored.
     * @return Number of bytes read (1 = success, 0 = no data, <0 = error)
     */
    int readByte(uint8_t &out);

    /**
     * @brief Get the UART port number for this instance.
     */
    uart_port_t getPort() const { return m_uart_num; }

    /**
     * @brief Get the driver type for this instance.
     */
    DriverType getDriverType() const { return m_driver_type; }

private:
    uart_port_t m_uart_num;
    DriverType m_driver_type;
};