#pragma once

#include <Arduino.h>


#define SHA256_SIZE 32
#define CRC_SIZE 2

void calc_crc(uint8_t *buffer, size_t length);
uint16_t calc_crc_16_ccitt_false(const uint8_t * data, size_t length);
void generate_hash(String str_to_hash, byte * out_hash);
bool validate_crc(uint8_t *buffer, size_t length_with_crc);
