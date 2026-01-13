#pragma once

#include "IConnection.hpp"
#include "UARTDriver.hpp"

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
     * @brief Thread to handle incoming data from the SIM modem
     */
    void tick(void * arg);

private:    
    UARTDriver m_modem_uart { UART_NUM_1 };   // UART connected to Quectel
    SimStatus   m_status { SimStatus::DISCONNECTED };
    uint32_t    m_retry_count { 0 };



private:
    /**
     * @brief Send an AT command and wait for expected response.
     */
    bool sendAT(const char* cmd, const char* expect, int timeout_ms);
};
