#pragma once

#include <Arduino.h>
#include <RH_RF95.h>
#include <SPI.h>

// Declare rf95 as an external object
extern RH_RF95 rf95;

// Declare a mutex for protecting rf95
extern SemaphoreHandle_t rf95_mutex;

void lora_listener(void * param);