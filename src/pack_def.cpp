#include "pack_def.h"
#include "config.h"
#include "msg_queue.h"
#include "crypt.h"
#include "hash_cache.h"
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

    for (int i = 0; i < ADDRESS_SIZE; i++) 
    {
        des_node[i] = (char)buf[i];
    }

    for (int i = 0; i < ADDRESS_SIZE; i++)
    {
        src_node[i] = (char)buf[ADDRESS_SIZE + i];
    }
    
    des_node[ADDRESS_SIZE] = '\0';
    src_node[ADDRESS_SIZE] = '\0';
    
    switch (buf[0])
    {
        case SN001_SUC_RSP_ID:
        {
            sn001_suc_rsp suc_rsp;
            int index = 0;
            
            suc_rsp.src_node = src_node;
            suc_rsp.des_node = des_node;

            suc_rsp.data.rs485_humidity = buf[ADDRESS_SIZE * 2];
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
            cn001_notify_error(src_node, buf[14]);
            break;
        }
    default:
        break;
    }
}

/**
 * @brief The following function notifies the user that @param src_node
 * is not functioning properly.
 * @param rc
 */
void cn001_notify_error(String src_node, uint8_t rc)
{
    /** This function needs to do something in the database to display the error for
     * src_node.
     */

    switch (rc)
    {
        case SN001_ERR_RSP_CODE_A:
            /*handle error*/
            
            break;
        case SN001_ERR_RSP_CODE_B:
            /*handle error*/
            break;
        
    default:
        break;
    }

    
}