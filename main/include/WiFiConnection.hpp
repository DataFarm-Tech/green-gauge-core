#pragma once

#include "IConnection.hpp"
#include "esp_wifi.h"

/**
 * @class WifiConnection
 * @brief Implements a Wi-Fi network connection.
 *
 * Provides methods to connect to a Wi-Fi network, check the connection status,
 * and disconnect from the network.
 */
class WifiConnection : public IConnection {
private:
    EventGroupHandle_t wifi_event_group;

    /**
     * @brief Handles Wi-Fi and IP events.
     *
     * This static callback function is registered with the ESP-IDF event loop to handle
     * Wi-Fi and IP-related events. It sets the appropriate bits in the event group to
     * signal connection status changes.
     *
     * @param arg Pointer to the WifiConnection instance (used to access member variables).
     * @param event_base The event base (e.g., WIFI_EVENT, IP_EVENT).
     * @param event_id The specific event ID (e.g., WIFI_EVENT_STA_START, IP_EVENT_STA_GOT_IP).
     * @param event_data Pointer to event-specific data (may be nullptr for some events).
     */
    static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

public:
    /**
     * @brief Establishes a connection to the Wi-Fi network.
     *
     * @return true if the connection was successful, false otherwise.
     */
    bool connect() override;

    /**
     * @brief Checks if the Wi-Fi connection is currently active.
     *
     * @return true if connected, false otherwise.
     */
    bool isConnected() override;

    /**
     * @brief Disconnects from the Wi-Fi network.
     */
    void disconnect() override;
};
