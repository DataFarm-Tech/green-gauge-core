#include "Ping.hpp"
#include <string.h>
#include "lwip/netdb.h"
#include "lwip/inet.h"

const char* Ping::TAG = "Ping";

Ping::Ping() 
    : m_event_group(nullptr) {
}

Ping::~Ping() {
    if (m_event_group) {
        vEventGroupDelete(m_event_group);
    }
}

esp_err_t Ping::ping(const char* host, 
                     uint32_t count, 
                     uint32_t timeout_ms,
                     uint32_t wait_timeout_sec) {
    if (!host) {
        ESP_LOGE(TAG, "Invalid host parameter");
        return ESP_ERR_INVALID_ARG;
    }
    
    // Create event group for synchronization
    m_event_group = xEventGroupCreate();
    if (!m_event_group) {
        ESP_LOGE(TAG, "Failed to create event group");
        return ESP_ERR_NO_MEM;
    }
    
    // Resolve hostname to IP
    ip_addr_t target_addr;
    esp_err_t ret = resolveHost(host, target_addr);
    if (ret != ESP_OK) {
        vEventGroupDelete(m_event_group);
        m_event_group = nullptr;
        return ret;
    }
    
    // Configure ping
    esp_ping_config_t ping_config = ESP_PING_DEFAULT_CONFIG();
    ping_config.target_addr = target_addr;
    ping_config.count = count;
    ping_config.timeout_ms = timeout_ms;
    ping_config.interval_ms = 1000;
    ping_config.task_stack_size = 4096;
    ping_config.task_prio = 2;
    
    // Set callbacks
    esp_ping_callbacks_t cbs = {};
    cbs.on_ping_success = onPingSuccess;
    cbs.on_ping_timeout = onPingTimeout;
    cbs.on_ping_end = onPingEnd;
    cbs.cb_args = m_event_group;  // Pass event group to callbacks
    
    // Create ping session
    esp_ping_handle_t ping_handle;
    ret = esp_ping_new_session(&ping_config, &cbs, &ping_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create ping session: %s", esp_err_to_name(ret));
        vEventGroupDelete(m_event_group);
        m_event_group = nullptr;
        return ret;
    }
    
    ESP_LOGI(TAG, "Pinging %s...", host);
    
    // Start ping
    esp_ping_start(ping_handle);
    
    // Wait for ping to complete
    EventBits_t bits = xEventGroupWaitBits(
        m_event_group,
        PING_DONE_BIT,
        pdTRUE,
        pdFALSE,
        pdMS_TO_TICKS(wait_timeout_sec * 1000)
    );
    
    // Cleanup
    vEventGroupDelete(m_event_group);
    m_event_group = nullptr;
    
    if (!(bits & PING_DONE_BIT)) {
        ESP_LOGE(TAG, "Ping operation timed out");
        return ESP_ERR_TIMEOUT;
    }
    
    return ESP_OK;
}

void Ping::onPingSuccess(esp_ping_handle_t hdl, void *args) {
    uint8_t ttl;
    uint16_t seqno;
    uint32_t elapsed_time, recv_len;
    ip_addr_t target_addr;
    
    esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
    esp_ping_get_profile(hdl, ESP_PING_PROF_TTL, &ttl, sizeof(ttl));
    esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
    esp_ping_get_profile(hdl, ESP_PING_PROF_SIZE, &recv_len, sizeof(recv_len));
    esp_ping_get_profile(hdl, ESP_PING_PROF_TIMEGAP, &elapsed_time, sizeof(elapsed_time));
    
    ESP_LOGI(TAG, "%lu bytes from %s icmp_seq=%u ttl=%u time=%lu ms",
             (unsigned long)recv_len, ipaddr_ntoa(&target_addr), 
             seqno, ttl, (unsigned long)elapsed_time);
}

void Ping::onPingTimeout(esp_ping_handle_t hdl, void *args) {
    uint16_t seqno;
    ip_addr_t target_addr;
    
    esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
    esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
    
    ESP_LOGW(TAG, "From %s icmp_seq=%u timeout", ipaddr_ntoa(&target_addr), seqno);
}

void Ping::onPingEnd(esp_ping_handle_t hdl, void *args) {
    uint32_t transmitted;
    uint32_t received;
    uint32_t total_time_ms;
    
    esp_ping_get_profile(hdl, ESP_PING_PROF_REQUEST, &transmitted, sizeof(transmitted));
    esp_ping_get_profile(hdl, ESP_PING_PROF_REPLY, &received, sizeof(received));
    esp_ping_get_profile(hdl, ESP_PING_PROF_DURATION, &total_time_ms, sizeof(total_time_ms));
    
    ESP_LOGI(TAG, "%lu packets transmitted, %lu received, time %lums",
             (unsigned long)transmitted, (unsigned long)received, 
             (unsigned long)total_time_ms);
    
    if (transmitted > 0) {
        uint32_t loss = ((transmitted - received) * 100) / transmitted;
        ESP_LOGI(TAG, "Packet loss: %lu%%", (unsigned long)loss);
    }
    
    // Delete the ping session
    esp_ping_delete_session(hdl);
    
    // Signal completion
    EventGroupHandle_t event_group = (EventGroupHandle_t)args;
    if (event_group) {
        xEventGroupSetBits(event_group, PING_DONE_BIT);
    }
}

esp_err_t Ping::resolveHost(const char* host, ip_addr_t& target_addr) {
    struct addrinfo hints = {};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_RAW;
    
    struct addrinfo *res = NULL;
    int err = getaddrinfo(host, NULL, &hints, &res);
    if (err != 0 || res == NULL) {
        ESP_LOGE(TAG, "DNS lookup failed for %s: %d", host, err);
        return ESP_FAIL;
    }
    
    // Convert address
    if (res->ai_family == AF_INET) {
        struct in_addr addr4 = ((struct sockaddr_in *)(res->ai_addr))->sin_addr;
        inet_addr_to_ip4addr(ip_2_ip4(&target_addr), &addr4);
    } else {
        struct in6_addr addr6 = ((struct sockaddr_in6 *)(res->ai_addr))->sin6_addr;
        inet6_addr_to_ip6addr(ip_2_ip6(&target_addr), &addr6);
    }
    
    freeaddrinfo(res);
    return ESP_OK;
}