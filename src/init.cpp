#include <Arduino.h>

#include "cli_th.h"
#include "init.h"
#include "interrupts.h"
#include "config.h"
#include "hw.h"

TaskHandle_t read_serial_cli_th; //thread handler for CLI thread
TaskHandle_t process_state_ch_th;
volatile device_state_t current_state = CONTROLLER_STATE;

/**
 * @brief This thread is the first to run and initializes the system.
 * It sets up the hardware and starts the other threads. Depending on the active state
 * of the system, it will start particular threads.
 * @param parameter - The parameter passed to the task
 * @return None
 */
void init_p()
{
  int sensor_pin = 0;
  int controller_pin = 0;

  Serial.begin(BAUD_RATE);
  sleep(5);
  printf("init_p: Starting initialization...\n");

  pinMode(INT_STATE_PIN, INPUT);
  pinMode(INT_STATE_PIN_2, INPUT);

  sensor_pin = digitalRead(INT_STATE_PIN);
  controller_pin = digitalRead(INT_STATE_PIN_2);
  
  switch_state(sensor_pin, controller_pin); //force a check on the switch state

  // Attach interrupts to both pins to monitor state changes
  attachInterrupt(digitalPinToInterrupt(INT_STATE_PIN), has_state_changed, CHANGE);
  attachInterrupt(digitalPinToInterrupt(INT_STATE_PIN_2), has_state_changed, CHANGE);
  
  xTaskCreatePinnedToCore(process_state_change, "process_state_change", 10000, NULL, 1, &process_state_ch_th, 0);
  
  //CLI Thread creation.
  print_motd();
  xTaskCreatePinnedToCore(read_serial_cli, "read_serial_cli", 10000, NULL, 1, &read_serial_cli_th, 1);
}
