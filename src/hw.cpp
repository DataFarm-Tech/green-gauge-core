#include <Arduino.h>
#include <RH_RF95.h>
#include <SPI.h>
#include <WiFi.h>

#include "config.h"
#include "hw.h"
#include "utils.h"
#include "lora/lora_listener.h"

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

  // Create the mutex
  rf95_mutex = xSemaphoreCreateMutex();
  if (rf95_mutex == NULL) 
  {
      PRINT_ERROR("Failed to create rf95 mutex!");
      return; // exit if mutex not created
  }

  if (xSemaphoreTake(rf95_mutex, portMAX_DELAY) == pdTRUE)
  {
      while (!rf95.init())
      {
          PRINT_ERROR("rfm95w module init failed...");
          sleep(5); //output red LED
      }

      if (!rf95.setFrequency(RF95_FREQ))
      {
          PRINT_ERROR("unable to set frequency for rfm95w module...");
          sleep(5); //output red LED
      }

      rf95.setTxPower(23, false);
      rf95.setModemConfig(RH_RF95::Bw125Cr48Sf4096);

      xSemaphoreGive(rf95_mutex); // release the lock when done
  }

  return;
}


/** Change this to WiFi manager. */
void wifi_connect() 
{
    const char *ssid = "MyWifi6E";
    const char *password = "ExpZZLN9U8V2";

    WiFi.begin(ssid, password);

    PRINT_INFO("Connecting to WiFi");

    while (WiFi.status() != WL_CONNECTED) {
        printf(".");
        delay(500);
    }
}