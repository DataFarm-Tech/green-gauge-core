#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "utils.h"

/**
 * These are the thread handlers for the different threads in the system.
 * They are used to manage the threads and ensure that they are running correctly.
 * The threads are responsible for different tasks in the system, such as reading
 * serial input, processing state changes, and handling LoRa communication.
 */
TaskHandle_t read_serial_cli_th = NULL;
TaskHandle_t process_state_ch_th = NULL;
TaskHandle_t lora_listener_th = NULL;
TaskHandle_t main_app_th = NULL;
TaskHandle_t http_th = NULL;

/**
 * @brief This function creates a thread and pins it to a specific core.
 * @param func - The function to be executed by the thread
 * @param name - The name of the thread
 * @param th - The thread handle
 * @param core - The core to which the thread should be pinned
 * @return None
 */
void create_th(TaskFunction_t func, const char* name, int stack_size, TaskHandle_t* th, int core)
{
    if (*th == NULL) 
    {
        xTaskCreatePinnedToCore(func, name, stack_size, NULL, 1, th, core);   
    } 
    else 
    {
        PRINT_ERROR("Thread already exists.");
    }
}

/** 
 * @brief This function deletes a thread if it exists.
 * @param th - The thread handle to be deleted
 * @return None
 */
void delete_th(TaskHandle_t th)
{
    if (th != NULL) 
    {
        vTaskDelete(th);
        PRINT_INFO("Thread deleted");

        th = NULL; // Set the pointer to NULL after deletion
    }
    else 
    {
        PRINT_WARNING("Thread has not been init...");
    }
}
