#pragma once

#include <Arduino.h>

#define INT_STATE_PIN 14
#define INT_STATE_PIN_2 15

#define TIMER_PRESCALER 80  // Prescaler value

extern hw_timer_t * timer;
extern volatile bool hourly_timer_flag;
extern const uint64_t alarm_value_hourly;

//Function definitions
void has_state_changed() IRAM_ATTR;
void on_hourly_timer() IRAM_ATTR;
void process_state_change(void *param);
void switch_state(const int sensor_pin, const int controller_pin);
void tear_down();
bool is_key_set();
