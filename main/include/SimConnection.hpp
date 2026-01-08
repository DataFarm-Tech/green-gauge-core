#pragma once

#include "IConnection.hpp"

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
};
