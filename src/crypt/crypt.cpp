#include <stdint.h>
#include <Arduino.h>
#include "crypt.h"
#include <Crypto.h>  // For SHA256

// Define the constants as macros
#define CRC16_CCITT_FALSE_INIT 0xFFFF  // Initial CRC value
#define CRC16_POLYNOMIAL 0x11021      // Polynomial for CRC-16-CCITT-FALSE

void calc_crc(uint8_t *buffer, size_t length);
uint16_t calc_crc_16_ccitt_false(uint8_t *data, size_t length);

/**
 * 
 * @brief This function calculates the CRC-16-CCITT-FALSE checksum of the data in the buffer and appends it to the end of the buffer.
 * @param buffer - The data buffer for which the CRC needs to be calculated
 * @param length - The length of the data in the buffer
 * @return None
 */
void calc_crc(uint8_t *buffer, size_t length) 
{
    // Calculate CRC of the data in the buffer
    uint16_t crc = calc_crc_16_ccitt_false(buffer, length);
  
    // Append CRC to the end of the buffer (2 bytes for CRC-16)
    buffer[length] = (crc >> 8) & 0xFF;  // High byte
    buffer[length + 1] = crc & 0xFF;     // Low byte
}

/**
 * 
 * @brief This function calculates the CRC-16-CCITT-FALSE checksum of the data.
 * @param data - The data for which the CRC needs to be calculated
 * @param length - The length of the data
 * @return uint16_t - The calculated CRC-16-CCITT-FALSE checksum
 */
uint16_t calc_crc_16_ccitt_false(uint8_t *data, size_t length) 
{
    uint16_t crc = CRC16_CCITT_FALSE_INIT;  // Use the defined initial CRC value
    for (size_t i = 0; i < length; i++) 
    {
        crc ^= (data[i] << 8);  // XOR byte into the top byte of CRC
        for (uint8_t bit = 0; bit < 8; bit++) 
        {
            if (crc & 0x8000) 
            {
                crc = (crc << 1) ^ CRC16_POLYNOMIAL;  // Use the defined polynomial
            } 
            else 
            {
                crc <<= 1;
            }

            crc &= 0xFFFF;  // Keep CRC within 16 bits
        }
    }
    return crc;
}


/**
 * @brief The following function generates a hash.
 * @param str_to_hash The string that is being hashed
 * @param out_hash A array that is populated and used where ever this is called.
 * @return None
 */
void generate_hash(String str_to_hash, byte * out_hash)
{
    SHA256 hasher;
    hasher.doUpdate(str_to_hash.c_str(), str_to_hash.length());
    hasher.doFinal(out_hash);
}