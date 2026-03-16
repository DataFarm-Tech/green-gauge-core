#pragma once

#include <unistd.h>
#include <string>
#include <functional>
#include "CoapPktAssm.hpp"

using PacketChunkCallback = std::function<bool(const uint8_t*, size_t)>;

/**
 * @class IConnection
 * @brief Interface for network connection implementations.
 *
 * IConnection defines a common interface for different types of network
 * connections (Ethernet, WiFi, SIM, etc.) providing methods to connect,
 * check connection status, and disconnect.
 */
class IConnection {
public:
    /**
     * @brief Virtual destructor.
     *
     * Ensures proper cleanup of derived connection classes.
     */
    virtual ~IConnection() = default;

    /**
     * @brief Establishes the network connection.
     *
     * @return True if the connection was successfully established, false otherwise.
     *
     * Must be implemented by derived classes for the specific connection type.
     */
    virtual bool connect() = 0;

    /**
     * @brief Checks whether the connection is currently active.
     *
     * @return True if the connection is established and operational, false otherwise.
     *
     * Must be implemented by derived classes.
     */
    virtual bool isConnected() = 0;

    /**
     * @brief Terminates the network connection.
     *
     * Performs any cleanup required to safely disconnect from the network.
     * Must be implemented by derived classes.
     */
    virtual void disconnect() = 0;

    virtual bool startTelnetSession() {
        return true;
    }

    virtual bool sendPacket(const uint8_t * cbor_buffer,
                            const size_t cbor_buffer_len,
                            const PktEntry_t pkt_config,
                            std::string* response = nullptr) = 0;

    virtual bool sendPacketStream(const uint8_t * cbor_buffer,
                                  const size_t cbor_buffer_len,
                                  const PktEntry_t pkt_config,
                                  const PacketChunkCallback& onChunk)
    {
        std::string response;
        if (!sendPacket(cbor_buffer, cbor_buffer_len, pkt_config, &response))
        {
            return false;
        }

        if (!onChunk)
        {
            return true;
        }

        return onChunk(reinterpret_cast<const uint8_t*>(response.data()), response.size());
    }

    virtual bool streamHttpsGet(const std::string& url,
                                const PacketChunkCallback& onChunk)
    {
        (void)url;
        (void)onChunk;
        return false;
    }
};
