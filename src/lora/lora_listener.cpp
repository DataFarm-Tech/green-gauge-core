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

/**
 * @brief The following thread listens for incoming
 * LoRa messages. It checks whether the packet is meant for itself.
 * @param param
 */
void lora_listener(void * param)
{
    while (1)
    {
        if (xSemaphoreTake(rf95_mh, portMAX_DELAY) == pdTRUE) // Lock the mutex before accessing the rf95 mutex
        {
            if (rf95.available())
            {
                uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
                uint8_t buf_len = sizeof(buf);
                packet packet;

                if (rf95.recv(buf, &buf_len))
                {
                    packet = describe_packet(buf, buf_len);

                    if (packet.des_node == ID)
                    {
                        printf("Packet from %s to %s\n", packet.src_node, packet.des_node);
                        
                        switch (current_state)
                        {
                            case CONTROLLER_STATE:
                                //post recieved data to Q, do not create new packet
                                break;
                            case SENSOR_STATE:
                                //append data to buf, and relay (decrement ttl by 1)
                                //switch src and dest addresses before relaying
                                break;
                        
                        default:
                            break;
                        }
                    }
                }
            }
            
            xSemaphoreGive(rf95_mh);
        }

        sleep(1);
    }
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
