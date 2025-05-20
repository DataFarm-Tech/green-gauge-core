#pragma once

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
 * Adding this macro to en/dis for development
 * reasons, since there is no interupt pins on the 
 * devices with LoRa modules.
 */
#define INT_STATE_EN 0


/**
 * Adding this macro to en/dis for development
 * reasons, since there is no interupt pins on the 
 * devices with LoRa modules.
 */
#define DEBUG_MODE 1

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
