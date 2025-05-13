#include "rs485_interface.h"
#include <Arduino.h>
#include "utils.h"
#include "pack_def.h"

/**
 * @brief Simulates reading data from an RS485 interface.
 * @param buf Pointer to the buffer where the data will be stored.
 * @param buf_len Length of the buffer.
 * @note This function fills the buffer with random data for simulation purposes.
 *       In a real implementation, this function would read data from the RS485 interface.
 */
uint8_t read_rs485(char * data, uint8_t buf_len)
{
    if (buf_len < DATA_SIZE)
    {
        PRINT_ERROR("Buffer length is too small");
        return EXIT_FAILURE;
    }

    bool rs485_disconnect = true;

    for (int i = 0; i < buf_len; i++)
    {
        data[i] = (char)random(0, 255); // Simulate reading a byte from RS485

        if (data[i] != 1)
        {
            rs485_disconnect = false;
        }
    }

    if (rs485_disconnect)
    {
        PRINT_ERROR("All RS485 data values were 1. Invalid response.");
        return SN001_ERR_RSP_CODE_A;
    }

    return EXIT_SUCCESS;
}
