#pragma once
#include <Arduino.h>

struct device_config {
    char api_key[128];
};
extern device_config config;


void init_eeprom_int();
void save_config();
void read_config();
void clear_config();