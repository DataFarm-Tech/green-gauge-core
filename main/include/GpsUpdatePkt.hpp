#pragma once

#include <sys/socket.h>
#include <string>
#include "IPacket.hpp"

#include <cbor.h>
#include "esp_log.h"
#include <cstdint>
#include <cstddef>
#include "GPS.hpp"
#include "Key.hpp"

/**
 * @class GpsUpdatePkt
 * @brief Represents a GPS update data packet that can be serialized to CBOR and sent via CoAP.
 *
 * The GpsUpdatePkt class extends the IPacket interface and provides functionality for reading
 * GPS coordinates and encoding them into CBOR format before transmission over CoAP.
 */
class GpsUpdatePkt : public IPacket
{
private:
    std::string GPSCoord;

    static constexpr const char* NODE_ID_KEY = "node_id";
    static constexpr const char* GPS_KEY = "gps";
    static constexpr const char* SECRET_KEY_KEY = "secretkey";
    static constexpr const char* KEY_KEY = "key";


public:
    GpsUpdatePkt(PktType _pkt_type, std::string _node_id, std::string _uri, std::string _gps_coord)
        : IPacket(_pkt_type, _node_id, _uri), GPSCoord(_gps_coord)
    {
    }

    /**
     * @brief Converts the current GpsUpdatePkt data to a CBOR-encoded byte buffer.
     *
     * @return Pointer to the encoded CBOR buffer, or nullptr if encoding fails.
     *
     * This method encodes the node ID and GPS coordinates into a CBOR payload
     * suitable for transmission over a CoAP connection.
     */
    const uint8_t *toBuffer() override;
};