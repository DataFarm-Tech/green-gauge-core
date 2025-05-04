#pragma once
#include <Arduino.h>
#include "crypt/crypt.h"

#define ADDRESS_SIZE 6
#define PACKET_LENGTH 47

typedef struct {
    char des_node[ADDRESS_SIZE + 1];  // Destination node as string
    char src_node[ADDRESS_SIZE + 1];  // Source node as string
    uint8_t ttl;                      // Time-to-live
    uint8_t hash[SHA256_SIZE];       // SHA256 hash
    uint8_t data[7];                 // Data field
    uint16_t crc;                    // CRC
} packet;

void create_packet(uint8_t *packet_to_send, const packet *pkt, uint8_t seq_id);
packet describe_packet(const uint8_t *buf, uint8_t buf_len);
