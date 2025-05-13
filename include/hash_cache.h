#pragma once

#include <Arduino.h>
#include "eeprom.h"

#define HASH_CACHE_SIZE 10
#define HASH_SIZE 32


void hash_cache_init();
bool hash_cache_contains(const uint8_t *hash);
void hash_cache_add(const uint8_t *hash);