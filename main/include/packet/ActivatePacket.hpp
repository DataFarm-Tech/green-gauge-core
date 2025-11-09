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
class ActivatePacket : public IPacket {
private:
    std::string TAG;
    static constexpr size_t HMAC_KEY_SIZE = 32;
    
    const uint8_t secretKey[HMAC_KEY_SIZE] = {
        0x12, 0xA4, 0x56, 0xB7, 0x8C, 0x91, 0xDE, 0xF3,
        0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x01, 0x23,
        0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0x12,
        0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0x12
    };


    void computeKey(uint8_t * out_hmac) const;

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
    ActivatePacket(const std::string& id, const std::string& uri_endpoint, 
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
