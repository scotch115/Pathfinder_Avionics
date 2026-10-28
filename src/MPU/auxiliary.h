/*********************************************************************************************************************************************************************/
/*********************************************************************************************************************************************************************/
/************************************************************************ Auxillary Functions ************************************************************************/
/*********************************************************************************************************************************************************************/
/*********************************************************************************************************************************************************************/
void blep(uint8_t buzzerPin, int pitch, int length) {
  // unsigned long startTime = millis(); // TODO - Is it possible to use clock cycles for buzzer duration?
  if (!mute) {
    tone(buzzerPin, pitch);
    digitalWrite(LEDPIN, HIGH);
    delay(length);
    noTone(buzzerPin);
    digitalWrite(LEDPIN, LOW);
  }
}

void deflectionTest() {
  servo0.attach(servo0Pin);
  servo1.attach(servo1Pin);
  servo2.attach(servo2Pin);
  servo3.attach(servo3Pin);
  // Test servo functionality
  servo0.write(180);
  servo1.write(-180);
  servo2.write(180);
  servo3.write(-180);
  delay(2000);
  servo0.write(-180);
  servo1.write(180);
  servo2.write(-180);
  servo3.write(180);
  delay(2000);
  servo0.write(90);
  servo1.write(90);
  servo2.write(90);
  servo3.write(90);
  delay(2000);
}

// TODO - Implement continuity check to determing if servos are even connected
/**
  * Check pin continuity 
  * @param testPin - Pin Number
  * @param curPinMode - 0 = INPUT; 1 = OUTPUT
  */
void continuity(int testPin, int curPinMode) {
  if (curPinMode != 0) {
    pinMode(testPin, INPUT);
    delay(100);
  }
  digitalWrite(testPin, HIGH);

  int pinState = digitalRead(testPin);
  if (pinState == HIGH) {
    Serial.println("Continuity good");
  } else {
    Serial.println("Continuity failure");
  }

  digitalWrite(testPin, LOW);

  // Reset pin to original mode
  if (curPinMode == 0) {
    pinMode(testPin, INPUT);
  } else {
    pinMode(testPin, OUTPUT);
  }

}

// void deploy() {
//  chutesFired = true;
//  // Fire relay
//  digitalWrite(PYRO_1, HIGH);
//  delay(1000);
//  digitalWrite(PYRO_1, LOW);
//  delay(100);
// }

// void touchdown() {
//  altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);
//  if (altitude - lastAlt == 0) {
//     landed = true;
//     vehicleState = "TOUCHDOWN";
//   }
// }


// CRC-8 Checksum Encode
uint8_t encodeCRC8(byte *data, size_t length) {
  uint8_t polynomial = 0x31; 
  uint8_t crc = 0xFF;
  size_t i, j;

  for (i = 0; i < length; i++) {
    crc ^= data[i] << 8;
    for (j = 0; j < 8; j++) {
      if ((crc & 0x80) != 0) {
        crc = (crc << 1) ^ polynomial;
      } else {
        crc <<= 1;
      }
    }
  }

  return crc;
}

// CRC-8 Checksum Decode
bool decodeCRC8(byte *data, uint8_t rx_crc) {
  // Generate CRC-8 using the data received from the PDTP packet
  uint8_t localCRC = encodeCRC8(data, 26); // Payload size is always going to be 26 bytes
  // Check if the calculated CRC-8 value matches the value received from the PDTP packet
  if (rx_crc == localCRC) {
    Serial.println("CRC MATCH");
    return true;
  } else {
    Serial.println("CRC MISMATCH");
    return false;
  }
}

/************************************************************************/
/************************ CPU Armed Hype Song ;) ************************/
/************************************************************************/
#define NOTE_B4  494
#define NOTE_D5  587
#define NOTE_E5  659
#define NOTE_FS5 740
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_B5  988
#define REST      0


int tempo = 169;
int buzzer = 15;

int melody[] = {
  NOTE_FS5,8, NOTE_FS5,8,NOTE_D5,8, NOTE_B4,8, REST,8, NOTE_B4,8, REST,8, NOTE_E5,8, 
  REST,8, NOTE_E5,8, REST,8, NOTE_E5,8, NOTE_GS5,8, NOTE_GS5,8, NOTE_A5,8, NOTE_B5,8,
  NOTE_A5,8, NOTE_A5,8, NOTE_A5,8, NOTE_E5,8, REST,8, NOTE_D5,8, REST,8, NOTE_FS5,8, 
  REST,8, NOTE_FS5,8, REST,8, NOTE_FS5,8, NOTE_E5,8, NOTE_E5,8, NOTE_FS5,8, NOTE_E5,8,
  NOTE_FS5,8, NOTE_FS5,8,NOTE_D5,8, NOTE_B4,8, REST,8, NOTE_B4,8, REST,8, NOTE_E5,8, 
  REST,8, NOTE_E5,8, REST,8, NOTE_E5,8, NOTE_GS5,8, NOTE_GS5,8, NOTE_A5,8, NOTE_B5,8,
  NOTE_A5,8, NOTE_A5,8, NOTE_A5,8, NOTE_E5,8, REST,8, NOTE_D5,8, REST,8, NOTE_FS5,8, 
  REST,8, NOTE_FS5,8, REST,8, NOTE_FS5,8, NOTE_E5,8, NOTE_E5,8, NOTE_FS5,8, NOTE_E5,8,
};

int notes = sizeof(melody) / sizeof(melody[0]) / 2;
int wholenote = (60000 * 4) / 169;
int divider = 0, noteDuration = 0;

void readyForLaunch() {
  for (int thisNote = 0; thisNote < notes * 2; thisNote = thisNote + 2) {
    divider = melody[thisNote + 1];
    if (divider > 0) {
      noteDuration = (wholenote) / divider;
    } else if (divider < 0) {
      noteDuration = (wholenote) / abs(divider);
    }
    tone(buzzer, melody[thisNote], noteDuration * 0.9);
    delay(noteDuration);
    noTone(buzzer);
  }
}
