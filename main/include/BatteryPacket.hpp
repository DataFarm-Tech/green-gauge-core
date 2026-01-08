#pragma once

#include <string>
#include "IPacket.hpp"

#include <cbor.h>
#include <sys/socket.h>
#include "esp_log.h"
#include <cstdint>
#include <cstddef>

/**
 * @class BatteryPacket
 * @brief Represents a battery data packet that can be serialized to CBOR and sent via CoAP.
 *
 * The BatteryPacket class extends the IPacket interface and provides functionality for reading
 * battery data (level, health) and encoding it into CBOR format before transmission over CoAP.
 */
class BatteryPacket : public IPacket {
private:
    uint8_t level;   ///< Current battery level percentage (0–100)
    uint8_t health;  ///< Battery health percentage (0–100)
    std::string TAG;

public:
    /**
     * @brief Constructs a BatteryPacket instance.
     *
     * @param id Unique node identifier (e.g., device ID or serial number).
     * @param uri_endpoint The target CoAP endpoint URI (e.g., "coap://192.168.1.100/sensor").
     * @param lvl Initial battery level value (optional, defaults to 0).
     * @param hlth Initial battery health value (optional, defaults to 0).
     *
     * The constructor sets up identifying and connection parameters for the packet.
     */
    BatteryPacket(const std::string& id, const std::string& uri_endpoint, uint8_t lvl = 0, uint8_t hlth = 0, const std::string& tag = "BatteryPacket")
    : level(lvl), health(hlth), TAG(tag)
    {
        nodeId = id;
        uri = uri_endpoint;
    }

    /**
     * @brief Converts the current BatteryPacket data to a CBOR-encoded byte buffer.
     *
     * @return Pointer to the encoded CBOR buffer, or nullptr if encoding fails.
     *
     * This method encodes the node ID, battery level, and battery health into a CBOR payload
     * suitable for transmission over a CoAP connection.
     */
    const uint8_t * toBuffer() override;

    /**
     * @brief Reads battery information from the Battery Management System (BMS).
     *
     * @return True if data was successfully read, false otherwise.
     *
     * Currently, this function simulates reading battery data but can be extended to support
     * actual hardware interfaces such as UART, I2C, or CAN bus communication.
     */
    bool readFromBMS();
};
