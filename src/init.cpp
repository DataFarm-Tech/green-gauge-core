#include <stdio.h>
#include "init.h"
#include <Arduino.h>

/**
 * @brief This thread is the first to run and initializes the system.
 * It sets up the hardware and starts the other threads. Depending on the active state
 * of the system, it will start particular threads.
 * @param parameter - The parameter passed to the task
 * @return None
 */
void init_p()
{
    printf("init_p: Starting initialization...\n");

    //Call threads: CLI
    //Interupt, check state
    //Depending on state, start threads
    //LoRa, HTTP

}