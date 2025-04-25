#ifndef INIT_H
#define INIT_H

#include <Arduino.h>

typedef enum {
    UNDEFINED_STATE = 0,
    SENSOR_STATE = 1,
    CONTROLLER_STATE = 2,
} device_state_t;

// Function declarations
void init_p();

// External variables
extern volatile device_state_t current_state;

#endif // INIT_H
