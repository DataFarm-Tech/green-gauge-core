#include "NPK.hpp"
#include <stdio.h>
#include "Logger.hpp"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "EEPROMConfig.hpp"
#include "Types.hpp"
#include "UARTDriver.hpp"
#include "Utils.hpp"

NPK::NPK()
{
    // Constructor can initialize calibration, if needed
}

void NPK::sendModbusRequest(const uint8_t *packet, size_t packet_size)
{
    // uart_flush(UART_NUM_2); // Flush any existing data

    for (size_t i = 0; i < packet_size; i++)
    {
        rs485_uart.writeByte(packet[i]);
    }
    printf("%d Bytes written to UART NPK.\n", packet_size);
}

size_t NPK::readModbusResponse(uint8_t *rx_buffer, size_t buffer_size, uint32_t timeout_ms)
{
    buffer_size = 7;
    size_t len = 0;
    uint32_t start_time = xTaskGetTickCount();
    uint32_t timeout_ticks = pdMS_TO_TICKS(timeout_ms);

    while (len < buffer_size)
    {
        if (!rs485_uart.readByte(rx_buffer[len]))
        {
            if ((xTaskGetTickCount() - start_time) > timeout_ticks)
            {
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(1)); // Small delay to avoid busy-waiting
        }
        else
        {
            len++;
            start_time = xTaskGetTickCount();
        }
    }

    return len;
}

uint16_t NPK::calculateCRC16(const uint8_t *data, size_t length)
{
    uint16_t crc = 0xFFFF;

    for (size_t i = 0; i < length; i++)
    {
        crc ^= data[i];
        for (int j = 0; j < 8; j++)
        {
            if (crc & 0x0001)
            {
                crc = (crc >> 1) ^ 0xA001;
            }
            else
            {
                crc >>= 1;
            }
        }
    }

    return crc;
}

bool NPK::validateResponse(const uint8_t *rx_buffer, size_t length)
{
    size_t byte_count, expected_len = 0;
    uint16_t calculated_crc, received_crc = 0;

    if (rx_buffer[0] != DEV_ADDR || rx_buffer[1] != FUNC_CODE) {
        g_logger.warning("Invalid response header: addr=0x%02X func=0x%02X",
                         rx_buffer[0], rx_buffer[1]);
        return false;
    }

    // Get byte count and verify length
    byte_count = rx_buffer[2];
    expected_len = MODBUS_HEADER_SIZE + byte_count + MODBUS_CRC_SIZE; // header + data + CRC

    if (length != expected_len)
    {
        g_logger.warning("Expected %zu bytes, got %zu", expected_len, length);
        return false;
    }

    // Verify CRC
    calculated_crc = calculateCRC16(rx_buffer, length - MODBUS_CRC_SIZE);
    received_crc = static_cast<uint16_t>(rx_buffer[length - MODBUS_CRC_SIZE] | (rx_buffer[length - MODBUS_CRC_SIZE + 1] << 8));

    if (received_crc != calculated_crc)
    {
        g_logger.warning("CRC mismatch: received=0x%04X calculated=0x%04X",
                         received_crc, calculated_crc);
        return false;
    }

    return true;
}

uint16_t NPK::parseRegisterValue(const uint8_t *rx_buffer, size_t register_offset)
{
    std::string hexStr = Utils::bytesToHexString(rx_buffer[register_offset], rx_buffer[register_offset + 1]);
    return Utils::hexStringToInt(hexStr);
}


bool NPK::npk_collect(const MeasurementEntry &m_entry, uint16_t reading[NPK_COLLECT_SIZE])
{
    static_assert(NPK_COLLECT_SIZE == 5, "NPK_COLLECT_SIZE must be 35");

    uint8_t rx_buffer[RX_BUFFER_SIZE];
    size_t len = 0;
    uint16_t raw_value = 0;

    g_logger.info("Starting collection of %d readings for measurement type %d",
                  NPK_COLLECT_SIZE, static_cast<int>(m_entry.type));

    for (size_t sample = 0; sample < NPK_COLLECT_SIZE; sample++)
    {
        memset(rx_buffer, 0, RX_BUFFER_SIZE);

        sendModbusRequest(m_entry.packet, m_entry.packet_size);

        len = readModbusResponse(rx_buffer, RX_BUFFER_SIZE, 500);

        if (len != MODBUS_HEADER_SIZE + MODBUS_CRC_SIZE + MODBUS_PAYLOAD_SIZE)
        {
            g_logger.warning("No data received from RS485 NPK sensor on sample %zu", sample);
            return false;
        }

        // Validate response
        if (!validateResponse(rx_buffer, len))
        {
            g_logger.warning("Validation failed on sample %zu", sample);
            return false;
        }

        raw_value = parseRegisterValue(rx_buffer, m_entry.offset);

        printf("%d", raw_value);
        reading[sample] = raw_value;
        // Small delay between readings to avoid overwhelming the sensor
        vTaskDelay(pdMS_TO_TICKS(50));
        printf("\n");
    }

    g_logger.info("Completed collection of %d readings for measurement type %d",
                  NPK_COLLECT_SIZE, static_cast<int>(m_entry.type));

    return true;
}

void NPK::npk_calib()
{
    g_logger.info("Calibration routine not implemented yet");
}