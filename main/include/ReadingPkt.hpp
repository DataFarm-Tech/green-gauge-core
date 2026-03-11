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
#include <cstring>  // For memcpy

class ReadingPkt : public IPacket
{
private:
    uint16_t reading[NPK_COLLECT_SIZE];  // ✅ Changed to uint16_t
    MeasurementType m_type;
    uint64_t session_count;
    const char* mTypeToString() const;

    static constexpr const char* NODE_ID_KEY = "node_id";
    static constexpr const char* KEY_KEY = "key";
    static constexpr const char* M_TYPE = "m_type";
    static constexpr const char* READINGS_ARR = "readings";
    static constexpr const char* SESSION = "session";

public:
    ReadingPkt(PktType _pkt_type, std::string _node_id, std::string _uri, uint16_t _reading[NPK_COLLECT_SIZE], MeasurementType _m_type, uint64_t _session_counter)
        : IPacket(_pkt_type, _node_id, _uri), m_type(_m_type), session_count(_session_counter)
    {
        // ✅ COPY the readings array!
        memcpy(this->reading, _reading, sizeof(uint16_t) * NPK_COLLECT_SIZE);
    }

    const uint8_t *toBuffer() override;
};