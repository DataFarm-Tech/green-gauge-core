#include "UARTDriver.hpp"
#include <string.h>
#include <stdarg.h>
#include "esp_log.h"

static const char* TAG = "UARTDriver";

UARTDriver::UARTDriver(uart_port_t uart_num, DriverType driver_type) 
    : m_uart_num(uart_num), m_driver_type(driver_type) {
}

void UARTDriver::init(int baud, int tx_pin, int rx_pin, 
                       int rts_pin, int cts_pin,
                       int rx_buffer_size, int tx_buffer_size) {
    
    if (m_driver_type == DriverType::USB_SERIAL_JTAG) {
        // Configure USB-Serial-JTAG driver
        usb_serial_jtag_driver_config_t usb_config = {};
        usb_config.rx_buffer_size = rx_buffer_size;
        usb_config.tx_buffer_size = tx_buffer_size;
        
        esp_err_t ret = usb_serial_jtag_driver_install(&usb_config);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to install USB-Serial-JTAG driver: %s", esp_err_to_name(ret));
        } else {
            ESP_LOGI(TAG, "USB-Serial-JTAG driver installed successfully");
        }
        return;
    }
    
    // Standard UART configuration
    uart_config_t cfg = {};
    cfg.baud_rate = baud;
    cfg.data_bits = UART_DATA_8_BITS;
    cfg.parity = UART_PARITY_DISABLE;
    cfg.stop_bits = UART_STOP_BITS_1;
    cfg.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    cfg.rx_flow_ctrl_thresh = 122;

    uart_param_config(m_uart_num, &cfg);
    uart_set_pin(m_uart_num, tx_pin, rx_pin, rts_pin, cts_pin);
    uart_driver_install(m_uart_num, rx_buffer_size, tx_buffer_size, 0, NULL, 0);
    
    ESP_LOGI(TAG, "UART%d initialized at %d baud", m_uart_num, baud);
}

void UARTDriver::write(const char* text) {
    if (m_driver_type == DriverType::USB_SERIAL_JTAG) {
        size_t len = strlen(text);
        usb_serial_jtag_write_bytes(text, len, portMAX_DELAY);
        return;
    }
    
    uart_write_bytes(m_uart_num, text, strlen(text));
}

void UARTDriver::writef(const char* fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    write(buf);
}

int UARTDriver::readByte(uint8_t &out) {
    if (m_driver_type == DriverType::USB_SERIAL_JTAG) {
        // Non-blocking read from USB-Serial-JTAG
        int len = usb_serial_jtag_read_bytes(&out, 1, 0); // 0 ticks = non-blocking
        return (len > 0) ? 1 : 0;
    }
    
    // Standard UART read
    int len = uart_read_bytes(m_uart_num, &out, 1, 0);
    return (len > 0) ? 1 : 0;
}