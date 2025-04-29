#include <Arduino.h>

#include "interrupts.h"
#include "th/th_handler.h"
#include "init.h"
#include "utils.h"
#include "hw.h"
#include "main_app/main_app.h"
#include "http/https_comms.h"
#include "msg_queue.h"
#include "lora/lora_listener.h"
#include "ntp/ntp.h"
#include "mh/mutex_h.h"

/**
 * Adding this macro to en/dis for development
 * reasons, since there is no interupt pins on the 
 * devices with LoRa modules.
 */
#define LORA_EN 1

volatile unsigned long last_interrupt_time = 0;
volatile bool state_change_detected = false;

void switch_state(const int sensor_pin, const int controller_pin);
void tear_down();

/**
 * @brief Interrupt service routine for state change detection
 * Keeps processing minimal to ensure quick execution
 */
void IRAM_ATTR has_state_changed() 
{
    if ((millis() - last_interrupt_time) > 50) 
    {
        state_change_detected = true;
    }
    
    last_interrupt_time = millis();
}

/**
 * @brief Process state changes based on pin conditions
 * This is called from the main loop to handle actual state transitions
 */
void process_state_change(void *param) 
{
    int sensor_pin = 0;
    int controller_pin = 0;
    while (1) 
    {
        if (state_change_detected) 
        {
            sensor_pin = digitalRead(INT_STATE_PIN);
            controller_pin = digitalRead(INT_STATE_PIN_2);
            
            switch_state(sensor_pin, controller_pin);
            
            state_change_detected = false;
        }
        
        vTaskDelay(10 / portTICK_PERIOD_MS);
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
    //TODO teardown clears the queue before the mutexes are intialized
    tear_down();
    
    if (sensor_pin == LOW && controller_pin == HIGH) 
    {
        if (current_state != SENSOR_STATE) 
        {
            PRINT_INFO("Switching to sensor state\n");
            current_state = SENSOR_STATE;
            
            // do rs485 pin setup
        }
    } 
    else 
    {
        if (current_state != CONTROLLER_STATE) 
        {
            PRINT_INFO("Switching to controller state\n");
            current_state = CONTROLLER_STATE;
            
            wifi_connect();

            init_mutex(current_state);
            
            create_th(main_app, "main_app", MAIN_APP_TH_STACK_SIZE, &main_app_th, 1);
            create_th(http_send, "http_th", HTTP_TH_STACK_SIZE, &http_th, 1); // core 0 is used for network related tasks
            
        }
    }

    #if LORA_EN == 1
        // rfm95w_setup();
        create_th(lora_listener, "lora_listener", LORA_LISTENER_TH_STACK_SIZE, &lora_listener_th, 1);
        //start lora thread
    #endif
}

/**
 * @brief The following function removes all threads and clears the 
 * queue
 * @return None
 */
void tear_down() 
{
    close_sys_time();

    delete_th(&lora_listener_th);      
    delete_th(&main_app_th);
    delete_th(&http_th);

    if (msg_queue_mh != NULL)
    {
        if (xSemaphoreTake(msg_queue_mh, portMAX_DELAY) == pdTRUE) {
            while (!internal_msg_q.empty()) internal_msg_q.pop();
            xSemaphoreGive(msg_queue_mh);
        }
    }

    sleep(2);
    return;
}
