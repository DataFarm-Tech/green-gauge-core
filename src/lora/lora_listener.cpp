#include "lora_listener.h"
#include "mh/mutex_h.h"
#include <RH_RF95.h>
#include <SPI.h>
#include "hw.h"
#include "utils.h"
#include "config.h"
#include "pack_def/pack_def.h"
#include "hash_cache/hash_cache.h"
#include "rs485/rs485_interface.h"
#include "crypt/crypt.h"
#include "msg_queue.h"

RH_RF95 rf95(RFM95_NSS, RFM95_INT); // Create the rf95 obj


void swap_src_dest_addresses(uint8_t buffer[]);


/**
 * @brief The following thread listens for incoming
 * LoRa messages. It checks whether the packet is meant for itself.
 * @param param
 */
void lora_listener(void * param)
{
    while (1)
    {
        if (rf95.available())
        {
            if (xSemaphoreTake(rf95_mh, portMAX_DELAY) == pdTRUE)
            {
                uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
                uint8_t buf_len = sizeof(buf);

                if (rf95.recv(buf, &buf_len))
                {
                    if (buf_len < SN001_ERR_RSP_LEN) /** This is the smallest msg len */
                    {
                        xSemaphoreGive(rf95_mh);
                        continue;
                    }
                    
                    /** Before doing anything with buffer, we check that 
                     * the buffer is not empty and the length is correct.
                     * We also check that the buffer is not corrupted.
                     * We do this by calculating the CRC of the buffer and comparing it
                     * to the CRC stored in the last two bytes of the buffer.
                     */
                    if (!validate_crc(buf, buf_len))
                    {
                        xSemaphoreGive(rf95_mh);
                        continue;
                    }
                    
                    /**
                     * If the TTL is 0, 
                     * it means that the packet should be dropped.
                     */
                    if (buf[ADDRESS_SIZE * 2 + 1] == 0)
                    {
                        xSemaphoreGive(rf95_mh);
                        continue;
                    }
                    /**
                     * If the hash cache contains the hash of the packet,
                     * it means that the packet has already been processed.
                     * We drop the packet. If it hasnt been processed,
                     * we add it to the hash cache.
                     * We do this to prevent processing the same packet multiple times.
                     * This is important because the packet may be relayed multiple times
                     * and we dont want to process the same packet multiple times.
                     */

                    if (hash_cache_contains(buf))
                    {
                        xSemaphoreGive(rf95_mh);
                        continue;
                    }
                    else
                    {
                        hash_cache_add(buf);
                    }
                    
                    if (memcmp(buf, ID, ADDRESS_SIZE) == MEMORY_CMP_SUCCESS) /** Checks ownership of packet */
                    {
                        switch (current_state)
                        {
                            case CONTROLLER_STATE:
                            {
                                if (xSemaphoreTake(msg_queue_mh, portMAX_DELAY) == pdTRUE)
                                {
                                    internal_msg_q.push(describe_packet(buf, buf_len));
                                    xSemaphoreGive(msg_queue_mh);
                                }
                                
                                break;
                            }
                            case SENSOR_STATE:
                            {
                                char rs485_buf[DATA_SIZE];
                                uint8_t rc = read_rs485(rs485_buf, DATA_SIZE);

                                switch (rc)
                                {
                                    case SN001_SUC_RSP_CODE:
                                    {
                                        sn001_rsp pkt_resp;
                                        uint8_t packet_success[SN001_SUC_RSP_LEN];

                                        memcpy(pkt_resp.src_node, buf, ADDRESS_SIZE);
                                        memcpy(pkt_resp.des_node, buf + ADDRESS_SIZE, ADDRESS_SIZE);
                                        memcpy(pkt_resp.data, rs485_buf, DATA_SIZE);
                                        pkt_resp.ttl = buf[ADDRESS_SIZE * 2];

                                        pkt_sn001_rsp(packet_success, &pkt_resp, seq_id);
                                        send_packet(packet_success, sizeof(packet_success));
                                        break;
                                    }
                                    case SN001_ERR_RSP_CODE_A:
                                    {
                                        sn001_err_rsp pkt_err;
                                        uint8_t packet_err[SN001_ERR_RSP_LEN];

                                        memcpy(pkt_err.src_node, buf, ADDRESS_SIZE);
                                        memcpy(pkt_err.des_node, buf + ADDRESS_SIZE, ADDRESS_SIZE);
                                        pkt_err.ttl = buf[ADDRESS_SIZE * 2];
                                        pkt_err.err_code = rc;

                                        pkt_sn001_err_rsp(packet_err, &pkt_err, seq_id);
                                        send_packet(packet_err, sizeof(packet_err));
                                        break;
                                    }
                                    default:
                                        break;
                                }

                                if (xSemaphoreTake(seq_mh, portMAX_DELAY) == pdTRUE)
                                {
                                    seq_id++;
                                    xSemaphoreGive(seq_mh);
                                }

                                break;            
                            }
                        default:
                            break;
                        }
                    }
                    else
                    {
                        buf[ADDRESS_SIZE * 2 + 1] -= 1; // Decrement TTL
                        send_packet(buf, buf_len); // Send the packet
                    }
                }
                }
                
                xSemaphoreGive(rf95_mh);
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);

        sleep(1);
    }
}


/**
 * @brief Swaps the source and destination addresses in a given buffer.
 *        The function assumes that the first ADDRESS_SIZE bytes represent
 *        the source address, and the next ADDRESS_SIZE bytes represent 
 *        the destination address.
 * 
 * @param buffer Pointer to the buffer containing the addresses.
 */
void swap_src_dest_addresses(uint8_t buffer[])
{
    uint8_t tmp[ADDRESS_SIZE];
    memcpy(tmp, buffer, ADDRESS_SIZE);
    memcpy(buffer, buffer + ADDRESS_SIZE, ADDRESS_SIZE);
    memcpy(buffer + ADDRESS_SIZE, tmp, ADDRESS_SIZE);
}


/**
 * @brief Sends a packet using the RF95 module.
 * 
 * @param packet_to_send Pointer to the packet to be sent.
 * @param packet_len Length of the packet to be sent.
 */
void send_packet(uint8_t* packet_to_send, uint8_t packet_len)
{
    if (xSemaphoreTake(rf95_mh, portMAX_DELAY) == pdTRUE)
    {
        while (!rf95.send(packet_to_send, packet_len)) 
        {
            PRINT_STR("Packet failed");
        }

        xSemaphoreGive(rf95_mh); // Release the mutex
    }
    
}
