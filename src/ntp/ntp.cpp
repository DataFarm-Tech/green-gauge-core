#include <NTPClient.h>       // NTPClient library for getting time
#include <WiFiUdp.h>         // UDP library for communication
#include <WiFi.h>
#include "ntp.h"
#include "utils.h"
#include "mh/mutex_h.h"

#define TIMEZONE_OFFSET 0 //UTC timezone
#define UPDATE_INTERVAL 60000
#define NTP_SERVER_POOL "pool.ntp.org"
#define TIMEOUT_MS 60000

typedef enum {
    UNDEFINED_STATE_SYS = 0,
    START_STATE = 1,
    STOP_STATE = 2
} sys_time_state_t;

WiFiUDP udp;
NTPClient time_client(udp, NTP_SERVER_POOL, TIMEZONE_OFFSET, UPDATE_INTERVAL);  // define and init ntp client
sys_time_state_t active_time_state = UNDEFINED_STATE_SYS;

/**
 * @brief The following function starts the sys time. This time is used
 * to track when to call app_main.
 */
bool start_sys_time()
{
    if (active_time_state != START_STATE)
    {
        if (WiFi.status() != WL_CONNECTED) 
        {
            return false;
        }
        
        // Lock the mutex before starting the time_client
        if (xSemaphoreTake(ntp_client_mh, portMAX_DELAY) == pdTRUE)
        {
            time_client.begin();
            active_time_state = START_STATE;

            xSemaphoreGive(ntp_client_mh); // Release the mutex
        }
    
        return true;
    }

    return true;
    
}

/**
 * @brief The function below retrieves the sys time. This is used by the app_main threads
 * and the CLI.
 * @details Before getting the sys time, the function checks whether internet is working.
 */
bool get_sys_time(uint32_t * currentTime)
{
    switch (active_time_state)
    {
        case STOP_STATE:
        case UNDEFINED_STATE_SYS:
            printf("Unable to retrieve NTP time\n");
            return false;
            break;
        case START_STATE:
            if (WiFi.status() != WL_CONNECTED) 
            {
                return false;
            }

             // Lock the mutex before updating the time
             if (xSemaphoreTake(ntp_client_mh, portMAX_DELAY) == pdTRUE)
             {
                 time_client.update();
                 *currentTime = time_client.getEpochTime();
                 xSemaphoreGive(ntp_client_mh); // Release the mutex
             }
            return true;
            break;
        
    default:
        break;
    }

    return false;
}

/**
 * @brief The following function stops the time_client.
 * This is called when switching between a controller to a sensor.
 */
void close_sys_time()
{
    if (active_time_state == START_STATE)
    {
        // Lock the mutex before stopping the time_client
        if (xSemaphoreTake(ntp_client_mh, portMAX_DELAY) == pdTRUE)
        {
            udp.stop();
            PRINT_INFO("Closing NTP time");
            active_time_state = STOP_STATE;

            xSemaphoreGive(ntp_client_mh); // Release the mutex
        }
    }
}