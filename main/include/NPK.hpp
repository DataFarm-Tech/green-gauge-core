#pragma once
#include <cstdint>
#include <cstring>
#include "UARTDriver.hpp"
#include "driver/gpio.h"
#include <vector>
#include <array>

#include "Config.hpp"

/**
 * @brief Modbus RTU packet structure and constants for NPK sensor communication
 * This section defines the structure of Modbus RTU packets used to communicate with the NPK sensor, including the sizes of various fields, the device address, function codes, and register offsets for each measurement type. The constants are used throughout the NPK class to construct requests and parse responses from the sensor.
 * The Modbus RTU protocol requires a specific packet format, including a device address, function code, data payload, and CRC16 checksum for error checking. The defined constants ensure that the packets are constructed correctly and that responses are validated properly.
 * The measurement offsets are calculated based on the order of the registers in the sensor's response, allowing for easy access to specific measurement values when parsing the response buffer.
 * Note: The CRC16 values in the request packets are pre-calculated for the specific commands being sent to the sensor. If the commands or register addresses change, the CRC16 values will need to be recalculated accordingly.
 * The NPK class uses these constants to implement functions for sending Modbus requests, reading responses, validating data, and extracting measurement values for Nitrogen, Phosphorus, Potassium, Moisture, pH, and Temperature from the sensor.
 * The defined constants also include the expected sizes of responses for single measurements and for reading all sensors at once, which are used to validate the responses received from the sensor.
 */
#define MODBUS_ADDR_SIZE 1       // Device address byte
#define MODBUS_FUNC_SIZE 1       // Function code byte
#define MODBUS_BYTE_COUNT_SIZE 1 // Byte count field size
#define MODBUS_CRC_SIZE 2        // CRC16 (Low + High bytes)
#define MODBUS_REGISTER_SIZE 2   // Each register is 2 bytes
#define MODBUS_HEADER_SIZE (MODBUS_ADDR_SIZE + MODBUS_FUNC_SIZE + MODBUS_BYTE_COUNT_SIZE)
#define MODBUS_PAYLOAD_SIZE 2
#define MODBUS_MIN_RESPONSE (MODBUS_HEADER_SIZE + MODBUS_CRC_SIZE) // 5 bytes minimum

/**
 * @brief The following constants define the positions of various fields in the Modbus RTU response packet, as well as the expected sizes of responses for single measurements and for reading all sensors at once. These constants are used to validate and parse the responses received from the NPK sensor.
 * The field positions are calculated based on the standard Modbus RTU response format, which includes a device address, function code, byte count, data payload, and CRC16 checksum.
 */
#define MODBUS_ADDR_POS 0
#define MODBUS_FUNC_POS 1
#define MODBUS_BYTE_COUNT_POS 2
#define MODBUS_DATA_START_POS 3

#define DEV_ADDR 0x01
#define FUNC_CODE 0x03

/**
 * @brief The following offsets are used to locate the specific measurement values in the Modbus response payload.
 * Each measurement corresponds to a specific register in the sensor, and the offsets are calculated based on the order of the registers in the response. The offsets are multiplied by 2 because each register is
 * 2 bytes long, and we also add 3 to account for the device address, function code, and byte count fields at the start of the response. For example, to access the Nitrogen value which is in the 4th register (offset 4), we calculate the byte position as (4 * 2) + 3 = 11 bytes into the response payload.
 * This allows us to directly access the correct bytes in the response buffer when parsing the sensor data.
 */
#define MOISTURE_OFFSET 0
#define TEMPER_OFFSET 1
// #define CONDUC_OFFSET 2 //NOT USED
#define PH_OFFSET 3
#define NITROGEN_OFFSET 4
#define PHOS_OFFSET 5
#define POT_OFFSET 6

/**
 * @brief Measurement types supported by the NPK sensor
 */
enum class MeasurementType
{
    Nitrogen,
    Phosphorus,
    Potassium,
    Moisture,
    PH,
    Temperature
};

// Measurement type name constants
constexpr const char *NITROGEN = "nitrogen";
constexpr const char *PHOSPHORUS = "phosphorus";
constexpr const char *POTASSIUM = "potassium";
constexpr const char *MOISTURE = "moisture";
constexpr const char *PH = "ph";
constexpr const char *TEMPERATURE = "temperature";

// Forward declaration
class ReadingPacket;

/**
 * @brief Class for handling NPK soil sensor operations via RS485/Modbus RTU
 *
 * This class handles communication with the CWT NPK soil sensor which measures:
 * - Nitrogen (N), Phosphorus (P), Potassium (K)
 * - Humidity/Moisture
 * - Temperature
 * - pH
 *
 * Default sensor settings:
 * - Device Address: 0x01
 * - Baud Rate: 4800
 * - Format: 8N1
 */
class NPK
{
public:
    // Packet size constants (must be defined before MEASUREMENT_TABLE)
    static constexpr size_t PACKET_SIZE = 8;
    static constexpr size_t RX_BUFFER_SIZE = 19;

    /**
     * @brief Table entry linking measurement type, name, and Modbus packet
     */
    struct MeasurementEntry
    {
        MeasurementType type;
        const uint8_t *packet;
        size_t packet_size;
        size_t offset;
    };

    /**
     * @brief Constructor for NPK class
     */
    NPK();

    /**
     * @brief Collect NPK readings and store them in the provided buffer
     * @param m_entry Measurement entry containing packet info
     * @param reading Output buffer to store the reading
     * @return true if successful, false otherwise
     */
    static bool npk_collect(const MeasurementEntry &m_entry, uint16_t reading[NPK_COLLECT_SIZE]);

    /**
     * @brief Calibrate the NPK sensor
     */
    void npk_calib();

private:
    /**
     * @brief Send a Modbus request packet
     * @param packet Pointer to packet data
     * @param packet_size Size of the packet
     */
    static void sendModbusRequest(const uint8_t *packet, size_t packet_size);

    /**
     * @brief Read Modbus response from sensor
     * @param rx_buffer Buffer to store response
     * @param buffer_size Maximum buffer size
     * @param timeout_ms Timeout in milliseconds
     * @return Number of bytes read
     */
    static size_t readModbusResponse(uint8_t *rx_buffer, size_t buffer_size, uint32_t timeout_ms);

    /**
     * @brief Validate Modbus response (address, function code, length, CRC)
     * @param rx_buffer Response buffer
     * @param length Length of response
     * @return true if valid, false otherwise
     */
    static bool validateResponse(const uint8_t *rx_buffer, size_t length);

    /**
     * @brief Calculate Modbus CRC16
     * @param data Data buffer
     * @param length Length of data
     * @return CRC16 value
     */
    static uint16_t calculateCRC16(const uint8_t *data, size_t length);

    /**
     * @brief Parse single register value from response
     * @param rx_buffer Response buffer
     * @param register_offset Offset of register in response (0-6 for the 7 registers)
     * @return Parsed 16-bit value
     */
    static uint16_t parseRegisterValue(const uint8_t *rx_buffer, size_t register_offset);

    /**
     * @brief Extract measurement value based on type
     * @param rx_buffer Response buffer containing all registers
     * @param type Measurement type to extract
     * @return Raw measurement value
     */
    static uint16_t extractMeasurement(const uint8_t *rx_buffer, MeasurementType type);

    /**
     * @brief Convert raw sensor value to float based on measurement type
     * @param raw_value Raw 16-bit value from sensor
     * @param type Measurement type
     * @return Converted float value
     */
    static float convertRawValue(uint16_t raw_value, MeasurementType type);

    // // Read single humidity value (Register 0x0000)
    // static constexpr uint8_t READ_HUMIDITY[PACKET_SIZE] = {
    //     0x01,       // Device Address
    //     0x03,       // Function Code (Read Holding Registers)
    //     0x00, 0x00, // Start Address (Humidity)
    //     0x00, 0x01, // Number of registers to read (1)
    //     0x84, 0x0A  // CRC16 (Low, High)
    // };

    // // Read single temperature value (Register 0x0001)
    // static constexpr uint8_t READ_TEMPERATURE[PACKET_SIZE] = {
    //     0x01,       // Device Address
    //     0x03,       // Function Code
    //     0x00, 0x01, // Start Address (Temperature)
    //     0x00, 0x01, // Number of registers
    //     0xD5, 0xCA  // CRC16
    // };

    // // Read single PH value (Register 0x0003)
    // static constexpr uint8_t READ_PH[PACKET_SIZE] = {
    //     0x01,       // Device Address
    //     0x03,       // Function Code
    //     0x00, 0x03, // Start Address (PH)
    //     0x00, 0x01, // Number of registers
    //     0x74, 0x0A  // CRC16
    // };

    // // Read single Nitrogen value (Register 0x0004)
    // static constexpr uint8_t READ_NITROGEN[PACKET_SIZE] = {
    //     0x01,       // Device Address
    //     0x03,       // Function Code
    //     0x00, 0x04, // Start Address (Nitrogen)
    //     0x00, 0x01, // Number of registers
    //     0xC5, 0xCB  // CRC16
    // };

    // // Read single Phosphorus value (Register 0x0005)
    // static constexpr uint8_t READ_PHOSPHORUS[PACKET_SIZE] = {
    //     0x01,       // Device Address
    //     0x03,       // Function Code
    //     0x00, 0x05, // Start Address (Phosphorus)
    //     0x00, 0x01, // Number of registers
    //     0x94, 0x0B  // CRC16
    // };

    // // Read single Potassium value (Register 0x0006)
    // static constexpr uint8_t READ_POTASSIUM[PACKET_SIZE] = {
    //     0x01,       // Device Address
    //     0x03,       // Function Code
    //     0x00, 0x06, // Start Address (Potassium)
    //     0x00, 0x01, // Number of registers
    //     0x64, 0x0B  // CRC16
    // };

    // Read ALL sensors at once (7 registers: Humidity through Potassium)
    static constexpr uint8_t READ_ALL_SENSORS[PACKET_SIZE] = {
        0x01,       // Device Address
        0x03,       // Function Code
        0x00, 0x00, // Start Address (Humidity)
        0x00, 0x07, // Number of registers (7)
        0x04, 0x08  // CRC16
    };

public:
    // Measurement table - must be defined AFTER the packet arrays
    static constexpr MeasurementEntry MEASUREMENT_TABLE[] = {
    {MeasurementType::Nitrogen,    READ_ALL_SENSORS, PACKET_SIZE, 11},  // Register 4: bytes 11-12
    {MeasurementType::Moisture,    READ_ALL_SENSORS, PACKET_SIZE, 3},   // Register 0: bytes 3-4
    {MeasurementType::PH,          READ_ALL_SENSORS, PACKET_SIZE, 9},   // Register 3: bytes 9-10
    {MeasurementType::Phosphorus,  READ_ALL_SENSORS, PACKET_SIZE, 13},  // Register 5: bytes 13-14
    {MeasurementType::Potassium,   READ_ALL_SENSORS, PACKET_SIZE, 15},  // Register 6: bytes 15-16
    {MeasurementType::Temperature, READ_ALL_SENSORS, PACKET_SIZE, 5}    // Register 1: bytes 5-6
};

    static constexpr size_t MEASUREMENT_TABLE_SIZE = sizeof(MEASUREMENT_TABLE) / sizeof(MeasurementEntry);
};