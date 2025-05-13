#include "err_handle.h"
#include <Arduino.h>
#include "hw.h"

/**
 * Change these pin numbers as per your hardware setup.
 */

#define LORA_STATUS_OK     LORA_LED_GREEN
#define LORA_STATUS_FAIL   LORA_LED_RED

#define WIFI_STATUS_OK     WIFI_LED_GREEN
#define WIFI_STATUS_FAIL   WIFI_LED_RED

#define SENSOR_STATUS_OK    SENSOR_LED_GREEN
#define SENSOR_STATUS_FAIL  SENSOR_LED_RED



/**
 * @brief Sets interface LED states based on passed status and interface type.
 */
void err_led_state(pof_e type, int_state_e interface_state)
{
    int green_pin = -1;
    int red_pin = -1;

    // Select appropriate pins based on interface type
    switch (type)
    {
        case LORA:
            green_pin = LORA_STATUS_OK;
            red_pin = LORA_STATUS_FAIL;
            break;
        case WIFI:
            green_pin = WIFI_STATUS_OK;
            red_pin = WIFI_STATUS_FAIL;
            break;
        case SENSOR:
            green_pin = SENSOR_STATUS_OK;
            red_pin = SENSOR_STATUS_FAIL;
            break;
        default:
            // Invalid type, optionally log or return early
            return;
    }

    // Set LEDs based on interface state
    switch (interface_state)
    {
        case INT_STATE_OK:
            if (digitalRead(green_pin) != HIGH)
            {
                digitalWrite(green_pin, HIGH);
                digitalWrite(red_pin, LOW);
            }
            break;

        case INT_STATE_ERROR:
            if (digitalRead(red_pin) != HIGH)
            {
                digitalWrite(green_pin, LOW);
                digitalWrite(red_pin, HIGH);
            }
            break;

        default:
            // Unknown state, do nothing or log
            break;
    }
}


/**
 * @brief The following function sets all pins to
 * output. This allows them to be manipulated.
 */
void led_pin_init()
{
    /** 
     * Convert all LED pins into output pins
     */
    pinMode(LORA_STATUS_OK, OUTPUT);
    pinMode(LORA_STATUS_FAIL, OUTPUT);

    pinMode(WIFI_STATUS_OK, OUTPUT);
    pinMode(WIFI_STATUS_FAIL, OUTPUT);

    pinMode(SENSOR_STATUS_OK, OUTPUT);
    pinMode(SENSOR_STATUS_FAIL, OUTPUT);

    /**
     * Set all green LED's to high and red LEDS to low
     * by default.
     */
    digitalWrite(LORA_STATUS_OK, HIGH);
    digitalWrite(LORA_STATUS_FAIL, LOW);

    digitalWrite(WIFI_STATUS_OK, HIGH);
    digitalWrite(WIFI_STATUS_FAIL, LOW);

    digitalWrite(SENSOR_STATUS_OK, HIGH);
    digitalWrite(SENSOR_STATUS_FAIL, LOW);
}