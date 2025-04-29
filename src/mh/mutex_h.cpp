#include <Arduino.h>
#include "mutex_h.h"
#include "utils.h"

SemaphoreHandle_t ntp_client_mh;  // Mutex handle
SemaphoreHandle_t rf95_mh;  // Mutex handle

void init_mutex(device_state_t state)
{
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

            ntp_client_mh = xSemaphoreCreateMutex();
            if (ntp_client_mh == NULL)
            {
                DEBUG();
                PRINT_ERROR("Failed to create mutex for time_client");
            }
            
            break;
        case SENSOR_STATE:
            //TODO: init sensor mutex's
            break;
        
        default:
            break;
    }
}