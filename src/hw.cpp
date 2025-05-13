#include <Arduino.h>
#include <RH_RF95.h>
#include <SPI.h>
#include <WiFi.h>

#include "config.h"
#include "hw.h"
#include "utils.h"
#include "lora_listener.h"
#include "mutex_h.h"
#include "err_handle.h"

/**
 * @brief This function initializes the hardware for the RFM95W module.
 * It sets up the pins and configures the module for communication.
 * It also sets the frequency and power settings for the module.
 * @return None
 */
void rfm95w_setup()
{
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  if (xSemaphoreTake(rf95_mh, portMAX_DELAY) == pdTRUE)
  {
      while (!rf95.init())
      {
          PRINT_ERROR("rfm95w module init failed...");
          err_led_state(LORA, INT_STATE_OK);
          sleep(5); //output red LED
      }

      if (!rf95.setFrequency(RF95_FREQ))
      {
          PRINT_ERROR("unable to set frequency for rfm95w module...");
          err_led_state(LORA, INT_STATE_OK);
          sleep(5); //output red LED
      }

      rf95.setTxPower(23, false);
      rf95.setModemConfig(RH_RF95::Bw125Cr48Sf4096);
      
      err_led_state(LORA, INT_STATE_OK);

      xSemaphoreGive(rf95_mh); // release the lock when done
  }

  return;
}


/**
 * Will change to wifi manager
 */
void wifi_connect() 
{
    if (WiFi.status() == WL_CONNECTED) 
    {
        PRINT_INFO("Already connected to WiFi");
        return;
    }

    const char *ssid = "MyWifi6E";
    const char *password = "ExpZZLN9U8V2";

    PRINT_INFO("Connecting to WiFi...");

    WiFi.begin(ssid, password);

    unsigned long startAttemptTime = millis();

    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) 
    {
        printf(".");
        delay(500);
    }

    if (WiFi.status() == WL_CONNECTED) 
    {
        PRINT_INFO("WiFi connected!");
        err_led_state(WIFI, INT_STATE_OK);
    } 
    else 
    {
        PRINT_WARNING("Failed to connect to WiFi.");
        err_led_state(WIFI, INT_STATE_ERROR);
    }
}

/**
 * @brief The following function disconnects the WiFi.
 * @param erase_creds The following parameter refers to erasing the creds or keeping them
 */
void wifi_disconnect(bool erase_creds) 
{
    if (WiFi.status() == WL_CONNECTED) 
    {
        PRINT_INFO("Disconnecting from WiFi...");
        WiFi.disconnect(erase_creds); // Set to false if you don't want to erase credentials
    }

    err_led_state(WIFI, INT_STATE_ERROR);
    
}