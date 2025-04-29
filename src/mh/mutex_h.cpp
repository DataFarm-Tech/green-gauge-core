#include <Arduino.h>
#include "mutex_h.h"
#include "utils.h"

SemaphoreHandle_t ntp_client_mh;  // Mutex handle
SemaphoreHandle_t rf95_mh;  // Mutex handle
SemaphoreHandle_t msg_queue_mh;  // Mutex handle

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

            // //init msg queue mutex
            msg_queue_mh = xSemaphoreCreateMutex();
            if (msg_queue_mh == NULL)
            {
                DEBUG();
                PRINT_ERROR("Failed to create mutex for msg queue");
            }else{
                printf("msg queue mutex created\n");
            }
            
            break;
        case SENSOR_STATE:
            //TODO: init sensor mutex's

            // //TODO REMOVE///////////
            // // //init msg queue mutex
            // msg_queue_mh = xSemaphoreCreateMutex();
            // if (msg_queue_mh == NULL)
            // {
            //     DEBUG();
            //     PRINT_ERROR("Failed to create mutex for msg queue");
            // }else{
            //     printf("msg queue mutex created\n");
            // }
            // ///////////////////////
            break;
        
        default:
            break;
    }
}