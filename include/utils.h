#pragma once

#include <Arduino.h>
#include <stdio.h>
#include <ESP32Ping.h>

#include "config.h"

#define cli_printf Serial.printf
#define cli_print Serial.print

#define BUFFER_SIZE 128

#define DEBUG() printf("%s: %d: %s\n", __func__, __LINE__, __FILE__)

#define PRINT_STR(to_print) printf("[%s]: %s\n", ID, to_print)
#define PRINT_INT(to_print) printf("[%s]: %d\n", ID, to_print)

/**
 * @brief A macro to print a string with a specific format
 * @param to_print - The string to print
 * @param format - The format string
 * @param ... - The arguments to format
 */
#define PRINT_INFO(to_print) printf("[INFO]: %s\n", to_print)
#define PRINT_WARNING(to_print) printf("[WARNING]: %s\n", to_print)
#define PRINT_ERROR(to_print) printf("[ERROR]: %s\n", to_print)

/**
 * @brief A macro to get the maximum of two values
 * @param a - The first value
 * @param b - The second value
 * @return The maximum of a and b
 */
#define MIN(a, b) ((a) < (b) ? (a) : (b))


void print_prompt();
void print_motd();
void trim_newline(char * str);