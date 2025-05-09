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
 * @brief Validates the CRC-16-CCITT-FALSE of the given buffer.
 * @param buffer - The buffer that includes both data and the appended CRC (last 2 bytes).
 * @param length_with_crc - Total length of the buffer, including the 2 CRC bytes.
 * @return true if CRC is valid, false otherwise.
 */
bool validate_crc(uint8_t *buffer, size_t length_with_crc)
{
    if (length_with_crc < 2)
        return false;  // Not enough data to contain a CRC

    size_t data_length = length_with_crc - 2;

    // Calculate the CRC of the data portion
    uint16_t computed_crc = calc_crc_16_ccitt_false(buffer, data_length);

    // Extract the CRC from the last two bytes of the buffer
    uint16_t received_crc = (buffer[data_length] << 8) | buffer[data_length + 1];

    // Compare calculated and received CRC
    return computed_crc == received_crc;
}


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