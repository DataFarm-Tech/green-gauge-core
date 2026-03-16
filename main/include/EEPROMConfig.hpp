#pragma once

#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "Types.hpp"
#include "DeviceConfig.hpp"

/**
 * @brief Manages persistent device configuration storage using ESP-IDF NVS.
 *
 * Provides typed read/write helpers for all fields in DeviceConfig,
 * including strings, booleans, integers, floats, and raw binary blobs.
 */
class EEPROMConfig {
private:
    nvs_handle_t handle;
    const char* TAG = "EEPROMConfig";
    const char* NAMESPACE = "config_ns";

    /**
     * @brief Initialises the NVS flash partition, erasing it if corrupted or version-mismatched.
     */
    void init_nvs();

    /**
     * @brief Reads a null-terminated string from NVS into a character buffer.
     * @param key     NVS key to read.
     * @param dest    Destination buffer.
     * @param max_len Maximum number of bytes to write into dest, including null terminator.
     * @return true on success, false if the key does not exist or the buffer is too small.
     */
    bool readString(const char* key, char* dest, size_t max_len);

    /**
     * @brief Reads a boolean value from NVS (stored as uint8).
     * @param key  NVS key to read.
     * @param dest Pointer to the destination bool.
     * @return true on success, false if the key does not exist.
     */
    bool readBool(const char* key, bool* dest);

    /**
     * @brief Reads a uint8_t value from NVS.
     * @param key  NVS key to read.
     * @param dest Pointer to the destination uint8_t.
     * @return true on success, false if the key does not exist.
     */
    bool readU8(const char* key, uint8_t* dest);

    /**
     * @brief Reads a uint32_t value from NVS.
     * @param key  NVS key to read.
     * @param dest Pointer to the destination uint32_t.
     * @return true on success, false if the key does not exist.
     */
    bool readU32(const char* key, uint32_t* dest);

    /**
     * @brief Reads a uint64_t value from NVS.
     * @param key  NVS key to read.
     * @param dest Pointer to the destination uint64_t.
     * @return true on success, false if the key does not exist.
     */
    bool readU64(const char* key, uint64_t* dest);

    /**
     * @brief Reads a float value from NVS (stored as a decimal string).
     * @param key  NVS key to read.
     * @param dest Pointer to the destination float.
     * @return true on success, false if the key does not exist.
     */
    bool readFloat(const char* key, float* dest);

    /**
     * @brief Writes a null-terminated string to NVS.
     * @param key   NVS key to write.
     * @param value Null-terminated string to store.
     * @return true on success, false on NVS write error.
     */
    bool writeString(const char* key, const char* value);

    /**
     * @brief Writes a boolean value to NVS (stored as uint8).
     * @param key   NVS key to write.
     * @param value Value to store.
     * @return true on success, false on NVS write error.
     */
    bool writeBool(const char* key, bool value);

    /**
     * @brief Writes a uint8_t value to NVS.
     * @param key   NVS key to write.
     * @param value Value to store.
     * @return true on success, false on NVS write error.
     */
    bool writeU8(const char* key, uint8_t value);

    /**
     * @brief Writes a uint32_t value to NVS.
     * @param key   NVS key to write.
     * @param value Value to store.
     * @return true on success, false on NVS write error.
     */
    bool writeU32(const char* key, uint32_t value);

    /**
     * @brief Writes a uint64_t value to NVS.
     * @param key   NVS key to write.
     * @param value Value to store.
     * @return true on success, false on NVS write error.
     */
    bool writeU64(const char* key, uint64_t value);

    /**
     * @brief Writes a float value to NVS (serialised as a decimal string with 2dp).
     * @param key   NVS key to write.
     * @param value Value to store.
     * @return true on success, false on NVS write error.
     */
    bool writeFloat(const char* key, float value);

    /**
     * @brief Reads a raw binary blob from NVS.
     * @param key  NVS key to read.
     * @param dest Destination buffer. Must be at least @p len bytes.
     * @param len  Number of bytes to read.
     * @return true on success, false if the key does not exist or sizes do not match.
     */
    bool readBlob(const char* key, void* dest, size_t len);

    /**
     * @brief Writes a raw binary blob to NVS.
     * @param key   NVS key to write.
     * @param value Pointer to the data to store.
     * @param len   Number of bytes to write.
     * @return true on success, false on NVS write error.
     */
    bool writeBlob(const char* key, const void* value, size_t len);

public:
    EEPROMConfig() : handle(0) {}

    /**
     * @brief Initialises NVS and opens the config namespace for read/write access.
     * @return true if the namespace was opened successfully, false otherwise.
     */
    bool begin();

    /**
     * @brief Closes the NVS namespace handle, releasing associated resources.
     */
    void close();

    /**
     * @brief Serialises and writes a DeviceConfig to NVS, then commits the transaction.
     * @param config The configuration to persist.
     * @return true if all fields were written and committed successfully, false otherwise.
     */
    bool saveConfig(const DeviceConfig& config);

    /**
     * @brief Reads all DeviceConfig fields from NVS into the provided struct.
     * @param config Reference to the DeviceConfig to populate.
        * @return true if schema version matches and all required keys are present/valid.
     */
    bool loadConfig(DeviceConfig& config);

    /**
     * @brief Erases all keys in the config namespace and commits the change.
     * @return true on success, false if the erase or commit failed.
     */
    bool eraseConfig();
};

extern EEPROMConfig eeprom;