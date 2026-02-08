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

    // Wait for initial response
    // vTaskDelay(pdMS_TO_TICKS(100));

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
    uint8_t byte_count = 0;
    size_t expected_len = 0;
    uint16_t calculated_crc = 0;
    uint16_t received_crc = 0;

    // Verify minimum length
    if (length < 5)
    {
        g_logger.warning("Response too short: %zu bytes", length);
        return false;
    }

    // Check slave address and function code
    if (rx_buffer[0] != 0x01 || rx_buffer[1] != 0x03)
    {
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

uint16_t NPK::extractMeasurement(const uint8_t *rx_buffer, MeasurementType type)
{
    // Map measurement type to register offset (for READ_ALL_SENSORS response)
    size_t offset = 0;

    switch (type)
    {
    case MeasurementType::Moisture:
        offset = 0;
        break; // Register 0x0000
    case MeasurementType::Temperature:
        offset = 1;
        break; // Register 0x0001
    // Conductivity at offset 2 (not used in table)
    case MeasurementType::PH:
        offset = 3;
        break; // Register 0x0003
    case MeasurementType::Nitrogen:
        offset = 4;
        break; // Register 0x0004
    case MeasurementType::Phosphorus:
        offset = 5;
        break; // Register 0x0005
    case MeasurementType::Potassium:
        offset = 6;
        break; // Register 0x0006
    default:
        return 0;
    }

    offset *= 2;
    offset += 3;

    return parseRegisterValue(rx_buffer, offset);
}

bool NPK::npk_collect(const MeasurementEntry &m_entry, uint8_t reading[NPK_COLLECT_SIZE])
{
    printf("Running NPK collect\n");
    // Compile-time check that NPK_COLLECT_SIZE is correct
    static_assert(NPK_COLLECT_SIZE == 5, "NPK_COLLECT_SIZE must be 35");

    uint8_t rx_buffer[RX_BUFFER_SIZE];
    size_t len = 0;
    float value = 0.0f;
    uint16_t raw_value = 0;

    g_logger.info("Starting collection of %d readings for measurement type %d",
                  NPK_COLLECT_SIZE, static_cast<int>(m_entry.type));

    static int x = 0;
    printf("Type number: %d\n", x);

    for (size_t sample = 0; sample < NPK_COLLECT_SIZE; sample++)
    {
        memset(rx_buffer, 0, RX_BUFFER_SIZE);

        sendModbusRequest(m_entry.packet, m_entry.packet_size);

        len = readModbusResponse(rx_buffer, RX_BUFFER_SIZE, 500);

        // check buffer len is greater than 2.
        // if it is: size_t num_bytes = rx_buffer[2]
        // check num_bytes == 2
        // read num_bytes forward from 2. num_bytes should be 2

        if (len != 7)
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

        // Parse the single register value (for single-register queries)
        raw_value = extractMeasurement(rx_buffer, m_entry.type);

        printf("%d\n", raw_value);
        // Small delay between readings to avoid overwhelming the sensor
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    g_logger.info("Completed collection of %d readings for measurement type %d",
                  NPK_COLLECT_SIZE, static_cast<int>(m_entry.type));

    return true;
}

void NPK::npk_calib()
{
    g_logger.info("Calibration routine not implemented yet");
}