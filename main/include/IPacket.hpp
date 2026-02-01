#pragma once

#include <string>
#include <cstdint>
#include <sys/socket.h>
#include "coap3/coap.h"
#include <cbor.h>
#include "esp_log.h"
#include <cstddef>
#include "Types.hpp"

/**
 * @class IPacket
 * @brief Abstract base class for packets used in CoAP communication.
 *
 * Provides an interface for creating, serializing, and sending packets
 * over a CoAP network. Derived classes should implement the serialization
 * method to convert packet data into a buffer.
 */
class IPacket {
protected:
    std::string nodeId;        ///< Unique identifier for the node sending the packet
    size_t bufferLength = 0;   ///< Length of the serialized packet buffer
    std::string uri;           ///< URI endpoint for the CoAP packet

    /// Internal buffer to store serialized packet data
    uint8_t buffer[GEN_BUFFER_SIZE];

public:
    /**
     * @brief Virtual destructor.
     *
     * Ensures proper cleanup of derived packet classes.
     */
    virtual ~IPacket() = default;

    /**
     * @brief Serializes the packet into a buffer.
     *
     * Must be implemented by derived classes to provide the packet
     * data in a byte buffer suitable for sending over the network.
     *
     * @return Pointer to the serialized buffer.
     */
    virtual const uint8_t * toBuffer() = 0;

    /**
     * @brief Gets the length of the serialized packet buffer.
     *
     * @return Size of the buffer in bytes.
     */
    size_t getBufferLength() const { return bufferLength; }
};