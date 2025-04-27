#include <NTPClient.h>       // NTPClient library for getting time
#include <WiFiUdp.h>         // UDP library for communication
#include <WiFi.h>
#include "ntp.h"
#include "utils.h"

#define TIMEZONE_OFFSET 0 //UTC timezone
#define UPDATE_INTERVAL 60000
#define NTP_SERVER_POOL "pool.ntp.org"
#define TIMEOUT_MS 60000

WiFiUDP udp;
NTPClient time_client(udp, NTP_SERVER_POOL, TIMEZONE_OFFSET, UPDATE_INTERVAL);  // define and init ntp client

/**
 * @brief The following function starts the sys time. This time is used
 * to track when to call app_main.
 */
bool start_sys_time()
{
    if (WiFi.status() != WL_CONNECTED) 
    {
        return false;
    }
    
    time_client.begin();
    return true;
}

/**
 * @brief The function below retrieves the sys time. This is used by the app_main threads
 * and the CLI.
 * @details Before getting the sys time, the function checks whether internet is working.
 */
bool get_sys_time(uint32_t * currentTime)
{
    if (WiFi.status() != WL_CONNECTED) 
    {
        return false;
    }

    time_client.update();
    *currentTime = time_client.getEpochTime();
    return true;
}