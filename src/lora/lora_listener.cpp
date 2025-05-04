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
                pack_def packet;

                if (rf95.recv(buf, &buf_len))
                {
                    packet = describe_packet(buf, buf_len);

                    if (packet.des_node == ID)
                    {
                        switch (current_state)
                        {
                            case CONTROLLER_STATE:
                                //post recieved data to Q
                                break;
                            case SENSOR_STATE:
                                //append data to buf, and relay (decrement ttl by 1)
                                //switch src and dest addresses before relaying
                                break;
                        
                        default:
                            break;
                        }
                    }
                    else
                    {
                        //packet not for me, relay and decrement by 1

                    }
                }
                
            }
            
            xSemaphoreGive(rf95_mh); // Release the mutex
        }

        sleep(1); //doesnt do anything yet
        //this thread will add to the Q
    }
}

// void relay_packet(uint8_t * buff)
// {

// }


// void relay_packet(uint8_t * buff, )
// {
//     //extract ttl from buff

//     //if ttl != 1
//     //  extract cache from buff

//     //  if (!hash_cache_contains(buff.cache))
//     //      add_hash_cache(buff.cache)
//     //      decrement ttl by 1
//     //      set new ttl in packet
//     //      send_packet();
//extract sha256hash from buf.
                    // if (!hash_cache_contains(sha256Hash)) {
                    //     hash_cache_add(sha256Hash);
                    //      send packet;
                    // }
                    //else:
                    //  drop the packet
// }

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
