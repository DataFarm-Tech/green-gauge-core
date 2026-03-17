#include "CoapOTAUpdater.hpp"

extern "C" {
#include "esp_http_client.h"
#include "esp_ota_ops.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
}

#include <cstdio>
#include <cctype>
#include <cstring>
#include "CoapPktAssm.hpp"
#include "CborDecoder.hpp"
#include "EEPROMConfig.hpp"
#include "Utils.hpp"

CoapOTAUpdater::CoapOTAUpdater(Communication& communication, const char* current_firmware_version)
    : comm(communication),
            current_version(current_firmware_version ? current_firmware_version : "")
{
        std::memset(&http_config, 0, sizeof(http_config));
        http_config.timeout_ms = 60000;
        http_config.transport_type = HTTP_TRANSPORT_OVER_SSL;
        http_config.buffer_size = 2048;
        http_config.buffer_size_tx = 1024;
}

bool CoapOTAUpdater::streamFirmwareFromHttpsToOta(const std::string& firmware_url, esp_ota_handle_t ota_handle, size_t& total_written)
{
    total_written = 0;

    // Preferred path: stream HTTPS bytes via active communication transport (SIM modem on EC25).
    const bool streamed_over_transport = comm.streamHttpsGet(
        firmware_url,
        [ota_handle, &total_written](const uint8_t* chunk, size_t chunk_len) -> bool {
            if (!chunk || chunk_len == 0)
            {
                return true;
            }

            const esp_err_t write_err = esp_ota_write(ota_handle, chunk, chunk_len);
            if (write_err != ESP_OK)
            {
                printf("esp_ota_write failed during transport stream: %s\n", esp_err_to_name(write_err));
                return false;
            }

            total_written += chunk_len;
            return true;
        });

    if (streamed_over_transport)
    {
        if (total_written == 0)
        {
            printf("Transport HTTPS stream completed but returned no payload\n");
            return false;
        }

        return true;
    }

    if (comm.getType() == ConnectionType::SIM)
    {
        printf("SIM HTTPS stream failed; skipping esp_http_client fallback\n");
        return false;
    }

    printf("Transport HTTPS stream unavailable, falling back to esp_http_client\n");

    http_config.url = firmware_url.c_str();

    esp_http_client_handle_t client = esp_http_client_init(&http_config);
    if (!client)
    {
        printf("Failed to initialize HTTP client for firmware download\n");
        return false;
    }

    auto cleanup_client = [&client]() {
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
    };

    bool success = false;
    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK)
    {
        printf("esp_http_client_open failed: %s\n", esp_err_to_name(err));
        cleanup_client();
        return false;
    }

    const int64_t status_code = esp_http_client_fetch_headers(client);
    if (status_code < 0)
    {
        printf("Failed to fetch firmware download headers\n");
        cleanup_client();
        return false;
    }

    const int http_status = esp_http_client_get_status_code(client);
    if (http_status != 200)
    {
        printf("Firmware HTTPS download failed with status %d\n", http_status);
        cleanup_client();
        return false;
    }

    char http_buffer[2048];
    while (true)
    {
        const int bytes_read = esp_http_client_read(client, http_buffer, sizeof(http_buffer));
        if (bytes_read < 0)
        {
            printf("esp_http_client_read failed\n");
            cleanup_client();
            return false;
        }

        if (bytes_read == 0)
        {
            if (esp_http_client_is_complete_data_received(client))
            {
                success = true;
            }
            break;
        }

        err = esp_ota_write(ota_handle, http_buffer, static_cast<size_t>(bytes_read));
        if (err != ESP_OK)
        {
            printf("esp_ota_write failed during HTTPS stream: %s\n", esp_err_to_name(err));
            cleanup_client();
            return false;
        }

        total_written += static_cast<size_t>(bytes_read);
    }

    cleanup_client();

    return success;
}

int CoapOTAUpdater::compareVersions(const std::string& v1, const std::string& v2)
{
    auto readSegment = [](const std::string& version, size_t& pos) -> int {
        if (pos >= version.size())
        {
            return 0;
        }

        const size_t next_pos = version.find('.', pos);
        const size_t end = (next_pos == std::string::npos) ? version.size() : next_pos;
        const std::string segment = version.substr(pos, end - pos);
        pos = (next_pos == std::string::npos) ? version.size() : (next_pos + 1);

        int value = 0;
        bool saw_digit = false;
        for (char ch : segment)
        {
            if (!std::isdigit(static_cast<unsigned char>(ch)))
            {
                break;
            }
            saw_digit = true;
            value = (value * 10) + (ch - '0');
        }

        return saw_digit ? value : 0;
    };

    size_t pos1 = 0;
    size_t pos2 = 0;
    while (pos1 < v1.size() || pos2 < v2.size())
    {
        const int seg1 = readSegment(v1, pos1);
        const int seg2 = readSegment(v2, pos2);

        if (seg1 < seg2)
        {
            return -1;
        }
        if (seg1 > seg2)
        {
            return 1;
        }
    }

    return 0;
}


bool CoapOTAUpdater::isFirmwareAvailable()
{
    std::string firmware_version_cbor;
    CborStringTransform version_transform;
    static constexpr int MAX_VERSION_CHECK_ATTEMPTS = 2;

    bool got_response = false;
    for (int attempt = 1; attempt <= MAX_VERSION_CHECK_ATTEMPTS; ++attempt)
    {
        firmware_version_cbor.clear();
        if (comm.sendPacket(nullptr, 0, firmwareversion_entry, firmware_version_cbor))
        {
            got_response = true;
            break;
        }

        printf("Firmware version request failed (attempt %d/%d)\n", attempt, MAX_VERSION_CHECK_ATTEMPTS);
        if (attempt < MAX_VERSION_CHECK_ATTEMPTS)
        {
            vTaskDelay(pdMS_TO_TICKS(1500));
        }
    }

    if (!got_response) {
        printf("Failed to check for firmware update after retries\n");
        return false;
    }

    version_transform.incoming = firmware_version_cbor;
    if (!CborDecoder::decodeText(version_transform))
    {
        printf("Failed to decode firmware version CBOR (payload size: %zu bytes)\n", firmware_version_cbor.size());
        return false;
    }

    std::string firmware_version = version_transform.outgoing;
    Utils::trimTrailingWhitespace(firmware_version);

    if (firmware_version.empty())
    {
        printf("Firmware version response is empty\n");
        return false;
    }

    available_version = firmware_version;
    const bool update_available = compareVersions(available_version, current_version) > 0;

    printf("OTA version check: current='%s', available='%s', update=%s\n",
           current_version.c_str(),
           available_version.c_str(),
           update_available ? "yes" : "no");

    return update_available;
}

bool CoapOTAUpdater::executeUpdate()
{
    CborStringTransform firmware_transform;
    std::string firmware_version_to_store = available_version;
    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(nullptr);
    esp_ota_handle_t ota_handle = 0;
    std::string firmware_cbor;
    std::string firmware_url;
    size_t total_written = 0;
    
    if (!update_partition)
    {
        printf("Failed to get next OTA partition\n");
        return false;
    }
    
    if (firmware_version_to_store.empty())
    {
        std::string firmware_version_cbor;
        CborStringTransform version_transform;
        
        if (!comm.sendPacket(nullptr, 0, firmwareversion_entry, firmware_version_cbor))
        {
            printf("Failed to read firmware version before OTA write\n");
            return false;
        }

        version_transform.incoming = firmware_version_cbor;
        if (!CborDecoder::decodeText(version_transform))
        {
            printf("Failed to decode firmware version CBOR before OTA write\n");
            return false;
        }

        firmware_version_to_store = version_transform.outgoing;

        Utils::trimTrailingWhitespace(firmware_version_to_store);
    }

    if (firmware_version_to_store.empty())
    {
        printf("Firmware version to store is empty\n");
        return false;
    }
    
    esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
    
    if (err != ESP_OK)
    {
        printf("esp_ota_begin failed: %s\n", esp_err_to_name(err));
        return false;
    }

    if (!comm.sendPacket(nullptr, 0, firmwaredownload_entry, firmware_cbor))
    {
        esp_ota_abort(ota_handle);
        printf("Firmware URL request failed\n");
        return false;
    }

    firmware_transform.incoming = firmware_cbor;
    if (!CborDecoder::decodeText(firmware_transform))
    {
        esp_ota_abort(ota_handle);
        printf("Failed to decode firmware URL CBOR\n");
        return false;
    }

    firmware_url = firmware_transform.outgoing;
    Utils::trimTrailingWhitespace(firmware_url);

    if (firmware_url.empty())
    {
        esp_ota_abort(ota_handle);
        printf("Firmware URL is empty\n");
        return false;
    }

    if (!streamFirmwareFromHttpsToOta(firmware_url, ota_handle, total_written))
    {
        esp_ota_abort(ota_handle);
        printf("Firmware HTTPS streaming failed\n");
        return false;
    }

    if (total_written == 0)
    {
        esp_ota_abort(ota_handle);
        printf("Firmware payload is empty\n");
        return false;
    }

    err = esp_ota_end(ota_handle);
    if (err != ESP_OK)
    {
        printf("esp_ota_end failed: %s\n", esp_err_to_name(err));
        return false;
    }

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK)
    {
        printf("esp_ota_set_boot_partition failed: %s\n", esp_err_to_name(err));
        return false;
    }

    std::strncpy(g_device_config.manf_info.fw_ver.value,
                 firmware_version_to_store.c_str(),
                 MANF_MAX_LEN - 1);
    g_device_config.manf_info.fw_ver.value[MANF_MAX_LEN - 1] = '\0';

    if (!eeprom.saveConfig(g_device_config))
    {
        printf("Failed to persist firmware version to EEPROM\n");
        return false;
    }

    available_version = firmware_version_to_store;
    current_version = firmware_version_to_store;

    printf("Firmware streamed and written successfully (%zu bytes)\n", total_written);
    return true;
}
