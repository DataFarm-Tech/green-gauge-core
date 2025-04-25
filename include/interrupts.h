#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <Arduino.h>

#define INT_STATE_PIN 32
#define INT_STATE_PIN_2 33

//Function definitions
void has_state_changed() IRAM_ATTR;
void process_state_change(void *param);
void switch_state(const int sensor_pin, const int controller_pin);

#endif // INTERRUPTS_H
