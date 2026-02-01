#pragma once

#include <string>
#include <cstdint>
#include <sys/socket.h>
#include "coap3/coap.h"
#include <cbor.h>
#include "esp_log.h"
#include <cstddef>
#include "Types.hpp"
#include "Coap.hpp"

/**
 * @class IPacket
 * @brief Abstract base class for packets used in CoAP communication.
 *
 * Provides an interface for creating, serializing, and sending packets
 * over a CoAP network. Derived classes should implement the serialization
 * method to convert packet data into a buffer.
 */
class IPacket
{
protected:
    PktType pkt_type;
    std::string node_id;     ///< Unique identifier for the node sending the packet
    std::string uri;         ///< URI endpoint for the CoAP packet
    size_t bufferLength = 0; ///< Length of the serialized packet buffer
    uint8_t buffer[GEN_BUFFER_SIZE];

public:
    /**
     * @brief Virtual destructor.
     *
     * Ensures proper cleanup of derived packet classes.
     */
    IPacket(PktType _pkt_type, std::string _node_id, std::string _uri) : pkt_type(_pkt_type), node_id(_node_id), uri(_uri)
    {
    }

    /**
     * @brief Serializes the packet into a buffer.
     *
     * Must be implemented by derived classes to provide the packet
     * data in a byte buffer suitable for sending over the network.
     *
     * @return Pointer to the serialized buffer.
     */
    virtual const uint8_t *toBuffer() = 0;

    /**
     * @brief Gets the length of the serialized packet buffer.
     *
     * @return Size of the buffer in bytes.
     */
    size_t getBufferLength() const { return bufferLength; }
};