#include "UARTConsole.hpp"
#include <string.h>
#include <stdarg.h>

#define CLI_UART UART_NUM_0

void UARTConsole::init(int baud) {
    uart_config_t cfg = {};
    cfg.baud_rate = baud;
    cfg.data_bits = UART_DATA_8_BITS;
    cfg.parity = UART_PARITY_DISABLE;
    cfg.stop_bits = UART_STOP_BITS_1;
    cfg.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;

    uart_param_config(CLI_UART, &cfg);
    uart_set_pin(CLI_UART, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE,
                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(CLI_UART, 1024, 0, 0, NULL, 0);
}

void UARTConsole::write(const char* text) {
    uart_write_bytes(CLI_UART, text, strlen(text));
}

void UARTConsole::writef(const char* fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    write(buf);
}

int UARTConsole::readByte(uint8_t &out) {
    return uart_read_bytes(CLI_UART, &out, 1, 10 / portTICK_PERIOD_MS);
}
