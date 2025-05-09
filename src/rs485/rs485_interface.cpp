#include "rs485_interface.h"
#include <Arduino.h>
#include "utils.h"
#include "pack_def/pack_def.h"

/**
 * @brief Simulates reading data from an RS485 interface.
 * @param buf Pointer to the buffer where the data will be stored.
 * @param buf_len Length of the buffer.
 * @note This function fills the buffer with random data for simulation purposes.
 *       In a real implementation, this function would read data from the RS485 interface.
 */
void read_rs485(uint8_t * buf, uint8_t buf_len)
{
    if (buf == NULL || buf_len == 0)
    {
        printf("Error: read_rs485 requires a non-null buffer and a non-zero length.\n");
        DEBUG();
        return;
    }

    for (int i = PACKET_LENGTH - DATA_SIZE; i < DATA_SIZE; i++)
    {
        buf[i] = (uint8_t)random(0, 255); // Simulate reading data from RS485
    }

    return;
}