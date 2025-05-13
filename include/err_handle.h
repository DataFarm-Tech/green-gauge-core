#pragma once
#include "err_handle.h"

/**
 * @brief A enum type for reflecting an interface's
 * state. Either working or failing.
 */
typedef enum
{
    INT_STATE_OK,
    INT_STATE_ERROR
} int_state_e;

typedef enum
{
    LORA,
    WIFI,
    SENSOR
} pof_e; /** Point of Failure */


/**
 * @brief The following function set's all pins to
 * output. This allows them to be manipulated.
 */
void led_pin_init();
void err_led_state(pof_e type, int_state_e interface_state);