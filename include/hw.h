#pragma once
/*
* RS485 module pin definitions
*/
#define RS485_STATUS_OK 9
#define RS485_STATUS_ERROR 10
#define RS485_RX 16
#define RS485_TX 17
#define RS485_RTS 4
#define RS485_BAUD 4800

/*
* RFM95W module pin definitions
*/
#define RFM95_NSS 5 // NSS/CS P
#define RFM95_RST 14 // RST
#define RFM95_INT 27 // Interrupt/DIO0
#define RFM95_MOSI 23
#define RFM95_MISO 19
#define RFM95_SCK 18
#define RF95_FREQ 915.0 // DO NOT CHANGE FROM 915MHZ

void rfm95w_setup();
void wifi_connect();
void wifi_disconnect(bool erase_creds);