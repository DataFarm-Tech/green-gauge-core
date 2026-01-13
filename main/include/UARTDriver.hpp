#pragma once
#include "driver/uart.h"
#include <string>
#include "Types.hpp"

/**
 * @class   UARTDriver
 * @brief   Lightweight UART wrapper for console I/O.
 *
 * This class provides simple formatted printing and byte-level input handling
 * over the ESP-IDF UART driver. Each instance manages a specific UART port,
 * allowing multiple UART interfaces to be used simultaneously (e.g., one for
 * serial monitor on UART0, another for Quectel module on UART1).
 *
 * The console must be initialized with `init()` before any read or write
 * operations are performed.
 */
class UARTDriver {
public:
    /**
     * @brief Construct a new UART Console instance.
     *
     * Creates a console object tied to a specific UART port number.
     * The UART is not configured until init() is called.
     *
     * @param uart_num UART port number (e.g., UART_NUM_0, UART_NUM_1, UART_NUM_2)
     */
    UARTDriver(uart_port_t uart_num);

    /**
     * @brief Initialize the UART console.
     *
     * Configures the selected UART for console communication using the 
     * specified baud rate and pin assignments. This function sets up 
     * buffer sizes, UART parameters (8N1), and installs the UART driver.
     *
     * @param baud Baud rate (e.g., 115200)
     * @param tx_pin TX pin number (use UART_PIN_NO_CHANGE to keep default)
     * @param rx_pin RX pin number (use UART_PIN_NO_CHANGE to keep default)
     * @param rts_pin RTS pin (use UART_PIN_NO_CHANGE if not using flow control)
     * @param cts_pin CTS pin (use UART_PIN_NO_CHANGE if not using flow control)
     * @param rx_buffer_size RX buffer size in bytes (default: 1024)
     * @param tx_buffer_size TX buffer size in bytes (default: 0 = no TX buffer)
     */
    void init(int baud, 
              int tx_pin = UART_PIN_NO_CHANGE, 
              int rx_pin = UART_PIN_NO_CHANGE,
              int rts_pin = UART_PIN_NO_CHANGE,
              int cts_pin = UART_PIN_NO_CHANGE,
              int rx_buffer_size = GEN_BUFFER_SIZE,
              int tx_buffer_size = 0);

    /**
     * @brief Write a null-terminated string to the UART.
     *
     * Sends the provided text exactly as-is (no formatting), blocking until
     * the data has been queued for transmission.
     *
     * @param text C-string to write.
     */
    void write(const char* text);

    /**
     * @brief Write formatted text to the UART.
     *
     * Works like `printf()`, supporting full `printf` formatting options.
     * Data is formatted into an internal buffer before being written.
     *
     * @param fmt Format string.
     * @param ... Additional arguments for the format string.
     */
    void writef(const char* fmt, ...);

    /**
     * @brief Read a single byte from the UART (non-blocking).
     *
     * Attempts to read 1 byte from the UART RX buffer.  
     * If a byte is available, it is written into `out` and the function
     * returns `1`.  
     * If no data is available, the function returns `0`.  
     * A negative return value indicates a UART driver error.
     *
     * @param out Reference where the read byte will be stored.
     * @return Number of bytes read:  
     *         - `1` = byte read  
     *         - `0` = no data  
     *         - `<0` = error
     */
    int readByte(uint8_t &out);

    /**
     * @brief Get the UART port number for this instance.
     *
     * @return uart_port_t The UART port number.
     */
    uart_port_t getPort() const { return m_uart_num; }

private:
    uart_port_t m_uart_num;  ///< UART port number for this instance
};