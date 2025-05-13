#include "lora_listener.h"
#include "mutex_h.h"
#include <RH_RF95.h>
#include <SPI.h>
#include "hw.h"
#include "utils.h"
#include "config.h"
#include "pack_def.h"
#include "hash_cache.h"
#include "rs485_interface.h"
#include "crypt.h"
#include "msg_queue.h"
#include "err_handle.h"

RH_RF95 rf95(RFM95_NSS, RFM95_INT); // Create the rf95 obj


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
                                cn001_handle_packet(buf, buf_len);
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
                                        /** Ensure green LED is set back to HIGH, when rs485 comms work. */
                                        err_led_state(SENSOR, INT_STATE_OK);
                                        sn001_rsp pkt_resp;
                                        uint8_t packet_success[SN001_SUC_RSP_LEN];

                                        memcpy(pkt_resp.src_node, buf, ADDRESS_SIZE); /** Copy the des_node of buffer into the src_node of new packet */
                                        memcpy(pkt_resp.des_node, buf + ADDRESS_SIZE, ADDRESS_SIZE); /** Copy the src_node of buffer into the des_node of new packet */
                                        memcpy(pkt_resp.data, rs485_buf, DATA_SIZE);
                                        pkt_resp.ttl = buf[ADDRESS_SIZE * 2];

                                        pkt_sn001_rsp(packet_success, &pkt_resp, seq_id);
                                        
                                        if (send_packet(packet_success, sizeof(packet_success)) == EXIT_SUCCESS)
                                        {
                                            if (xSemaphoreTake(seq_mh, portMAX_DELAY) == pdTRUE)
                                            {
                                                seq_id++;
                                                xSemaphoreGive(seq_mh);
                                            }
                                        }
                                        
                                        break;
                                    }
                                    case SN001_ERR_RSP_CODE_A:
                                    {
                                        /** Ensure RED LED is set too high, if the read from sensor fails */
                                        err_led_state(SENSOR, INT_STATE_ERROR);
                                        sn001_err_rsp pkt_err;
                                        uint8_t packet_err[SN001_ERR_RSP_LEN];

                                        memcpy(pkt_err.src_node, buf, ADDRESS_SIZE);
                                        memcpy(pkt_err.des_node, buf + ADDRESS_SIZE, ADDRESS_SIZE);
                                        pkt_err.ttl = buf[ADDRESS_SIZE * 2];
                                        pkt_err.err_code = rc;

                                        pkt_sn001_err_rsp(packet_err, &pkt_err, seq_id);
                                        
                                        if (send_packet(packet_err, sizeof(packet_err)) == EXIT_SUCCESS)
                                        {
                                            if (xSemaphoreTake(seq_mh, portMAX_DELAY) == pdTRUE)
                                            {
                                                seq_id++;
                                                xSemaphoreGive(seq_mh);
                                            }
                                        }

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
    }
}


/**
 * @brief Sends a packet using the RF95 module.
 * 
 * @param packet_to_send Pointer to the packet to be sent.
 * @param packet_len Length of the packet to be sent.
 */
int send_packet(uint8_t* packet_to_send, uint8_t packet_len)
{
    if (xSemaphoreTake(rf95_mh, portMAX_DELAY) == pdTRUE)
    {
        if (!rf95.send(packet_to_send, packet_len))
        {
            err_led_state(LORA, INT_STATE_ERROR);
            return EXIT_FAILURE;
        }

        err_led_state(LORA, INT_STATE_OK);

        xSemaphoreGive(rf95_mh); // Release the mutex
    }

    return EXIT_SUCCESS;
}
