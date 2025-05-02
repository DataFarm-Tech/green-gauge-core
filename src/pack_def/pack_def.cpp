#include "pack_def.h"
#include "config.h"
#include "msg_queue.h"
#include "crypt/crypt.h"
#include "hash_cache/hash_cache.h"

/**
 * @brief The following function populates a buffer, that includes packet information.
 * This packet follows the DF_PA_1 structure:
 * 
 * - 0-5 (Destination Node)
 * - 6-11 (Source Node)
 * - 12 (TTL)
 * - 13-45 (HASH)
 * - 46-47 (CRC)
 * @param packet_to_send The buffer to populate
 * @param src_node_id the address of the src node to be put in buffer
 * @param seq_id The incrementing seq_id to create hash with src_node
 * @param dest_node_id The address of the dest node to be put in buffer
 * @return None
 */


void create_packet(uint8_t * packet_to_send, const char * src_node_id, uint8_t seq_id, const char * dest_node_id)
{
    byte sha256Hash[SHA256_SIZE];
    String data_to_hash = String(src_node_id) + String(seq_id);
    generate_hash(data_to_hash, sha256Hash);


    int j;

    for (j = 0; j < ADDRESS_SIZE; j++)
    {
        packet_to_send[j] = dest_node_id[j];
    }

    for (j = 0; j < ADDRESS_SIZE; j++)
    {
        packet_to_send[ADDRESS_SIZE + j] = src_node_id[j];
    }

    packet_to_send[ADDRESS_SIZE * 2] = ttl;

    for (j = 0; j < SHA256_SIZE; j++)
    {
        packet_to_send[ADDRESS_SIZE * 2 + 1 + j] = sha256Hash[j];
    }

    for (j = ADDRESS_SIZE * 2 + 1 + SHA256_SIZE; j < PACKET_LENGTH - CRC_SIZE; j++)
    {
        packet_to_send[j] = 0;
    }

    calc_crc(packet_to_send, PACKET_LENGTH - CRC_SIZE);
}


pack_def describe_packet(uint8_t *buf, uint8_t buf_len)
{
    pack_def packet;

    // Extract destination node (6 bytes + null terminator)
    for (int i = 0; i < ADDRESS_SIZE; i++) 
    {
        packet.des_node[i] = (char)buf[i];
    }
    packet.des_node[ADDRESS_SIZE] = '\0';

    // Extract source node (6 bytes + null terminator)
    for (int i = 0; i < ADDRESS_SIZE; i++) 
    {
        packet.src_node[i] = (char)buf[ADDRESS_SIZE + i];
    }
    packet.src_node[ADDRESS_SIZE] = '\0';

    // TTL
    packet.ttl = buf[ADDRESS_SIZE * 2];

    // SHA256 hash
    for (int i = 0; i < SHA256_SIZE; i++)
    {
        packet.hash[i] = buf[ADDRESS_SIZE * 2 + 1 + i];
    }

    // Data
    for (int i = 0; i < 7; i++) 
    {
        packet.data[i] = buf[ADDRESS_SIZE * 2 + 1 + SHA256_SIZE + i];
    }

    // CRC
    if (buf_len >= 2) {
        packet.crc = (buf[buf_len - 2] << 8) | buf[buf_len - 1];
    } else {
        packet.crc = 0;
    }

    return packet;
}

