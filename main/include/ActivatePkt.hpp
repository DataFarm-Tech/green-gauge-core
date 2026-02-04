#pragma once

#include <sys/socket.h>
#include <string>
#include "IPacket.hpp"

#include <cbor.h>
#include "esp_log.h"
#include <cstdint>
#include <cstddef>
#include "GPS.hpp"

/**
 * @class BatteryPacket
 * @brief Represents a battery data packet that can be serialized to CBOR and sent via CoAP.
 *
 * The BatteryPacket class extends the IPacket interface and provides functionality for reading
 * battery data (level, health) and encoding it into CBOR format before transmission over CoAP.
 */
class ActivatePkt : public IPacket
{
private:
    std::string secretKeyUser;
    std::string GPSCoord;
    static constexpr size_t HMAC_KEY_SIZE = 32;

    const uint8_t secretKey[HMAC_KEY_SIZE] = {
        0x12, 0xA4, 0x56, 0xB7, 0x8C, 0x91, 0xDE, 0xF3,
        0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x01, 0x23,
        0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0x12,
        0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0x12};

    void computeKey(uint8_t *out_hmac) const;

public:
    ActivatePkt(PktType _pkt_type, std::string _node_id, std::string _uri, std::string _secret_key, std::string _gps_coord)
        : IPacket(_pkt_type, _node_id, _uri), secretKeyUser(_secret_key), GPSCoord(_gps_coord)
    {
    }

    /**
     * @brief Converts the current BatteryPacket data to a CBOR-encoded byte buffer.
     *
     * @return Pointer to the encoded CBOR buffer, or nullptr if encoding fails.
     *
     * This method encodes the node ID, battery level, and battery health into a CBOR payload
     * suitable for transmission over a CoAP connection.
     */
    const uint8_t *toBuffer() override;
};