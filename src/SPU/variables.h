/*************************************************************************************************************************************************************************/
/*************************************************************************************************************************************************************************/
/******************************************************************************** GLOBALS ********************************************************************************/
/*************************************************************************************************************************************************************************/
/*************************************************************************************************************************************************************************/

/*************************************************/
/***************** DEFINITIONS *******************/
/*************************************************/
/* -== TELEMETRY ==- */
/* Pinouts */
// LoRa Pin Assignments
#define RF95_FREQ 915.0
#define RFM95_RST 11
#define RFM95_CS 10
#define RFM95_INT 2

RH_RF95 rf95(RFM95_CS, RFM95_INT);

// WiFi Pin Assignments
//#define SPIWIFI       SPI  // The SPI port
//#define SPIWIFI_SS    13   // Chip select pin
//#define ESP32_RESETN  12   // Reset pin
//#define SPIWIFI_ACK   11   // a.k.a BUSY or READY pin
//#define ESP32_GPIO0   -1

// WiFi Module Definition
// WiFiUDP Udp;
// IPAddress remoteIp(10, 0, 0, 6);


/* -== IO ==- */
 // Chip Select for the Adalogger SD Card
const int CS = 4;

/*************************************************/
/******************* VARIABLES *******************/
/*************************************************/
/* -== STATE HANDLING ==- */
enum State {
  STARTUP,
  ARMED,
  IDLE,
  ASCENT,
  DESCENT,
  TOUCHDOWN,
  DEBUG,
  ERROR,
  ABORT,
};
State vehicleState = STARTLAUNCHSTATE;

unsigned long clockPeriod = 1000;
unsigned long clockReadInterval = 1000;
unsigned long prevTime = 0;
unsigned long prevDeployTime = 0;
unsigned long currentTime = 0;

uint16_t sequenceNumber = 0; // Static sequence number for PDTP

//* 1 byte = [4 bits][4 bits] => [status][pad size]
// 0 - INFO  //* Text only [0][max of 15]
// 1 - DATA  //* Sensor data [1][0]
// 2 - ACK   //* Response to/from secondary CPU [2][max of 15]
// 3 - ERROR //* Error detected, shit's fucked [3][0 (send data still but mark it as an error)]
byte statusByte;

// bool descent = false; // Literally never used that

struct payloadPacket {
  int16_t tmp;
  int16_t kpa;
  int16_t alt;
  int16_t hum;
  int16_t yaw;
  int16_t pch;
  int16_t rol;
  int16_t acx;
  int16_t acy;
  int16_t acz;
  int16_t yrc;
  int16_t prc;
  int16_t rrc;
};

union PDTP_Packet {
  payloadPacket payload;
  byte bytes[sizeof(payload)];
};

PDTP_Packet pldPkt;