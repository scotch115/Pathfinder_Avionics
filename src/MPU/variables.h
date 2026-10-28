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
//#define RFM95_CS 8
//#define RFM95_RST 4
//#define RFM95_INT 3

// WiFi Pin Assignments
//#define SPIWIFI       SPI  // The SPI port
//#define SPIWIFI_SS    13   // Chip select pin
//#define ESP32_RESETN  12   // Reset pin
//#define SPIWIFI_ACK   11   // a.k.a BUSY or READY pin
//#define ESP32_GPIO0   -1

// WiFi Module Definition
// WiFiUDP Udp;
// IPAddress remoteIp(10, 0, 0, 6);

// RF Pin Assignments
//#define RF95_FREQ 915.0

// LoRa Radio Module Definition
//RH_RF95 rf95(RFM95_CS, RFM95_INT);
//unsigned int localPort = 2390;      // local port to listen on

/* -== IO ==- */
// Battery Voltage Pin Assignment(for collecting battery status)
//#define VBATPIN A7

// Buzzer Pin Assignment
#define BUZZER_PIN 5

// LED Pin Assignment
#define LEDPIN 4

// Servo Pin Assignments
int servo0Pin = A1; // Top Right
int servo1Pin = A14; // Top Left
int servo2Pin = A0; // Bottom Right
int servo3Pin = A3; // Bottom Left

// Servo Definition
PWMServo servo0;
PWMServo servo1;
PWMServo servo2;
PWMServo servo3;

/* -== SENSORS ==- */
// Barometric Altimeter Definition
Adafruit_BME280 bme; // I2C

// Inertial Measurement Unit (IMU) Definition
Adafruit_MPU6050 imu; // I2C

/*************************************************/
/******************* VARIABLES *******************/
/*************************************************/
/* -== STATE HANDLING ==- */
enum State {
  STARTUP,
  ARMED,
  IDLE,
  DEEPSLEEP, // secret
  ASCENT,
  DESCENT,
  TOUCHDOWN,
  DEBUG,
  ERROR,
  ABORT,
};
State vehicleState = STARTLAUNCHSTATE;

/* -== Clock States ==- */
enum CLK {
  CLK_HI,
  CLK_LO
};

CLK clockState = CLK_LO;

unsigned long clockPeriod = 1000;
unsigned long clockReadInterval = 1000;
unsigned long clockTime = 0;
unsigned long prevTime = 0;
unsigned long prevDeployTime = 0;
unsigned long hmTime = 0;
unsigned long currentTime = 0;
unsigned long missionTime = 0;

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

// PYRO CHANNELS -- Need to update design with MOSFETS; need to disable all pyro references in the meantime
//#define PYRO_1 A2

int altitude, lastAlt, apogee = 0;

// Buzzer Sounds
unsigned int numTones = 6;
unsigned int tones[] = {523, 587, 659, 739, 830, 880};
/************* upper    C    D    E    F#   G#   A   */

/************************************
 *  Data Payload (String) Variables *
 ************************************/
// bool chutesFired = false;
// bool landed = false;

/********************************
 * IMU control/status variables *
 ********************************/
// bool dmpReady = false;  // set true if DMP init was successful
// uint8_t imuIntStatus;   // holds actual interrupt status byte from IMU
// uint8_t devStatus;      // return status after each device operation (0 = success, !0 = error)
// uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
// uint16_t fifoCount;     // count of all bytes currently in FIFO
// uint8_t fifoBuffer[64]; // FIFO storage buffer

/********************************
 * Orientation/motion variables *
 ********************************/
//Quaternion q;           // [w, x, y, z]         quaternion container
//VectorInt16 aa;         // [x, y, z]            accel sensor measurements
//VectorInt16 aaReal;     // [x, y, z]            gravity-free accel sensor measurements
//VectorInt16 aaWorld;    // [x, y, z]            world-frame accel sensor measurements
//VectorFloat gravity;    // [x, y, z]            gravity vector
sensors_event_t a, g, temp;
float euler[3];         // [psi, theta, phi]    Euler angle container
float ypr[3];           // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector

// Define base position values as zero, to convey the "starting" position
float xPos = 0;
float yPos = 0;
float zPos = 0;
// float xAccel = 0;

/******************************
 * IMU interruption detection *
 ******************************/
// volatile bool imuInterrupt = false;     // indicates whether IMU interrupt pin has gone high
// void dmpDataReady() {
//   imuInterrupt = true;
// }

/***********************************************************/
/********************** KALMAN FILTERS *********************/
/***********************************************************/
// SimpleKalmanFilter(e_mea, e_est, q)
// e_mea: Measurement Uncertainty
// e_est: Estimation Uncertainty
// q: Process Noise 
SimpleKalmanFilter xKalmanFilter(1, 1, 0.01);
SimpleKalmanFilter yKalmanFilter(1, 1, 0.01);
SimpleKalmanFilter zKalmanFilter(1, 1, 0.01);

SimpleKalmanFilter yawKalmanFilter(1, 1, 0.01);
SimpleKalmanFilter pitchKalmanFilter(1, 1, 0.01);
SimpleKalmanFilter rollKalmanFilter(1, 1, 0.01);

SimpleKalmanFilter tempFilter(1, 1, 0.01);
SimpleKalmanFilter pressureFilter(1, 1, 0.01);
SimpleKalmanFilter altitudeFilter(1,1, 0.01);
SimpleKalmanFilter humidityFilter(1,1, 0.01);

SimpleKalmanFilter deltaYawFilter(1, 1, 0.01);
SimpleKalmanFilter deltaPitchFilter(1, 1, 0.01);
SimpleKalmanFilter deltaRollFilter(1, 1, 0.01);
