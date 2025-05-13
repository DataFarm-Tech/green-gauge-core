#include "hash_cache.h"
#include <string.h>
#include "eeprom.h"
#include "msg_queue.h"

/**
 * @brief This function checks if two hashes are equal.
 * @param a - The first hash
 * @param b - The second hash
 * @return true if the hashes are equal, false otherwise
 */
static bool hashes_equal(const uint8_t *a, const uint8_t *b) {
    for (int i = 0; i < HASH_SIZE; i++) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

/**
 * @brief This function initializes the hash cache.
 * It sets the count to 0 and the head to 0.
 * It also clears the entries in the cache.
 * @return None
 */
void hash_cache_init() {
    config.cache.count = 0;
    config.cache.head = 0;
    memset(config.cache.entries, 0, sizeof(config.cache.entries));
}

/**
 * @brief This function checks if the hash is already in the cache.
 * @param hash - The full packet (hash is inside)
 * @return true if the hash is in the cache, false otherwise
 */
bool hash_cache_contains(const uint8_t *hash) {
    uint8_t hash_cache_entry[HASH_SIZE];
    memcpy(hash_cache_entry, hash + 14, HASH_SIZE);

    
    for (int i = 0; i < config.cache.count; i++) {
        uint8_t index = (config.cache.head + i) % hash_cache_size;
        if (hashes_equal(hash_cache_entry, config.cache.entries[index])) {
            return true;
        }
    }
    return false;
}

/**
 * @brief This function adds a hash to the cache.
 * It will replace the oldest entry if the cache is full.
 * @param hash - The hash to be added to the cache
 * @return None
 */
void hash_cache_add(const uint8_t *hash) {
    uint8_t hash_cache_entry[HASH_SIZE];
    memcpy(hash_cache_entry, hash + 14, HASH_SIZE);

    uint8_t index;
    if (config.cache.count < hash_cache_size) {
        index = (config.cache.head + config.cache.count) % hash_cache_size;
        config.cache.count++;
    } else {
        index = config.cache.head;
        config.cache.head = (config.cache.head + 1) % hash_cache_size;
    }
    memcpy(config.cache.entries[index], hash_cache_entry, HASH_SIZE);
    save_config();
}
