#include <stdio.h>
#include <Arduino.h>
#include "init.h"

TaskHandle_t init_p_th; //thread handler for lora listen thread

void setup()
{
    init_p();
}

void loop()
{
    
}