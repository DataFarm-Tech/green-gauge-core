#include <Arduino.h>
#include <RH_RF95.h>
#include <SPI.h>

#include "config.h"
#include "hw.h"
#include "utils.h"


RH_RF95 rf95(RFM95_NSS, RFM95_INT); // Create the rf95 obj

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

  return;
}