#include <stdio.h>
#include <Arduino.h>
#include "init.h"

TaskHandle_t init_p_th; //thread handler for lora listen thread

void setup()
{
    xTaskCreatePinnedToCore(init_p, "init_p", 10000, NULL, 1, &init_p_th, 0); //create lora listen thread
}