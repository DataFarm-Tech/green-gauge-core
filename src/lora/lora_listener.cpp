#include "lora_listener.h"
#include "mh/mutex_h.h"
#include <RH_RF95.h>
#include <SPI.h>
#include "hw.h"

RH_RF95 rf95(RFM95_NSS, RFM95_INT); // Create the rf95 obj

/**
 * @brief The following thread listens for incoming
 * LoRa messages. It checks whether the packet is meant for itself.
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

                if (rf95.recv(buf, &buf_len))
                {
                    if (memcmp(buf, ID, ADDRESS_SIZE) == MEMORY_CMP_SUCCESS)
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