#pragma once

#include <Arduino.h>
#include <stdio.h>

#include "config.h"

#define cli_printf Serial.printf
#define cli_print Serial.print

#define BUFFER_SIZE 128

char* constr_endp(const char* endpoint);

#define DEBUG() printf("%s: %d: %s\n", __func__, __LINE__, __FILE__)

#define PRINT_STR(to_print) printf("[%s]: %s\n", ID, to_print)
#define PRINT_INT(to_print) printf("[%s]: %d\n", ID, to_print)

#define PRINT_INFO(to_print) printf("[INFO]: %s\n", to_print)
#define PRINT_WARNING(to_print) printf("[WARNING]: %s\n", to_print)
#define PRINT_ERROR(to_print) printf("[ERROR]: %s\n", to_print)