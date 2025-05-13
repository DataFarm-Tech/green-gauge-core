#include <Arduino.h>
#include "mutex_h.h"
#include "utils.h"

SemaphoreHandle_t rf95_mh;  // Mutex handle
SemaphoreHandle_t msg_queue_mh;  // Mutex handle
SemaphoreHandle_t seq_mh;  // Mutex handle

void init_mutex(device_state_t state)
{
    seq_mh = xSemaphoreCreateMutex();
    if (seq_mh == NULL)
    {
        DEBUG();
        PRINT_ERROR("Failed to create mutex for seq");
        return;
    }

    rf95_mh = xSemaphoreCreateMutex();
    if (rf95_mh  == NULL)
    {
        DEBUG();
        PRINT_ERROR("Failed to create mutex for rf95");
        return;
    }
    
    switch (state)
    {
        case CONTROLLER_STATE:
        
            msg_queue_mh = xSemaphoreCreateMutex();
            if (msg_queue_mh == NULL)
            {
                DEBUG();
                PRINT_ERROR("Failed to create mutex for msg queue");
            }
            
            break;
        case SENSOR_STATE:
            //TODO
            break;
        
        default:
            break;
    }
}