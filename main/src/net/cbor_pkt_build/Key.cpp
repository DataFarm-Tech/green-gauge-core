#include "Key.hpp"
#include "psa/crypto.h"

// Required for static constexpr array with external linkage in C++14/17
constexpr uint8_t Key::secretKey[Key::HMAC_KEY_SIZE];

void Key::computeKey(uint8_t* out_hmac, size_t out_len) {
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_key_id_t key_id = 0;
    size_t mac_length = 0;

    psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_SIGN_HASH);
    psa_set_key_algorithm(&attributes, PSA_ALG_HMAC(PSA_ALG_SHA_256));
    psa_set_key_type(&attributes, PSA_KEY_TYPE_HMAC);
    psa_set_key_bits(&attributes, HMAC_KEY_SIZE * 8);

    if (psa_import_key(&attributes, secretKey, HMAC_KEY_SIZE, &key_id) != PSA_SUCCESS) {
        psa_reset_key_attributes(&attributes);
        return;
    }

    psa_mac_compute(key_id, PSA_ALG_HMAC(PSA_ALG_SHA_256),
                    NULL, 0,
                    out_hmac, out_len, &mac_length);

    psa_destroy_key(key_id);
    psa_reset_key_attributes(&attributes);
}