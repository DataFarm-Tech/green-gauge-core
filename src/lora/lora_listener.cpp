#include "lora_listener.h"
#include "mh/mutex_h.h"
#include <RH_RF95.h>
#include <SPI.h>
#include "hw.h"

RH_RF95 rf95(RFM95_NSS, RFM95_INT); // Create the rf95 obj

void lora_listener(void * param)
{
    while (1)
    {
        // Lock the mutex before starting the time_client
        if (xSemaphoreTake(rf95_mh, portMAX_DELAY) == pdTRUE)
        {
            if (rf95.available())
            {
                uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
                uint8_t buf_len = sizeof(buf);

                if (rf95.recv(buf, &buf_len))
                {
                    printf("Recieved Bytes\n");
                    /**
                     * 
                     * if packet is for me
                     * then if i am controller (post to Q)
                     * then if i am sensor (retrieve data from sensor) and relay (decrement ttl -1)
                     * else if packet is not for me
                     * then relay (decrement ttl -1)
                     */
                }
                
            }
            
            xSemaphoreGive(rf95_mh); // Release the mutex
        }

        sleep(1); //doesnt do anything yet
        //this thread will add to the Q
    }
    
}