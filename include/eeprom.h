#pragma once
#include <Arduino.h>
#include "hash_cache.h"

#define HASH_CACHE_SIZE 10
#define HASH_SIZE 32

typedef struct {
    uint8_t entries[HASH_CACHE_SIZE][HASH_SIZE];
    uint8_t count;
    uint8_t head;
} hash_cache;


struct device_config {
    char api_key[128];
    hash_cache cache;
};
extern device_config config;


void init_eeprom_int();
void save_config();
void read_config();
void clear_config();