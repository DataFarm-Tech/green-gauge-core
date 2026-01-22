#pragma once
#include "ping/ping_sock.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <string>

/**
 * @class Ping
 * @brief Simple ping utility for ESP-IDF
 * 
 * Provides synchronous ping functionality with configurable count and timeout.
 */
class Ping {
public:
    /**
     * @brief Construct a new Ping object
     */
    Ping();
    
    /**
     * @brief Destroy the Ping object
     */
    ~Ping();
    
    /**
     * @brief Ping a host (blocking call)
     * 
     * @param host Hostname or IP address to ping (e.g., "8.8.8.8" or "google.com")
     * @param count Number of ping packets to send (default: 4)
     * @param timeout_ms Timeout per ping in milliseconds (default: 1000)
     * @param wait_timeout_sec Maximum time to wait for all pings to complete (default: 30)
     * @return esp_err_t ESP_OK on success, ESP_FAIL on DNS failure, ESP_ERR_TIMEOUT on timeout
     */
    esp_err_t ping(const char* host, 
                   uint32_t count = 4, 
                   uint32_t timeout_ms = 1000,
                   uint32_t wait_timeout_sec = 30);

private:
    /**
     * @brief Callback for successful ping response
     */
    static void onPingSuccess(esp_ping_handle_t hdl, void *args);
    
    /**
     * @brief Callback for ping timeout
     */
    static void onPingTimeout(esp_ping_handle_t hdl, void *args);
    
    /**
     * @brief Callback for ping session end
     */
    static void onPingEnd(esp_ping_handle_t hdl, void *args);
    
    /**
     * @brief Resolve hostname to IP address
     * 
     * @param host Hostname or IP address
     * @param target_addr Output IP address
     * @return esp_err_t ESP_OK on success, ESP_FAIL on failure
     */
    esp_err_t resolveHost(const char* host, ip_addr_t& target_addr);
    
    EventGroupHandle_t m_event_group;
    static const int PING_DONE_BIT = BIT0;
    static const char* TAG;
};