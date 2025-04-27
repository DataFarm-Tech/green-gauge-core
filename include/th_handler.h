#pragma once

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define PROC_CS_TH_STACK_SIZE 5000
#define READ_SERIAL_CLI_TH_STACK_SIZE 5000
#define LORA_LISTENER_TH_STACK_SIZE 10000
#define MAIN_APP_TH_STACK_SIZE 5000
#define HTTP_TH_STACK_SIZE 10000

extern TaskHandle_t read_serial_cli_th;
extern TaskHandle_t process_state_ch_th;
extern TaskHandle_t lora_listener_th;
extern TaskHandle_t main_app_th;
extern TaskHandle_t http_th;

//Function definitions
void create_th(TaskFunction_t func, const char* name, int stack_size, TaskHandle_t* th, int core);
void delete_th(TaskHandle_t th);
void print_all_threads_detailed();