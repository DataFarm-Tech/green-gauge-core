// #include "ota/OTAUpdater.hpp"
// #include "esp_system.h"

// OTAUpdater::OTAUpdater(const char * tag)
//     : TAG(tag),
//       timeout_ms(10000),
//       buffer_size(8192),
//       buffer_size_tx(2048),
//       bulk_erase(true)
// {
//     memset(&httpConfig, 0, sizeof(httpConfig));
//     memset(&otaConfig, 0, sizeof(otaConfig));

//     // set default config values
//     httpConfig.keep_alive_enable = true;
//     httpConfig.crt_bundle_attach = esp_crt_bundle_attach;
//     otaConfig.http_config = &httpConfig;
// }

// void OTAUpdater::setTimeout(uint32_t ms) {
//     timeout_ms = ms;
// }

// void OTAUpdater::setBufferSizes(int rx, int tx) {
//     buffer_size = rx;
//     buffer_size_tx = tx;
// }

// void OTAUpdater::enableBulkErase(bool enable) {
//     bulk_erase = enable;
// }

// void OTAUpdater::applyConfig(const char* url) {
//     httpConfig.url = url;
//     httpConfig.timeout_ms = timeout_ms;
//     httpConfig.buffer_size = buffer_size;
//     httpConfig.buffer_size_tx = buffer_size_tx;
//     otaConfig.bulk_flash_erase = bulk_erase;
// }

// esp_err_t OTAUpdater::performUpdate(const char* url) {
//     applyConfig(url);
//     ESP_LOGI(TAG, "Starting OTA update from %s", url);

//     esp_err_t ret = esp_https_ota(&otaConfig);

//     if (ret == ESP_OK) {
//         ESP_LOGI(TAG, "OTA successful, restarting...");
//         esp_restart();
//     } else {
//         ESP_LOGE(TAG, "OTA failed: %s", esp_err_to_name(ret));
//     }

//     return ret;
// }
