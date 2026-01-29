#include "UARTDriver.hpp"
#include <string.h>
#include <stdarg.h>


UARTDriver::UARTDriver(uart_port_t uart_num) 
: m_uart_num(uart_num) {
}

void UARTDriver::init(int baud, int tx_pin, int rx_pin, 
                       int rts_pin, int cts_pin,
                       int rx_buffer_size, int tx_buffer_size) {
    uart_config_t cfg = {};
    cfg.baud_rate = baud;
    cfg.data_bits = UART_DATA_8_BITS;
    cfg.parity = UART_PARITY_DISABLE;
    cfg.stop_bits = UART_STOP_BITS_1;
    cfg.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    cfg.rx_flow_ctrl_thresh = 122;
    cfg.source_clk = UART_SCLK_APB;
    
    uart_param_config(m_uart_num, &cfg);
    uart_set_pin(m_uart_num, tx_pin, rx_pin, rts_pin, cts_pin);
    uart_driver_install(m_uart_num, rx_buffer_size, tx_buffer_size, 0, NULL, 0);
}

void UARTDriver::write(const char* text) {
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
    // Fully non-blocking: returns 1 if byte read, 0 if no data
    int len = uart_read_bytes(m_uart_num, &out, 1, 0); // 0 ticks => immediate return
    return (len > 0) ? 1 : 0;
}


UARTDriver m_modem_uart(UART_NUM_1);
   UARTDriver rs485_uart(UART_NUM_2);
