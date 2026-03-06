#include "CoapOTAUpdater.hpp"

extern "C" {
#include "esp_ota_ops.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
}

#include <cstdio>
#include <cstring>
#include "CoapPktAssm.hpp"
#include "CborDecoder.hpp"
#include "EEPROMConfig.hpp"
#include "Utils.hpp"

CoapOTAUpdater::CoapOTAUpdater(Communication& communication, const char* current_firmware_version)
    : comm(communication),
      current_version(current_firmware_version ? current_firmware_version : "")
{
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
    const bool update_available = available_version.compare(current_version) > 0;

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
    std::string firmware_binary;
    
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
        printf("Firmware download failed\n");
        return false;
    }

    firmware_transform.incoming = firmware_cbor;
    if (!CborDecoder::decodeBytes(firmware_transform))
    {
        esp_ota_abort(ota_handle);
        printf("Failed to decode firmware binary CBOR\n");
        return false;
    }

    firmware_binary = firmware_transform.outgoing;

    const size_t total_written = firmware_binary.size();

    if (total_written == 0)
    {
        esp_ota_abort(ota_handle);
        printf("Firmware payload is empty\n");
        return false;
    }

    err = esp_ota_write(ota_handle, firmware_binary.data(), firmware_binary.size());
    if (err != ESP_OK)
    {
        printf("esp_ota_write failed: %s\n", esp_err_to_name(err));
        esp_ota_abort(ota_handle);
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
