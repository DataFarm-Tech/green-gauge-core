#include <Arduino.h>

#include "cli.h"
#include "init.h"
#include "interrupts.h"
#include "config.h"
#include "hw.h"
#include "th_handler.h"
#include "eeprom.h"
#include "utils.h"
#include "err_handle.h"

/*
The current state must be undefined when initialising.
The logic does not understand what state it is in.
*/
volatile device_state_t current_state = UNDEFINED_STATE;

/**
 * @brief This thread is the first to run and initializes the system.
 * It sets up the hardware and starts the other threads. Depending on the active state
 * of the system, it will start particular threads.
 * @param parameter - The parameter passed to the task
 * @return None
 */
void init_p()
{
    Serial.begin(BAUD_RATE);
    sleep(5); /* This is provided to ensure serial console has enough time to init*/
    printf("init_p: Starting initialization...\n");

    // pinMode(INT_STATE_PIN, INPUT); /* Our interr pins must be set before an interrupt is called*/
    // pinMode(INT_STATE_PIN_2, INPUT);

    led_pin_init();

    init_eeprom_int(); /* init the eeprom interface*/
    read_config();

    // switch_state(digitalRead(INT_STATE_PIN), digitalRead(INT_STATE_PIN_2)); //force a check on the switch state

    // // Attach interrupts to both pins to monitor state changes
    // attachInterrupt(digitalPinToInterrupt(INT_STATE_PIN), has_state_changed, CHANGE);
    // attachInterrupt(digitalPinToInterrupt(INT_STATE_PIN_2), has_state_changed, CHANGE);

    // create_th(process_state_change, "process_state_change", PROC_CS_TH_STACK_SIZE, &process_state_ch_th, 0);

    switch_state(1, 0); //force a check on the switch state
    
    timer = timerBegin(0, TIMER_PRESCALER, true);
    
    if (!timer) 
    {
        printf("init_p: Failed to create timer\n");
        while(1);
    }

    timerAttachInterrupt(timer, &on_hourly_timer, true);
    timerAlarmWrite(timer, alarm_value_hourly, true);
    timerAlarmEnable(timer);


    //CLI Thread creation.
    print_motd();
    create_th(read_serial_cli, "read_serial_cli", READ_SERIAL_CLI_TH_STACK_SIZE, &read_serial_cli_th, 1);
}