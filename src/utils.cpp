#include <Arduino.h>
#include <WiFi.h>
#include <ESP32Ping.h>
#include "utils.h"
#include "config.h"
#include "msg_queue.h"

/**
 * @brief Constructs the endpoint URL.
 * @param endpoint The endpoint to be appended to the URL.
 * @return char* - The constructed endpoint.
 */
char* constr_endp(const char* endpoint)
{
    static char endp[200];  // Static buffer to persist after function returns

    if (strncmp(endpoint, "", sizeof(endpoint)) == 0)
    {
        PRINT_STR("Endpoint is empty");
        return NULL;
    }
    
    snprintf(endp, sizeof(endp), "http://%s:%s%s", API_HOST, API_PORT, endpoint);
    return endp;
}


void checkInternet() {
    const char* googleDNS = "google.com"; // Google DNS for internet check

    // 1. Loop until WiFi connects
    while (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi disconnected. Reconnecting...");
        WiFi.disconnect();
        WiFi.reconnect();
        delay(3000); // Prevents watchdog trigger
    }
    Serial.println("WiFi connected!");

    // 2. Loop until Google pings (confirms internet)
    while (true) {
        Serial.println("Pinging Google...");
        if (Ping.ping(googleDNS, 1)) {
            Serial.println("Google ping success! Internet OK.");
            break;
        }
        Serial.println("Google ping failed. Retrying in 3s...");
        delay(3000);
    }

    // 3. Loop until your server pings
    while (true) {
        Serial.println("Pinging df server server...");
        if (Ping.ping(API_HOST, 1)) {
            Serial.println("Server ping success! Ready to proceed.");
            break;
        }
        Serial.println("Server ping failed. Retrying in 3s...");
        delay(3000);
    }
}