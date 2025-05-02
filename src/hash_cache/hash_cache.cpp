#include "hash_cache.h"
#include <string.h>
#include "eeprom/eeprom.h"

static bool hashes_equal(const uint8_t *a, const uint8_t *b) {
    for (int i = 0; i < HASH_SIZE; i++) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

void hash_cache_init() {
    config.cache.count = 0;
    config.cache.head = 0;
    memset(config.cache.entries, 0, sizeof(config.cache.entries));
}

bool hash_cache_contains(const uint8_t *hash) {
    for (int i = 0; i < config.cache.count; i++) {
        uint8_t index = (config.cache.head + i) % HASH_CACHE_SIZE;
        if (hashes_equal(hash, config.cache.entries[index])) {
            return true;
        }
    }
    return false;
}

void hash_cache_add(const uint8_t *hash) {
    uint8_t index;
    if (config.cache.count < HASH_CACHE_SIZE) {
        index = (config.cache.head + config.cache.count) % HASH_CACHE_SIZE;
        config.cache.count++;
    } else {
        index = config.cache.head;
        config.cache.head = (config.cache.head + 1) % HASH_CACHE_SIZE;
    }
    memcpy(config.cache.entries[index], hash, HASH_SIZE);
    save_config();
}
