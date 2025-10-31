#include "WiFiConnection.hpp"
#include "esp_wifi.h"
#include "esp_log.h"
#include "freertos/event_groups.h"

#define WIFI_SSID "NETGEAR77"
#define WIFI_PASS "aquaticcarrot628"

static const char* TAG = "Wifi";
static EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;

extern "C" void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) 
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) 
    {
        esp_wifi_connect();
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) 
    {
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
    }
}

bool WifiConnection::connect() {
    ESP_LOGI(TAG, "Connecting to Wi-Fi...");

    wifi_event_group = xEventGroupCreate();

    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, nullptr);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, nullptr);

    wifi_config_t wifi_config = {};
    strncpy((char*)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, WIFI_PASS, sizeof(wifi_config.sta.password));

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    EventBits_t bits = xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
                                           pdFALSE, pdTRUE, pdMS_TO_TICKS(10000));
    return bits & CONNECTED_BIT;
}

bool WifiConnection::isConnected() {
    wifi_ap_record_t info;
    return esp_wifi_sta_get_ap_info(&info) == ESP_OK;
}


void WifiConnection::disconnect() {
    ESP_LOGI(TAG, "Disconnecting Wi-Fi...");
    esp_wifi_disconnect();
    esp_wifi_stop();
    esp_wifi_deinit();
}