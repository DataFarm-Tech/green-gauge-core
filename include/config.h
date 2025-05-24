#pragma once

/**
 * Adding a macro to en/dis LEDS 
 */
#define LED_PINS_EN 1


/**
 * Adding a macro to en/dis the BMS LEDS 
 */
#define BMS_PINS_EN 1

/** 
 * Adding this macro to enable/disable the Wifi
 * model's functionality. 
 */
#define WIFI_EN 1

/** 
 * Adding this macro to disable the CLI when deploying
 * products. 
 */
#define CLI_EN 1

/**
 * Adding this macro to en/dis for development
 * reasons, since there is no interupt pins on the 
 * devices with LoRa modules.
 */
#define LORA_EN 1

/**
 * Adding this macro to disable/enable the interupt functionality
 * If it is disabled, then it chooses the default state.
 */
#define INT_STATE_EN 1

#if INT_STATE_EN == 0
    #define CONTROLLER_DEF_STATE 1
#endif 


/**
 * Adding this macro to silence all output
 * including CLI and print statements.
 */
#define SILENCE_OUTPUT 1

/**
 * This macro is for testing and debugging reasons.
 * If this is set to 1, then certain print statements
 * will execute.
 */
#define DEBUG_MODE 0

#define ID "tcon12" //if ID is never defined in compilation process, it will be set to "UNDEFINED"
#define BAUD_RATE 115200

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

#define API_HOST "datafarmapi.duckdns.org" //DEFAULT HOST
#define API_PORT "80" //DEFAULT PORT
#define TX_POST_ENDPOINT "/controller/add/data" //ADD DATA ENDPOINT
#define TX_ACT_ENDPOINT "/controller/activate"
#define TX_GET_ENDPOINT "/user/get/node"
#define TX_POST_ENDPOINT_NOTIFY "/notifications/add"

#define MEMORY_CMP_SUCCESS 0