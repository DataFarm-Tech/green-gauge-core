#pragma once
/*
* RS485 module pin definitions
*/

#define RS485_RX 41
#define RS485_TX 42
#define RS485_RTS 18
#define RS485_BAUD 4800

/*
* RFM95W module pin definitions
*/
#define RFM95_NSS 10 // NSS/CS P
#define RFM95_RST 4 // RST
#define RFM95_INT 2 // Interrupt/DIO0
#define RFM95_MOSI 11
#define RFM95_MISO 13
#define RFM95_SCK 12
#define RF95_FREQ 915.0 // DO NOT CHANGE FROM 915MHZ

/*
* LED pin definitions
*/
#define LORA_LED_GREEN 1
#define LORA_LED_RED 5
#define WIFI_LED_GREEN 6
#define WIFI_LED_RED 7
#define SENSOR_LED_GREEN 8
#define SENSOR_LED_RED 9

void rfm95w_setup();
void wifi_connect();
void wifi_disconnect(bool erase_creds);