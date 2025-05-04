#include "pack_def.h"
#include "config.h"
#include "msg_queue.h"
#include "crypt/crypt.h"
#include "hash_cache/hash_cache.h"

void create_packet(uint8_t *packet_to_send, const packet *pkt, uint8_t seq_id)
{
    byte sha256Hash[SHA256_SIZE];
    String data_to_hash = String(pkt->src_node) + String(seq_id);
    generate_hash(data_to_hash, sha256Hash);

    int j;

    for (j = 0; j < ADDRESS_SIZE; j++) 
    {
        packet_to_send[j] = pkt->des_node[j];
    }

    for (j = 0; j < ADDRESS_SIZE; j++) 
    {
        packet_to_send[ADDRESS_SIZE + j] = pkt->src_node[j];
    }

    packet_to_send[ADDRESS_SIZE * 2] = pkt->ttl;

    for (j = 0; j < SHA256_SIZE; j++) 
    {
        packet_to_send[ADDRESS_SIZE * 2 + 1 + j] = sha256Hash[j];
    }

    for (j = 0; j < sizeof(pkt->data); j++) 
    {
        packet_to_send[ADDRESS_SIZE * 2 + 1 + SHA256_SIZE + j] = pkt->data[j];
    }

    calc_crc(packet_to_send, PACKET_LENGTH - CRC_SIZE);
}

packet describe_packet(const uint8_t *buf, uint8_t buf_len)
{
    packet pkt;

    for (int i = 0; i < ADDRESS_SIZE; i++) 
    {
        pkt.des_node[i] = (char)buf[i];
        pkt.src_node[i] = (char)buf[ADDRESS_SIZE + i];
    }
    
    pkt.des_node[ADDRESS_SIZE] = '\0';
    pkt.src_node[ADDRESS_SIZE] = '\0';

    pkt.ttl = buf[ADDRESS_SIZE * 2];

    for (int i = 0; i < SHA256_SIZE; i++) 
    {
        pkt.hash[i] = buf[ADDRESS_SIZE * 2 + 1 + i];
    }

    for (int i = 0; i < sizeof(pkt.data); i++) 
    {
        pkt.data[i] = buf[ADDRESS_SIZE * 2 + 1 + SHA256_SIZE + i];
    }

    pkt.crc = (buf_len >= 2) ? ((buf[buf_len - 2] << 8) | buf[buf_len - 1]) : 0;

    return pkt;
}
