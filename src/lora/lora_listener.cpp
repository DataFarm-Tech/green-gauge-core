#include "lora_listener.h"
#include <RH_RF95.h>
#include <SPI.h>
#include "hw.h"

RH_RF95 rf95(RFM95_NSS, RFM95_INT); // Create the rf95 obj

SemaphoreHandle_t rf95_mutex = NULL;

void lora_listener(void * param)
{
    while (1)
    {
        sleep(1); //doesnt do anything yet
        //this thread will add to the Q
    }
    
}