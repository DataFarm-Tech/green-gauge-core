#pragma once

#include <Arduino.h>
#include <RH_RF95.h>
#include <SPI.h>

extern RH_RF95 rf95;

void lora_listener(void * param);
int send_packet(uint8_t* packet_to_send, uint8_t packet_len);