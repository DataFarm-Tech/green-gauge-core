#include "bms.h"
#include "hw.h"
#include "Arduino.h"

typedef enum
{
    BAT_CHARGE_FULL,        // > 75%
    BAT_CHARGE_HIGH,        // 50%–75%
    BAT_CHARGE_MEDIUM,      // 25%–50%
    BAT_CHARGE_LOW,         // 3%–25%
    BAT_CHARGE_CRITICAL,     // < 3%
    BAT_CHARGE_UNDEFINED
} bat_charge_state_e;


bat_charge_state_e battery_state = BAT_CHARGE_UNDEFINED;

/**
 * @brief Retrieves all PIN states to calculate the 
 * current battery state percentage.
 * 
 * Need to validate this with Lawind.
 */
void get_battery_state()
{
    bool led1_on = digitalRead(BMS_LED_PIN_1) == LOW;
    bool led2_on = digitalRead(BMS_LED_PIN_2) == LOW;
    bool led3_on = digitalRead(BMS_LED_PIN_3) == LOW;

    if (!led1_on && !led2_on && !led3_on)
    {
        battery_state = BAT_CHARGE_FULL;  // 100%
    }
    else if (led1_on && led2_on && led3_on)
    {
        battery_state = BAT_CHARGE_CRITICAL;  // Possibly <3%
    }
    else if (led1_on && led2_on && !led3_on)
    {
        battery_state = BAT_CHARGE_HIGH;  // 50–75%
    }
    else if (led1_on && !led2_on && !led3_on)
    {
        battery_state = BAT_CHARGE_LOW;  // 3–25%
    }
    else if (led1_on && led2_on && led3_on)
    {
        battery_state = BAT_CHARGE_MEDIUM;  // 25–50%
    }
    else
    {
        battery_state = BAT_CHARGE_UNDEFINED;  // Catch-all for weird states
    }
}

