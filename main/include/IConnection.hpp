#pragma once

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
};
