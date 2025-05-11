#include "pack_def.h"
#include "config.h"
#include "msg_queue.h"
#include "crypt/crypt.h"
#include "hash_cache/hash_cache.h"
#include "utils.h"

#define CN001_REQ_ID 0x01
#define SN001_SUC_RSP_ID 0x02
#define SN001_ERR_RSP_ID 0x03

/**
 * The following defines are user to identify the
 * different types of error responses that can be sent.
 * 
 * The error codes are as follows:
 * SN001_ERR_RSP_CODE_A 0x0A - No connection to RS485
 * SN001_ERR_RSP_CODE_B 0x0B - Invalid data from RS485 (upper bounds and lower bounds)
 */
#define SN001_ERR_RSP_CODE_A 0x0A
#define SN001_ERR_RSP_CODE_B 0x0B


/**
 * @brief The following function generates a CN001 request packet.
 * @param buf The buffer to store the packet.
 * @param pkt The CN001 request packet structure.
 * @param seq_id The sequence ID for the packet.
 * @return None
 */
void pkt_cn001_req(uint8_t * buf, const cn001_req * pkt, uint8_t seq_id)
{
    byte sha256Hash[SHA256_SIZE];
    String data_to_hash = String(pkt->src_node) + String(seq_id);
    generate_hash(data_to_hash, sha256Hash);

    int j;
    int offset = 0;

    buf[offset++] = CN001_REQ_ID; /** Copy MSG_ID into buf */

    for (j = 0; j < ADDRESS_SIZE; j++) 
    {
        buf[offset++] = pkt->des_node[j];
    }

    for (j = 0; j < ADDRESS_SIZE; j++) 
    {
        buf[offset++] = pkt->src_node[j];
    }

    buf[offset++] = pkt->num_nodes;
    buf[offset++] = pkt->ttl;

    for (j = 0; j < SHA256_SIZE; j++) 
    {
        buf[offset++] = sha256Hash[j];
    }

    if (offset != CN001_REQ_LEN - CRC_SIZE)
    {
        printf("CN001_REQ packet length mismatch: expected %d, got %d\n", CN001_REQ_LEN, offset);
        return;
    }
    
    calc_crc(buf, offset);
}

/**
 * @brief The following function generates a SN001 response packet.
 * @param buf The buffer to store the packet.
 * @param pkt The SN001 response packet structure.
 * @param seq_id The sequence ID for the packet.
 * @return None
 */
void pkt_sn001_rsp(uint8_t * buf, const sn001_rsp * pkt, uint8_t seq_id)
{
    byte sha256Hash[SHA256_SIZE];
    String data_to_hash = String(pkt->src_node) + String(seq_id);
    generate_hash(data_to_hash, sha256Hash);

    int j;
    int offset = 0;

    buf[offset++] = SN001_SUC_RSP_ID;

    for (j = 0; j < ADDRESS_SIZE; j++) 
    {
        buf[offset++] = pkt->des_node[j];
    }

    for (j = 0; j < ADDRESS_SIZE; j++) 
    {
        buf[offset++] = pkt->src_node[j];
    }

    for (j = 0; j < DATA_SIZE; j++) 
    {
        buf[offset++] = pkt->data[j];
    }

    buf[offset++] = pkt->ttl;

    for (j = 0; j < SHA256_SIZE; j++) 
    {
        buf[offset++] = sha256Hash[j];
    }

    if (offset != SN001_SUC_RSP_LEN - CRC_SIZE) 
    {
        printf("SN001_RSP packet length mismatch: expected %d, got %d\n", SN001_SUC_RSP_LEN, offset);
        return;
    }
    
    calc_crc(buf, offset);
}
/**
 * @brief The following function generates a SN001 error response packet.
 * @param buf The buffer to store the packet.
 * @param pkt The SN001 error response packet structure.
 * @param seq_id The sequence ID for the packet.
 * @return None
 */
void pkt_sn001_err_rsp(uint8_t * buf, const sn001_err_rsp * pkt, uint8_t seq_id)
{
    byte sha256Hash[SHA256_SIZE];
    String data_to_hash = String(pkt->src_node) + String(seq_id);
    generate_hash(data_to_hash, sha256Hash);

    int j;
    int offset = 0;

    buf[offset++] = SN001_ERR_RSP_ID;

    for (j = 0; j < ADDRESS_SIZE; j++) 
    {
        buf[offset++] = pkt->des_node[j];
    }

    for (j = 0; j < ADDRESS_SIZE; j++) 
    {
        buf[offset++] = pkt->src_node[j];
    }

    buf[offset++] = pkt->err_code;
    buf[offset++] = pkt->ttl;

    for (j = 0; j < SHA256_SIZE; j++) 
    {
        buf[offset++] = sha256Hash[j];
    }

    if (offset != SN001_ERR_RSP_LEN - CRC_SIZE) 
    {
        printf("SN001_ERR_RSP packet length mismatch: expected %d, got %d\n", SN001_ERR_RSP_LEN, offset);
        return;
    }
    
    calc_crc(buf, offset);
}

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
