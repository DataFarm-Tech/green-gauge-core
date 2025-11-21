#pragma once
#include "driver/uart.h"
#include <string>

class UARTConsole {
public:
    static void init(int baud);
    static void write(const char* text);
    static void writef(const char* fmt, ...);
    static int readByte(uint8_t &out);
};
