#pragma once

#include "IConnection.hpp"
#include "ATCommandHndlr.hpp"
#include "CoapPktAssm.hpp"
/**
 * @enum SimStatus
 * @brief Represents the current status of the SIM connection.
 */
enum class SimStatus : uint8_t {
    DISCONNECTED,    // Modem off / UART not init
    INIT,            // UART init, modem ping
    NOTREADY,        // SIM not ready (CPIN != READY)
    REGISTERING,     // SIM ready, waiting for network
    CONNECTED,       // Registered on network
    ERROR            // Fatal / unrecoverable
};


/**
 * @class SimConnection
 * @brief Implements a cellular SIM-based network connection.
 *
 * Provides methods to connect, check connection status, and disconnect
 * from a SIM-based cellular network.
 */
class SimConnection : public IConnection {
public:
    /**
     * @brief Establishes a connection to the SIM network.
     *
     * @return true if the connection was successful, false otherwise.
     */
    bool connect() override;

    /**
     * @brief Checks if the SIM network connection is currently active.
     *
     * @return true if connected, false otherwise.
     */
    bool isConnected() override;

    /**
     * @brief Disconnects from the SIM network.
     */
    void disconnect() override;

    /**
     * @brief Sends packet
     */
    bool sendPacket(const uint8_t * cbor_buffer, const size_t cbor_buffer_len, const PktEntry_t pkt_config) override;

private:
    SimStatus sim_stat = SimStatus::DISCONNECTED;  // Default value
    static constexpr size_t RETRIES = 5;
    /**
     * @brief Closes COAP Session
     */
    void closeCoAPSession();
    void deactivatePDP();
    void closeUDPSocket();

    ATCommandHndlr hndlr;
};
