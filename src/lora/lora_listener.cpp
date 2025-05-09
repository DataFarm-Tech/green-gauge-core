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
        if (xSemaphoreTake(rf95_mh, portMAX_DELAY) == pdTRUE)
        {
            if (rf95.available())
            {
                uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
                uint8_t buf_len = sizeof(buf);

                if (rf95.recv(buf, &buf_len))
                {
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
                                if (xSemaphoreTake(msg_queue_mh, portMAX_DELAY) == pdTRUE)
                                {
                                    internal_msg_q.push(describe_packet(buf, buf_len));
                                    xSemaphoreGive(msg_queue_mh);
                                }
                                
                                break;
                            case SENSOR_STATE:
                                //reset ttl to the num_nodes
                                buf[ADDRESS_SIZE * 2 + 1] = buf[ADDRESS_SIZE * 2];
                                swap_src_dest_addresses(buf);
                                read_rs485(buf, buf_len);
                                send_packet(buf, buf_len); // Send the packet
                                break;
                        
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



void send_packet(uint8_t* packet_to_send, uint8_t packet_len)
{
    if (xSemaphoreTake(rf95_mh, portMAX_DELAY) == pdTRUE)
    {
        if (!rf95.send(packet_to_send, packet_len)) 
        {
            PRINT_STR("Packet failed");
        }

        xSemaphoreGive(rf95_mh); // Release the mutex
    }
    
}
