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

    packet_to_send[ADDRESS_SIZE * 2] = pkt->num_nodes;

    packet_to_send[ADDRESS_SIZE * 2 + 1] = pkt->ttl;

    for (j = 0; j < SHA256_SIZE; j++) 
    {
        packet_to_send[ADDRESS_SIZE * 2 + 1 + 1 + j] = sha256Hash[j];
    }

    for (j = 0; j < DATA_SIZE; j++) 
    {
        packet_to_send[ADDRESS_SIZE * 2 + 1 + 1 + SHA256_SIZE + j] = pkt->data[j];
    }

    calc_crc(packet_to_send, PACKET_LENGTH - CRC_SIZE);
    printf("OK\n");
}

message describe_packet(const uint8_t *buf, uint8_t buf_len)
{
    message msg;

    for (int i = 0; i < ADDRESS_SIZE; i++) 
    {
        msg.des_node[i] = (char)buf[i];
        msg.src_node[i] = (char)buf[ADDRESS_SIZE + i];
    }
    
    msg.des_node[ADDRESS_SIZE] = '\0';
    msg.src_node[ADDRESS_SIZE] = '\0';

    /* move this into a function*/
    msg.data.rs485_humidity = buf[ADDRESS_SIZE * 2];
    msg.data.rs485_temp = buf[ADDRESS_SIZE * 2 + 1];
    msg.data.rs485_con = buf[ADDRESS_SIZE * 2 + 2];
    msg.data.rs485_ph = buf[ADDRESS_SIZE * 2 + 3];
    msg.data.rs485_nit = buf[ADDRESS_SIZE * 2 + 4];
    msg.data.rs485_phos = buf[ADDRESS_SIZE * 2 + 5];
    msg.data.rs485_pot = buf[ADDRESS_SIZE * 2 + 6];

    return msg;
}
