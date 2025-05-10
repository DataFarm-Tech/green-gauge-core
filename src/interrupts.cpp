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
#include "eeprom/eeprom.h"
#include "hash_cache/hash_cache.h"
#include "mh/mutex_h.h"

#define TIMER_PRESCALER 80  // Prescaler value
#define TICKS_PER_SECOND 1000000
#define SECONDS_PER_HOUR 10

/**
 * Adding this macro to en/dis for development
 * reasons, since there is no interupt pins on the 
 * devices with LoRa modules.
 */
#define LORA_EN 1

const uint64_t alarm_value_hourly = (uint64_t)TICKS_PER_SECOND * SECONDS_PER_HOUR;
hw_timer_t * timer = NULL;
volatile bool hourly_timer_flag = false;
volatile unsigned long last_interrupt_time = 0;
volatile bool state_change_detected = false;

void switch_state(const int sensor_pin, const int controller_pin);
void tear_down();

/**
 * @brief Interrupt service routine for the timer
 * This function is called when the timer alarm goes off.
 * It sets the hourlyTimerFlag to true, indicating that
 * the timer has elapsed.
 */
void IRAM_ATTR on_hourly_timer() 
{
    hourly_timer_flag = true;
}


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
    int sensor_pin;
    int controller_pin;
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

/** TODO: double check this. */
bool is_key_set() {
    return config.api_key[0] != '\0';  // Check if the first character is not the null terminator
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
        if (current_state != SENSOR_STATE) 
        {
            tear_down(); /* Removes all threads, queues*/
            PRINT_INFO("Switching to sensor state\n");
            current_state = SENSOR_STATE;

            init_mutex(current_state);

            #if LORA_EN == 1
                // rfm95w_setup();
                hash_cache_init();
                create_th(lora_listener, "lora_listener", LORA_LISTENER_TH_STACK_SIZE, &lora_listener_th, 1);
            #endif
        }
    } 
    else 
    {
        if (current_state != CONTROLLER_STATE) 
        {
            tear_down(); /* Removes all threads, queues*/
            PRINT_INFO("Switching to controller state\n");
            current_state = CONTROLLER_STATE;
            
            wifi_connect(); /* Once controller starts it connects to wlan0*/
            
            if (!is_key_set()) /* Before proceeding key must exist, for http thread to use*/
            {
                printf("key is not set\n");
                DEBUG();
                activate_controller(); /* Retrieves a key from the API*/
            }
            
            get_nodes_list(); /* Get's the node_list from the API and saves to global variable.*/
            
            init_mutex(current_state);
            
            create_th(main_app, "main_app", MAIN_APP_TH_STACK_SIZE, &main_app_th, 1);
            create_th(http_send, "http_th", HTTP_TH_STACK_SIZE, &http_th, 1);

            #if LORA_EN == 1
                // rfm95w_setup();
                hash_cache_init();
                create_th(lora_listener, "lora_listener", LORA_LISTENER_TH_STACK_SIZE, &lora_listener_th, 1);
            #endif
            
        }
    }
}

/**
 * @brief The following function removes all threads and clears the 
 * queue
 * @return None
 */
void tear_down() 
{
    delete_th(&lora_listener_th);      
    delete_th(&main_app_th);
    delete_th(&http_th);

    if (msg_queue_mh != NULL)
    {
        if (xSemaphoreTake(msg_queue_mh, portMAX_DELAY) == pdTRUE) 
        {
            while (!internal_msg_q.empty()) internal_msg_q.pop();
            xSemaphoreGive(msg_queue_mh);
        }
        vSemaphoreDelete(msg_queue_mh);
        msg_queue_mh = NULL;
    }

    if (rf95_mh != NULL)
    {
        vSemaphoreDelete(rf95_mh);
        rf95_mh = NULL;
    }
    

    sleep(2);
    return;
}
