#include "CoapOTAUpdater.hpp"

extern "C" {
#include "esp_ota_ops.h"
}

#include <cstdio>
#include "CoapPktAssm.hpp"

CoapOTAUpdater::CoapOTAUpdater(Communication& communication, const char* current_firmware_version)
    : comm(communication),
      current_version(current_firmware_version ? current_firmware_version : "")
{
}

void CoapOTAUpdater::trimTrailingWhitespace(std::string& value)
{
    while (!value.empty() &&
           (value.back() == '\r' ||
            value.back() == '\n' ||
            value.back() == ' ' ||
            value.back() == '\t'))
    {
        value.pop_back();
    }
}

bool CoapOTAUpdater::isFirmwareAvailable()
{
    std::string firmware_version;

    if (!comm.sendPacket(nullptr, 0, firmwareversion_entry, firmware_version)) {
        printf("Failed to check for firmware update\n");
        return false;
    }

    trimTrailingWhitespace(firmware_version);

    if (firmware_version.empty())
    {
        printf("Firmware version response is empty\n");
        return false;
    }

    available_version = firmware_version;
    return available_version.compare(current_version) > 0;
}

bool CoapOTAUpdater::executeUpdate()
{
    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(nullptr);
    if (!update_partition)
    {
        printf("Failed to get next OTA partition\n");
        return false;
    }

    esp_ota_handle_t ota_handle = 0;
    esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
    if (err != ESP_OK)
    {
        printf("esp_ota_begin failed: %s\n", esp_err_to_name(err));
        return false;
    }

    size_t total_written = 0;
    esp_err_t write_err = ESP_OK;

    const bool download_ok = comm.sendPacketStream(
        nullptr,
        0,
        firmwaredownload_entry,
        [&](const uint8_t* chunk, size_t chunk_len) -> bool {
            if (!chunk || chunk_len == 0)
            {
                return true;
            }

            write_err = esp_ota_write(ota_handle, chunk, chunk_len);
            if (write_err != ESP_OK)
            {
                printf("esp_ota_write failed: %s\n", esp_err_to_name(write_err));
                return false;
            }

            total_written += chunk_len;
            return true;
        });

    if (!download_ok || write_err != ESP_OK)
    {
        esp_ota_abort(ota_handle);
        printf("Firmware download failed\n");
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

    printf("Firmware streamed and written successfully (%zu bytes)\n", total_written);
    return true;
}
