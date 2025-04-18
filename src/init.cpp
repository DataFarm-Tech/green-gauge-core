#include <stdio.h>
#include "cli_data.h"
#include "init.h"
#include <Arduino.h>

TaskHandle_t read_serial_cli_th; //thread handler for lora listen thread

/**
 * @brief This thread is the first to run and initializes the system.
 * It sets up the hardware and starts the other threads. Depending on the active state
 * of the system, it will start particular threads.
 * @param parameter - The parameter passed to the task
 * @return None
 */
void init_p()
{
    Serial.begin(115200);
    sleep(4);
    printf("init_p: Starting initialization...\n");
    print_motd();
    xTaskCreatePinnedToCore(read_serial_cli, "read_serial_cli", 10000, NULL, 1, &read_serial_cli_th, 0); //create lora listen thread

    //Call threads: CLI
    //Interupt, check state
    //Depending on state, start threads
    //LoRa, HTTP

}