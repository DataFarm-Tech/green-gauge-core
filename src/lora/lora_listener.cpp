#include "lora_listener.h"
#include "mh/mutex_h.h"
#include <RH_RF95.h>
#include <SPI.h>
#include "hw.h"
#include "utils.h"
#include "config.h"
#include "pack_def/pack_def.h"
#include "hash_cache/hash_cache.h"

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
                    /**
                     * If the TTL is 0, 
                     * it means that the packet should be dropped.
                     */
                    if (buf[ADDRESS_SIZE * 2 + 1] == 0)
                    {
                        xSemaphoreGive(rf95_mh);
                        continue;
                    }

                    if (hash_cache_contains(buf))
                    {
                        xSemaphoreGive(rf95_mh);
                        continue;
                    }
                    else
                    {
                        hash_cache_add(buf);
                    }
                    
                    
                    if (memcmp(buf, ID, ADDRESS_SIZE) == MEMORY_CMP_SUCCESS)
                    {
                        switch (current_state)
                        {
                            case CONTROLLER_STATE:
                                // Lock the mutex before accessing the queue
                                if (xSemaphoreTake(msg_queue_mh, portMAX_DELAY) == pdTRUE)
                                {
                                    internal_msg_q.push(describe_packet(buf, buf_len)); // Push the packet to the queue
                                    xSemaphoreGive(msg_queue_mh); // Release the mutex
                                }
                                
                                break;
                            case SENSOR_STATE:
                                //reset ttl to the num_nodes
                                buf[ADDRESS_SIZE * 2 + 1] = buf[ADDRESS_SIZE * 2];
                                swap_src_dest_addresses(buf);
                                
                                //read_sensor(buf, buf_len);
                                send_packet(buf, buf_len); // Send the packet
                                break;
                        
                        default:
                            break;
                        }
                    }
                    else
                    {
                        //relay the packet
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
    printf("OK\n");
    if (xSemaphoreTake(rf95_mh, portMAX_DELAY) == pdTRUE)
    {
        if (!rf95.send(packet_to_send, packet_len)) 
        {
            PRINT_STR("Packet failed");
        }

        xSemaphoreGive(rf95_mh); // Release the mutex
    }
    printf("OK\n");
}
