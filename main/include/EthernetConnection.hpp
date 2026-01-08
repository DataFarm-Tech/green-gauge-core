#pragma once

#include "IConnection.hpp"

/**
 * @class EthernetConnection
 * @brief Implements a network connection using a wired Ethernet interface.
 *
 * EthernetConnection provides the concrete implementation of the IConnection
 * interface for establishing, monitoring, and terminating Ethernet connections.
 */
class EthernetConnection : public IConnection {
public:
    /**
     * @brief Establishes an Ethernet network connection.
     *
     * @return True if the connection was successfully established, false otherwise.
     *
     * Implements the connect() method from the IConnection interface
     * for Ethernet-specific connection initialization.
     */
    bool connect() override;

    /**
     * @brief Checks whether the Ethernet connection is currently active.
     *
     * @return True if the Ethernet connection is established and operational, false otherwise.
     *
     * Overrides the isConnected() method from IConnection.
     */
    bool isConnected() override;

    /**
     * @brief Terminates the Ethernet network connection.
     *
     * Performs a clean disconnection from the network and releases
     * any Ethernet-specific resources.
     *
     * Overrides the disconnect() method from IConnection.
     */
    void disconnect() override;
};
