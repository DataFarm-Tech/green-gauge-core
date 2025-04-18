#include <stdio.h>
#include <Arduino.h>

#include "cli_data.h"
#include "init.h"
#include "interrupts.h"
#include "config.h"

typedef enum
{
    SENSOR_STATE = 1,
    CONTROLLER_STATE = 2,
} device_state_t;

TaskHandle_t read_serial_cli_th; //thread handler for CLI thread

volatile unsigned long debounceTimerPin1 = 0;
volatile unsigned long debounceTimerPin2 = 0;
volatile device_state_t current_state = SENSOR_STATE;
volatile bool stateChangeFlag = false;
const unsigned int DEBOUNCE_DELAY = 50;

/**
 * @brief This thread is the first to run and initializes the system.
 * It sets up the hardware and starts the other threads. Depending on the active state
 * of the system, it will start particular threads.
 * @param parameter - The parameter passed to the task
 * @return None
 */
void init_p()
{
    Serial.begin(BAUD_RATE);
    sleep(5);
    printf("init_p: Starting initialization...\n");
    
    //CLI Thread creation.
    print_motd();
    xTaskCreatePinnedToCore(read_serial_cli, "read_serial_cli", 10000, NULL, 1, &read_serial_cli_th, 0); //create lora listen thread
    
//     pinMode(INT_STATE_PIN, INPUT);
//     pinMode(INT_STATE_PIN_2, INPUT);
    
//     // attachInterrupt(INT_STATE_PIN, switch_state, RISING);
//     // attachInterrupt(INT_STATE_PIN_2, switch_state, RISING);
//     // For your init.cpp file
  
//   // Lambda-based debounce for first pin
//   attachInterrupt(INT_STATE_PIN, []() IRAM_ATTR {
//     unsigned long currentTime = millis();
//     if ((currentTime - debounceTimerPin1) >= DEBOUNCE_DELAY) {
//       current_state = SENSOR_STATE;
//       stateChangeFlag = true;
//       debounceTimerPin1 = currentTime;
//     }
//     Serial.println(current_state);
//   }, RISING);
  
//   // Lambda-based debounce for second pin
//   attachInterrupt(INT_STATE_PIN_2, []() IRAM_ATTR {
//     unsigned long currentTime = millis();
//     if ((currentTime - debounceTimerPin2) >= DEBOUNCE_DELAY) {
//       current_state = CONTROLLER_STATE;
//       stateChangeFlag = true;
//       debounceTimerPin2 = currentTime;
//     }
//     Serial.println(current_state);
//   }, RISING);
}
