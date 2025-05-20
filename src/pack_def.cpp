#include "pack_def.h"
#include "config.h"
#include "msg_queue.h"
#include "crypt.h"
#include "hash_cache.h"
#include "https_comms.h"
#include "utils.h"

#define CN001_REQ_ID 0x01
#define SN001_SUC_RSP_ID 0x02
#define SN001_ERR_RSP_ID 0x03

void cn001_notify_error(String src_node, uint8_t rc);

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

    buf[offset] = CN001_REQ_ID; /** Copy MSG_ID into buf */

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

    buf[offset] = SN001_SUC_RSP_ID;

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

    buf[offset++] = pkt->battery_lev;

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

    buf[offset] = SN001_ERR_RSP_ID;

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

    buf[offset++] = pkt->battery_lev;

    if (offset != SN001_ERR_RSP_LEN - CRC_SIZE) 
    {
        printf("SN001_ERR_RSP packet length mismatch: expected %d, got %d\n", SN001_ERR_RSP_LEN, offset);
        return;
    }
    
    calc_crc(buf, offset);
}

/**
 * @brief This function extracts the valuable data from the packet.
 * It checks whether the msg_id embedded is successful. If it's successful 
 * then it is added too the internal msg queue. Otherwise, a notification is sent
 * to the user.
 * @param buf - The packet
 * @param buf_len - the packet's length
 */
void cn001_handle_packet(const uint8_t *buf, uint8_t buf_len)
{
    String des_node;
    String src_node;

    uint8_t msg_id = buf[0];
    int i;

    /** Extract destination node from packet */
    for (i = 0; i < ADDRESS_SIZE; i++) 
    {
        des_node[i] = (char)buf[i + 1];
    }
    
    /** Extract source node from packet */
    for (i = 0; i < ADDRESS_SIZE; i++)
    {
        src_node[i] = (char)buf[1 + ADDRESS_SIZE + i];
    }
    
    des_node[ADDRESS_SIZE + 1] = '\0'; /** May need to do this not sure */
    src_node[ADDRESS_SIZE + 1] = '\0';
    
    switch (msg_id)
    {
        case SN001_SUC_RSP_ID:
        {
            sn001_suc_rsp suc_rsp;
            int index = 1; /** index is 1, since msg_id holds 1st byte. */
            
            suc_rsp.src_node = src_node;
            suc_rsp.des_node = des_node;

            suc_rsp.data.rs485_humidity = buf[ADDRESS_SIZE * 2 + index];
            suc_rsp.data.rs485_temp = buf[ADDRESS_SIZE * 2 + ++index];
            suc_rsp.data.rs485_con = buf[ADDRESS_SIZE * 2 + ++index];
            suc_rsp.data.rs485_ph = buf[ADDRESS_SIZE * 2 + ++index];
            suc_rsp.data.rs485_nit = buf[ADDRESS_SIZE * 2 + ++index];
            suc_rsp.data.rs485_phos = buf[ADDRESS_SIZE * 2 + ++index];
            suc_rsp.data.rs485_pot = buf[ADDRESS_SIZE * 2 + ++index];

            if (xSemaphoreTake(msg_queue_mh, portMAX_DELAY) == pdTRUE)
            {
                internal_msg_q.push(suc_rsp);
                xSemaphoreGive(msg_queue_mh);
            }
        
            break;
        }
        case SN001_ERR_RSP_ID:
        {
            /**
             * RED LED is NOT! set to high
             * since this is an error about another 
             * node.
             */
            cn001_notify_message(src_node, buf[13]);
            break;
        }
    default:
        break;
    }
}