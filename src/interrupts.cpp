#include "interrupts.h"
#include "init.h"
#include "utils.h"
#include <Arduino.h>

volatile unsigned long last_interrupt_time = 0;
volatile bool state_change_detected = false;

void switch_state(const int sensor_pin, const int controller_pin);

/**
 * @brief Interrupt service routine for state change detection
 * Keeps processing minimal to ensure quick execution
 */
void IRAM_ATTR has_state_changed() {
  // Simple debouncing to prevent multiple triggers
  if ((millis() - last_interrupt_time) > 50) {
    state_change_detected = true;
  }
  last_interrupt_time = millis();
}

/**
 * @brief Process state changes based on pin conditions
 * This is called from the main loop to handle actual state transitions
 */
void process_state_change(void *param) {
  int sensor_pin = 0;
  int controller_pin = 0;
  while (1)
  {
    if(state_change_detected)
    {
      sensor_pin = digitalRead(INT_STATE_PIN);
      controller_pin = digitalRead(INT_STATE_PIN_2);
      
      switch_state(sensor_pin, controller_pin);
      
      state_change_detected = false;
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);  // Add this line to yield every 10ms
  }

  vTaskDelete(NULL); // Delete the task when done
  
}

/**
 * @brief Switch the state of the system based on pin readings
 * @param sensor_pin - The state of the sensor pin
 * @param controller_pin - The state of the controller pin
 */
void switch_state(const int sensor_pin, const int controller_pin)
{
  if (sensor_pin == LOW && controller_pin == HIGH) 
  {
    if(current_state == SENSOR_STATE) return;
    PRINT_INFO("Switching to sensor state\n");
    current_state = SENSOR_STATE;
  } 
  else 
  {
    if(current_state == CONTROLLER_STATE) return;
    PRINT_INFO("Switching to controller state\n");
    current_state = CONTROLLER_STATE;
  }  
}