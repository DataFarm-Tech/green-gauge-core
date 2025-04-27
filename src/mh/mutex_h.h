#pragma once

#include <Arduino.h>
#include "init.h"

extern SemaphoreHandle_t ntp_client_mh;  // Mutex handle

void init_mutex(device_state_t state);
