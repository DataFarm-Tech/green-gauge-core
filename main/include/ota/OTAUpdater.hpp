// #pragma once

// #include "esp_https_ota.h"
// #include "esp_crt_bundle.h"
// #include "esp_log.h"

// /**
//  * @class OTAUpdater
//  * @brief Handles HTTPS OTA (Over-the-Air) firmware updates on the ESP32.
//  *
//  * This class encapsulates all OTA update configuration and logic, providing
//  * a simple interface to configure timeouts, buffer sizes, and flash erase
//  * behavior. It uses ESP-IDF’s built-in HTTPS OTA APIs and the default
//  * certificate bundle for secure connections.
//  *
//  * Typical usage:
//  * @code
//  * OTAUpdater ota;
//  * ota.setTimeout(15000);
//  * ota.setBufferSizes(8192, 4096);
//  * ota.enableBulkErase(true);
//  * ota.performUpdate("https://example.com/firmware.bin");
//  * @endcode
//  */
// class OTAUpdater {
// public:
//     /**
//      * @brief Construct a new OTAUpdater instance.
//      * 
//      * @param tag Log tag used for ESP_LOGx output (default: "OTA").
//      */
//     OTAUpdater(const char* tag = "OTA");

//     /**
//      * @brief Perform an OTA update using the specified HTTPS URL.
//      *
//      * The function configures the HTTPS client, initiates a secure download of
//      * the new firmware image, verifies it, and flashes it to the inactive OTA
//      * partition. On success, the device automatically restarts to boot the new
//      * firmware.
//      *
//      * @param url HTTPS URL pointing to the firmware binary.
//      * @return esp_err_t ESP_OK if the OTA succeeds; an error code otherwise.
//      */
//     esp_err_t performUpdate(const char* url);

//     /**
//      * @brief Set the HTTPS connection timeout.
//      *
//      * @param ms Timeout in milliseconds for the OTA HTTPS connection.
//      */
//     void setTimeout(uint32_t ms);

//     /**
//      * @brief Configure RX and TX buffer sizes for the HTTP client.
//      *
//      * Adjusting these may improve OTA performance for large binaries
//      * or networks with higher latency.
//      *
//      * @param rx Receive buffer size in bytes (default: 8192).
//      * @param tx Transmit buffer size in bytes (default: 2048).
//      */
//     void setBufferSizes(int rx, int tx);

//     /**
//      * @brief Enable or disable bulk flash erase during OTA.
//      *
//      * Bulk erase can speed up OTA updates on large flash chips by erasing
//      * the entire OTA partition at once, but increases flash wear slightly.
//      *
//      * @param enable True to enable bulk erase; false to disable.
//      */
//     void enableBulkErase(bool enable);

// private:
//     /** @brief Log tag for all OTA log messages. */
//     const char* TAG;

//     /** @brief HTTP client configuration for OTA requests. */
//     esp_http_client_config_t httpConfig;

//     /** @brief OTA configuration structure passed to esp_https_ota(). */
//     esp_https_ota_config_t otaConfig;

//     /** @brief HTTPS request timeout in milliseconds. */
//     uint32_t timeout_ms;

//     /** @brief HTTP receive buffer size. */
//     int buffer_size;

//     /** @brief HTTP transmit buffer size. */
//     int buffer_size_tx;

//     /** @brief Whether to perform a bulk flash erase before OTA flashing. */
//     bool bulk_erase;

//     /**
//      * @brief Apply runtime configuration to the internal OTA structures.
//      *
//      * This function prepares `httpConfig` and `otaConfig` just before
//      * the OTA process begins. It should only be used internally.
//      *
//      * @param url HTTPS firmware URL.
//      */
//     void applyConfig(const char* url);
// };
