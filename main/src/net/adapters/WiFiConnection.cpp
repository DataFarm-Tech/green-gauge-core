#include "WiFiConnection.hpp"
#include "esp_wifi.h"
#include "esp_log.h"
#include "freertos/event_groups.h"
#include "lwip/inet.h"
#include "lwip/ip4_addr.h"
#include "esp_netif_ip_addr.h"
#include "Logger.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

#define WIFI_SSID "NETGEAR77"
#define WIFI_PASS "aquaticcarrot628"

void WifiConnection::wifi_event_handler(void * arg, esp_event_base_t event_base, int32_t event_id, void * event_data) 
{
    WifiConnection * self = static_cast<WifiConnection*>(arg);

    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                esp_wifi_connect();
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGW("WIFI", "Disconnected, retrying...");
                esp_wifi_connect();
                break;
        
        default:
            break;
        }
    }
    else if (event_base == IP_EVENT) {
        if (event_id == IP_EVENT_STA_GOT_IP) {
                xEventGroupSetBits(self->wifi_event_group, BIT0);
                ESP_LOGI("WIFI", "Got IP address");
            }
    }
}

bool WifiConnection::connect() 
{
    wifi_event_group = xEventGroupCreate();

    esp_netif_init();
    esp_event_loop_create_default();
    
    esp_netif_t * netif = esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &WifiConnection::wifi_event_handler, this);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &WifiConnection::wifi_event_handler, this);

    wifi_config_t wifi_config = {};
    strncpy((char*)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, WIFI_PASS, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI("WIFI", "Connecting to SSID: %s", WIFI_SSID);

    EventBits_t bits = xEventGroupWaitBits(
        wifi_event_group,
        BIT0,
        pdFALSE,
        pdTRUE,
        pdMS_TO_TICKS(20000)  // wait 20 seconds max
    );

    bool connected = bits & BIT0;
    if (connected)
        ESP_LOGI("WIFI", "Connected successfully");
    else
        ESP_LOGE("WIFI", "Connection timed out");

    return connected;
}

bool WifiConnection::isConnected() 
{
    wifi_ap_record_t info;
    return esp_wifi_sta_get_ap_info(&info) == ESP_OK;
}

void WifiConnection::disconnect() 
{
    esp_wifi_disconnect();
    esp_wifi_stop();
    esp_wifi_deinit();
}

bool WifiConnection::sendPacket(const uint8_t * cbor_buffer, const size_t cbor_buffer_len, PktType pkt_type, const CoapMethod meth) {
    /**
     * Build CBOR Packet with pkt from NPK readings
     * Send packet using SOCKET
     */

    uint8_t coap_buffer[512];
    size_t coap_buffer_len = 0;
    int sock_fd = -1;
    struct sockaddr_in dest_addr;

    g_logger.info("Building CoAP packet from CBOR payload (%zu bytes)\n", cbor_buffer_len);


    coap_buffer_len = CoapPktAssm::buildCoapBuffer(coap_buffer, pkt_type, cbor_buffer, cbor_buffer_len, meth);


    if (coap_buffer_len == 0) {
        return false;
    }

    sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock_fd < 0) {
        g_logger.error("Failed to create socket\n");
        return false;
    }

    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(5683);
    inet_pton(AF_INET, "45.79.239.100", &dest_addr.sin_addr);


    // --- Send the packet ---
    int err = sendto(sock_fd, coap_buffer, coap_buffer_len, 0,
                     (struct sockaddr*)&dest_addr, sizeof(dest_addr));
    if (err < 0)
    {
        ESP_LOGE("WifiConnection", "Failed to send UDP packet: errno=%d", errno);
        close(sock_fd);
        return false;
    }

    ESP_LOGI("WifiConnection", "Sent %zu bytes to 45.79.239.100", coap_buffer_len);

    close(sock_fd);

    return false;
}
