#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <Arduino.h>

void has_state_changed() IRAM_ATTR;
void process_state_change(void *param);
void switch_state(const int sensor_pin, const int controller_pin);

#endif // INTERRUPTS_H
