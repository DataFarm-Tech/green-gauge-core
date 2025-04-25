#ifndef UTILS_H
#define UTILS_H

#include "config.h"
#include <stdio.h>

#define cli_printf Serial.printf
#define cli_print Serial.print

#define DEBUG() printf("%s: %d: %s\n", __func__, __LINE__, __FILE__)

#define PRINT_STR(to_print) printf("[%s]: %s\n", ID, to_print)
#define PRINT_INT(to_print) printf("[%s]: %d\n", ID, to_print)

#define PRINT_INFO(to_print) printf("[INFO]: %s\n", to_print)
#define PRINT_WARNING(to_print) printf("[WARNING]: %s\n", to_print)
#define PRINT_ERROR(to_print) printf("[ERROR]: %s\n", to_print)


#endif // UTILS_H