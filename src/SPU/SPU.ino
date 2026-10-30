/******************/
/* Libary Imports */
/******************/
#include "Wire.h"
#include "I2Cdev.h"
#include <SPI.h>
#include <SD.h>
#include <RH_RF95.h>
//#include <WiFiNINA.h>
// #include <WiFiUdp.h>

// Import support files
#include "flightConfiguration.h"
#include "variables.h"
#include "auxiliary.h"
#include "pdtp.h"


/********************************************************************************/
/********************************************************************************/
/********************************* Startup **************************************/
/********************************************************************************/
/********************************************************************************/

void setup() {
  // join I2C bus (I2Cdev library doesn't do this automatically)
  #if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
    Wire.begin(0x82);  // 0x82 is the Big Endian value of 115, the flight computer address
    // Wire.setClock(400000); // 400kHz I2C clock.
    Wire.setClock(10000); //? [4/24/2026] - 10kHz I2C clock. Not sure if this is too slow but 400 kHz seemed too fast.
    Wire.onReceive(pdtp_rx);
    // Wire.onRequest(pdtpTx);
  // #elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
  //   Fastwire::setup(400, true); //? [4/24/2026] - what is the point of this?
  #endif

}
void loop() {
  delay(50);
}
