#pragma once

#include <sys/socket.h>
#include <string>
#include "IPacket.hpp"

#include <cbor.h>
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
class ActivatePkt : public IPacket {
private:
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
    ActivatePkt(const std::string& id, const std::string& uri_endpoint, 
        const std::string& tag = "ActivatePacket")
    : TAG(tag)
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
};