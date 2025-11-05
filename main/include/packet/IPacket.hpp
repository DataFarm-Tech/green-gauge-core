#pragma once

#include <string>
#include <cstdint>
#include "coap3/coap.h"
#include <cbor.h>
#include <sys/socket.h>
#include "esp_log.h"
#include <cstddef>

/**
 * @enum CoAPResponseType
 * @brief Defines available CoAP response types
 *
 * Used to specify which network interface should be initialized
 * for data transmission.
 */
enum class CoAPResponseType : uint8_t {
    COAP_RESPONSE = 2,
    COAP_RESPONSE_CLIENT_ERROR = 4,
    COAP_RESPONSE_SERVER_ERROR = 5
};

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

    /// Default buffer size for packet serialization
    static constexpr size_t BUFFER_SIZE = 1024;

    /// Internal buffer to store serialized packet data
    uint8_t buffer[BUFFER_SIZE];

    /**
     * @brief CoAP message handler callback.
     *
     * Handles CoAP responses for a sent message.
     *
     * @param session Pointer to the CoAP session.
     * @param sent Pointer to the sent PDU.
     * @param received Pointer to the received PDU.
     * @param mid Message ID of the CoAP message.
     * @return coap_response_t indicating how the response was handled.
     */
    static coap_response_t message_handler(
        coap_session_t * session, 
        const coap_pdu_t * sent, 
        const coap_pdu_t * received, 
        const coap_mid_t mid
    );

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
     * @brief Sends the packet over the network.
     *
     * Uses the CoAP protocol to transmit the serialized packet to
     * the URI endpoint specified in the packet.
     */
    void sendPacket();

    /**
     * @brief Gets the length of the serialized packet buffer.
     *
     * @return Size of the buffer in bytes.
     */
    size_t getBufferLength() const { return bufferLength; }
};
