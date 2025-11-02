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
    Communication(ConnectionType type);

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

private:
    std::unique_ptr<IConnection> connection;  ///< Smart pointer to the active connection implementation.
};
