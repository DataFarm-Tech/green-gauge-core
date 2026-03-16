#pragma once

#include <memory>
#include "IConnection.hpp"

/**
 * @enum ConnectionType
 * @brief Defines available connection types for communication.
 *
 * Used to specify which network interface should be initialized
 * for data transmission.
 */
enum class ConnectionType : uint8_t {
    WIFI,      ///< Wi-Fi connection
    SIM,       ///< Cellular (SIM-based) connection
    ETHERNET   ///< Ethernet (wired) connection
};

/**
 * @class Communication
 * @brief Manages communication setup and control for multiple connection types.
 *
 * The Communication class provides a high-level abstraction for initializing
 * and managing different types of network connections (e.g., Wi-Fi, SIM, Ethernet).
 * It internally holds a polymorphic IConnection object corresponding to the selected
 * connection type.
 */
class Communication {
public:
    /**
     * @brief Constructs a Communication object for a given connection type.
     *
     * @param type The desired connection type (e.g., ConnectionType::WIFI).
     *
     * This constructor creates and stores the appropriate IConnection instance
     * based on the selected connection type.
     */
    Communication(ConnectionType selected_type);

    ConnectionType getType() const;

    /**
     * @brief Establishes the network connection.
     *
     * @return True if the connection was successfully established, false otherwise.
     *
     * Attempts to connect using the configured IConnection implementation
     * (e.g., Wi-Fi, cellular, or Ethernet).
     */
    bool connect();

    /**
     * @brief Checks whether the connection is currently active.
     *
     * @return True if the connection is established and operational, false otherwise.
     */
    bool isConnected();

    /**
     * @brief Disconnects the active network connection.
     *
     * Ensures a clean shutdown of the underlying connection interface and
     * releases any associated network resources.
     */
    void disconnect();

    /**
     * @brief Starts a telnet session for remote debugging and command execution.
     * @return true if the telnet session was started successfully, false otherwise
     */
    bool startTelnetSession();

    /**
     * @brief Sends a CBOR-encoded packet using the active connection.
     * @param cbor_buffer Pointer to the CBOR-encoded data buffer.
     * @param cbor_buffer_len Length of the CBOR data buffer in bytes.
     * @param pkt_config Configuration for the packet type being sent.
     * @param response Optional pointer to a string to receive the response from the server.
     * @return true if the packet was sent successfully and a response was received (if response is not nullptr), false otherwise.
     */
    bool sendPacket(const uint8_t * cbor_buffer, const size_t cbor_buffer_len, const PktEntry_t pkt_config);

    /**
     * @brief Sends a CBOR-encoded packet using the active connection and captures the response.
     * @param cbor_buffer Pointer to the CBOR-encoded data buffer.
     * @param cbor_buffer_len Length of the CBOR data buffer in bytes.
     * @param pkt_config Configuration for the packet type being sent.
     * @param response Reference to a string to receive the response from the server.
     * @return true if the packet was sent successfully and a response was received, false otherwise.
     */
    bool sendPacket(const uint8_t * cbor_buffer, const size_t cbor_buffer_len, const PktEntry_t pkt_config, std::string &response);

    /**
     * @brief Sends a CBOR-encoded packet using the active connection in a streaming manner.
     * @param cbor_buffer Pointer to the CBOR-encoded data buffer.
     * @param cbor_buffer_len Length of the CBOR data buffer in bytes.
     * @param pkt_config Configuration for the packet type being sent.
     * @param onChunk Callback function invoked for each chunk of data received.
     * @return true if the packet was sent successfully and all chunks were received, false otherwise.
     */
    bool sendPacketStream(const uint8_t * cbor_buffer,
                          const size_t cbor_buffer_len,
                          const PktEntry_t pkt_config,
                          const PacketChunkCallback& onChunk);

    bool streamHttpsGet(const std::string& url,
                        const PacketChunkCallback& onChunk);

private:
    ConnectionType connection_type;
    std::unique_ptr<IConnection> connection;  ///< Smart pointer to the active connection implementation.
};
