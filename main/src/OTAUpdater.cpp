#include "ota/OTAUpdater.hpp"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include <string.h>

static const char *TAG = "OTA";

static esp_err_t http_events(esp_http_client_event_t *evt) {
    switch (evt->event_id) {
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI(TAG, "Connected");
            break;
        case HTTP_EVENT_ON_DATA:
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "Disconnected");
            break;
        default:
            break;
    }
    return ESP_OK;
}

bool OTAUpdater::update(const char* url) {
    ESP_LOGI(TAG, "Starting OTA from: %s", url);

    esp_http_client_config_t http = {};
    http.url = url;
    http.event_handler = http_events;
    http.timeout_ms = 30000;
    http.keep_alive_enable = true;
    http.buffer_size = 1024;

    // If HTTP instead of HTTPS — bypass cert
    if (strncmp(url, "https://", 8) != 0) {
        http.cert_pem = ""; 
    }

    esp_https_ota_config_t cfg = {};
    cfg.http_config = &http;

    esp_err_t ret = esp_https_ota(&cfg);
    return ret == ESP_OK;
}

void OTAUpdater::printInfo() {
    const esp_partition_t *p = esp_ota_get_running_partition();
    ESP_LOGI(TAG, "Running partition: %s @ 0x%lx", p->label, p->address);
}
