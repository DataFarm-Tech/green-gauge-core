#pragma once

#include <sys/socket.h>
#include <string>
#include "IPacket.hpp"
#include "NPK.hpp"
#include "CoapPktAssm.hpp"

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
class ReadingPkt : public IPacket
{
private:
    uint8_t reading[NPK_COLLECT_SIZE];
    MeasurementType m_type;
    const char* mTypeToString() const;


public:
    ReadingPkt(PktType _pkt_type, std::string _node_id, std::string _uri, uint8_t _reading[NPK_COLLECT_SIZE], MeasurementType _m_type)
        : IPacket(_pkt_type, _node_id, _uri), m_type(_m_type)
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