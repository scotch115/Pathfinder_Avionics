/******************/
/* Libary Imports */
/******************/
#include "Wire.h"
#include "I2Cdev.h"
#include <Adafruit_BME280.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
//#include <avr/dtostrf.h>
//#include "MPU6050_6Axis_MotionApps612.h"
// #include <RH_RF95.h>
#include <PWMServo.h>
#include <SimpleKalmanFilter.h>
//#include <SPI.h>
//#include <WiFiNINA.h>
// #include <WiFiUdp.h>

// Import support files
#include "flightConfiguration.h"
#include "variables.h"
#include "sensors.h"
#include "auxiliary.h"
#include "clk.h"
#include "healthmonitor.h"
#include "pbit.h"
#include "pdtp.h"
#include "rcs.h"

/********************************************************************************/
/********************************************************************************/
/********************************* Startup **************************************/
/********************************************************************************/
/********************************************************************************/

void setup() {
  // join I2C bus (I2Cdev library doesn't do this automatically)
  #if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
    Wire.begin(0x73);  // 0x73 is the Big Endian value of 115, the flight computer address
    // Wire.setClock(400000); // 400kHz I2C clock.
    Wire.setClock(10000);  //? [4/24/2026] - 10kHz I2C clock. Not sure if this is too slow but 400 kHz seemed too fast.
    // Wire.onReceive(pdtpRx);
  // #elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
  //   Fastwire::setup(400, true); //? [4/24/2026] - what is the point of this?
  #endif

  /*   Prep RMF95 for flight   */
  // if (TELEMETRY == "ACTIVE") {
  //   pinMode(RFM95_RST, OUTPUT);
  //   digitalWrite(RFM95_RST, HIGH);
  // }

  // Serial.begin(9600);
  Serial.begin(115200);

  // Wait for serial to be available if board is in 'DEBUG' mode
  if (String(debugState) == "ON") {
    while (!Serial) {
    }
  }

  // Run pre-flight check
  preCheck();
}

/********************************************************************************/
/********************************************************************************/
/*********************************** FSW loop ***********************************/
/********************************************************************************/
/********************************************************************************/
void loop() {
  /**
 * @note - The FSW supports the following states:

 * @note - STARTUP     := No telem or sensor data until PBIT is complete
 * @note - ARMED       := Active sensor reads, intermittent telemetry (WiFi main, intermittent RF for health), all pyro channels ON
 * @note - IDLE        := All pyro channels OFF, "HM" mode only
 * @note - ASCENT      := Active flight profile - maximum read/write speeds, WiFi off/RF only, apogee detection and roll control active
 * @note - DESCENT     := Active flight profile - descent verified, chute ejection command sent and adequately logged, begin higher-speed telem broadcasting
 * @note - TOUCHDOWN   := Once verified, transmit "ONGRND" (or "GND") over RF and disable roll control, apogee detection and pyro channels; only sensors and telemetry; increase volume/pitch for HM sounds
 * @note - DEBUG       := Highly verbose mode with increased modularity in order to individually enable/disable systems through remote commands; all pyro channels disabled while in this state
 * @note - ERROR/ABORT := Based on what triggered the error either log all messages with the "ERROR" FSB status, or cancel pending commands (i.e. clear "T-Minus event schedule") //! Implement event schedule
 */
  currentTime = millis();
  hmTime = millis();
  missionTime = millis();
  if (clockEnable == true) {
    clk(); // Maybe clock should not interfere with hardware clock but instead just be used for the HM...
  }

  switch(vehicleState) {
    case ARMED: // 1Hz
      clockReadInterval = 500;  // When ARMED, beep/read to indicate high and low clock waves
      HM(clockReadInterval, 1000, false);
      flightControl();
      break;
    case IDLE: // 2Hz
      clockReadInterval = 1000;
      HM(clockReadInterval, 1000, true);
      flightControl();
      // TODO - If flight computer is in 'IDLE' mode, lower to 2Hz to further improve battery performance
      break;
    case DEEPSLEEP: // 5Hz
      clockReadInterval = 5000;
      HM(clockReadInterval, 800, false);
      flightControl();
      // TODO - If flight computer is in 'IDLE' mode, lower to 5Hz to further improve battery performance
      break;
    case ASCENT:
    case DESCENT:
      // Blep twice as in flight to indicate a change in state
      clockReadInterval = 1000;
      HM((clockReadInterval / 2), 1200, false);
      flightControl();
      break;
    // case STARTUP:
    //   return;
    case ERROR:
    case ABORT:
      // TODO - Probably need more than a Serial message here
      Serial.println();Serial.println();
      Serial.println("ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR");
      Serial.println("!!!!!!!   POWER DOWN THE ROCKET AND BE SURE TO DISENGAGE THE MOTOR    !!!!!!!");
      Serial.println("!!!!!!!   EVALUATE FLIGHT COMPUTER AND CIRCUITRY BEFORE CONTINUING    !!!!!!!");
      Serial.println("!!!!!!!  ENSURE CONTINUITY ON ALL COMPONENTS BEFORE ATTEMPTING FLIGHT !!!!!!!");
      Serial.println("ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR");
      Serial.println();Serial.println();
      blep(BUZZER_PIN, 311, 1000);
      break;
    case TOUCHDOWN:
      /* Play a deep, drone note */
      tone(BUZZER_PIN, 300);
      /* Beep periodically to help recovery crew find vehicle */
      blep(BUZZER_PIN, 523, 100);
      break;
    case DEBUG:
      /* Do not limit data stream for efficient debugging */
      HM(clockReadInterval, 1200, false);
      flightControl();
      clockPeriod = 5000;  // Set clock to 5Hz for increased visibility of messages coming in
      break;
    default:
      Serial.println("VEHICLE STATE UNKNOWN!");
      delay(200); 
      Serial.println("REVERTING STATE TO ARMED"); // TODO - Maybe store "last state" and revert to last "known good"? 
      break;
  }
  delay(50);
}

/**********************************************************/
/****************** Active Control Logic ******************/
/**********************************************************/
void flightControl() {
  // TODO - Calculate quaternions using raw IMU data without the use of DMP
  rcs(ypr);
  // pdtpTx(pldPkt);

  /********************************************/
  /********** CODE BELOW IS WIP ***************/
  /********************************************/
  
  // Check if rocket has reached/passed apogee
  // getVehicleAltitude();

  // if (chutesFired) {
  //   touchdown();
  // }

  /***************************************
  *    ? Get current battery level ?
  * ! REMOVE WHEN BATTERY IS UNPLUGGED  !
  **************************************/
  // Get current battery level
  // float measuredvbat = analogRead(VBATPIN);
  // measuredvbat *= 2;
  // measuredvbat *= 3.3;
  // measuredvbat /= 1024;
}
