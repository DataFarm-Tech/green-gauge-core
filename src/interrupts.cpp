#include <Arduino.h>

#include "interrupts.h"
#include "th_handler.h"
#include "init.h"
#include "utils.h"
#include "hw.h"
#include "main_app.h"


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
  /**
   * Can we trigger a interrupt by default on startup?
   * This ensures that all the init related functions are called, on startup even if
   * the state has not changed???
   */
  
  if (sensor_pin == LOW && controller_pin == HIGH) 
  {
    if (current_state != SENSOR_STATE)
    {
        PRINT_INFO("Switching to sensor state\n");
        current_state = SENSOR_STATE;
        
        delete_th(lora_listener_th);      
        delete_th(main_app_th);
        delete_th(http_th);      
        
        //do rs485 pin setup
    }
    
  } 
  else 
  {
    if (current_state != CONTROLLER_STATE)
    {
      PRINT_INFO("Switching to controller state\n");
      current_state = CONTROLLER_STATE;
      
      delete_th(lora_listener_th);      
      delete_th(main_app_th);
      delete_th(http_th);

      sleep(2);

      create_th(main_app, "main_app", MAIN_APP_TH_STACK_SIZE, &main_app_th, 1);

  
      //create http thread
      //create main_app thread
    }
    
  }  
//   rfm95w_setup();
  //create lora listener thread
}