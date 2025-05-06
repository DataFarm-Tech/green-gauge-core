#pragma once

#include <Arduino.h>

typedef enum {
    UNDEFINED_STATE = 0,
    SENSOR_STATE = 1,
    CONTROLLER_STATE = 2,
} device_state_t;

extern volatile device_state_t current_state;

void init_p();

