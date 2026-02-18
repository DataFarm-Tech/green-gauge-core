#pragma once
#include <cstddef>
#include <cstdint>

class Key {
public:
    static constexpr size_t HMAC_SIZE = 32;  // public so callers can size their buffers
    /**
     * @brief Computes an HMAC key using the device's secret key and stores it in the provided output buffer.
     * @param out_hmac Pointer to the buffer where the computed HMAC will be stored
     * @param out_len Size of the output buffer in bytes. Must be at least Key::HMAC_SIZE.
     */
    static void computeKey(uint8_t* out_hmac, size_t out_len);

private:
    static constexpr size_t HMAC_KEY_SIZE = 32;
    /**
     * @brief The device's secret key used for HMAC computation. In a real implementation, this should be securely stored and not hardcoded.
     * For demonstration purposes, we use a fixed key here. In production, consider using secure storage and provisioning mechanisms.
     */
    static constexpr uint8_t secretKey[HMAC_KEY_SIZE] = {
        0x12, 0xA4, 0x56, 0xB7, 0x8C, 0x91, 0xDE, 0xF3,
        0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x01, 0x23,
        0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0x12,
        0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0x12};
};