/******************************************************************************************************/
/*********************************** Payload Data Transmit Protocol ***********************************/
/******************************************************************************************************/
/** << 32 byte wide packet >>
 * Status Byte - 1 byte
 * Dest Address - 1 byte
 * Src Address - 1 byte
 * Payload - 26 bytes
 * Seq Num - 2 bytes
 * CRC - 1 byte
 * @param packet
 */
void pdtpTx(PDTP_Packet& packet) {
  byte pdtpPacket[32]; // Initialize the data packet as an array of bytes
  
  byte sourceAddress = 115; // MPU 
  byte destAddress = 130; // SPU

  packet.payload.tmp = int(trunc(bme.readTemperature()));
  packet.payload.kpa = int(trunc((bme.readPressure() / 100.0F) / 10)); // Convert to kPa
  packet.payload.alt = calculateAltitude(bme.readAltitude(SEALEVELPRESSURE_HPA), a.acceleration.y);
  packet.payload.hum = int(trunc(humidityFilter.updateEstimate(bme.readHumidity())));
  packet.payload.yaw = int(trunc(ypr[0])); // TODO - Fix YPR and ACCEL readings
  packet.payload.pch = int(trunc(ypr[1]));
  packet.payload.rol = int(trunc(ypr[2]));
  packet.payload.acx = int(trunc(a.acceleration.x));
  packet.payload.acy = int(trunc(a.acceleration.y));
  packet.payload.acz = int(trunc(a.acceleration.z));
  packet.payload.yrc = calculateYawDisplacement(g.gyro.z);
  packet.payload.prc = calculatePitchDisplacement(g.gyro.x);
  packet.payload.rrc = calculateRollDisplacement(g.gyro.y);

  // Now, calculate the CRC for the payload byte array
  uint8_t CRC = encodeCRC8(packet.bytes, sizeof(packet.bytes));

  // Populate pdtpPacket
  // 1. Status Byte (1 byte)
  pdtpPacket[0] = statusByte;

  // 2. Destination Address (1 byte)
  pdtpPacket[1] = destAddress;

  // 3. Source Address (1 byte)
  pdtpPacket[2] = sourceAddress;

  // 4. Payload (26 bytes)
  pdtpPacket[3] = (packet.payload.tmp >> 8) & 0xFF;
  pdtpPacket[4] = (packet.payload.tmp & 0xFF);
  pdtpPacket[5] = (packet.payload.kpa >> 8) & 0xFF;
  pdtpPacket[6] = (packet.payload.kpa & 0xFF);
  pdtpPacket[7] = (packet.payload.alt >> 8) & 0xFF;
  pdtpPacket[8] = (packet.payload.alt & 0xFF);
  pdtpPacket[9] = (packet.payload.hum >> 8) & 0xFF;
  pdtpPacket[10] = (packet.payload.hum & 0xFF);
  pdtpPacket[11] = (packet.payload.yaw >> 8) & 0xFF;
  pdtpPacket[12] = (packet.payload.yaw & 0xFF);
  pdtpPacket[13] = (packet.payload.pch >> 8) & 0xFF;
  pdtpPacket[14] = (packet.payload.pch & 0xFF);
  pdtpPacket[15] = (packet.payload.rol >> 8) & 0xFF;
  pdtpPacket[16] = (packet.payload.rol & 0xFF);
  pdtpPacket[17] = (packet.payload.acx >> 8) & 0xFF;
  pdtpPacket[18] = (packet.payload.acx & 0xFF);
  pdtpPacket[19] = (packet.payload.acy >> 8) & 0xFF;
  pdtpPacket[20] = (packet.payload.acy & 0xFF);
  pdtpPacket[21] = (packet.payload.acz >> 8) & 0xFF;
  pdtpPacket[22] = (packet.payload.acz & 0xFF);
  pdtpPacket[23] = (packet.payload.yrc >> 8) & 0xFF;
  pdtpPacket[24] = (packet.payload.yrc & 0xFF);
  pdtpPacket[25] = (packet.payload.prc >> 8) & 0xFF;
  pdtpPacket[26] = (packet.payload.prc & 0xFF);
  pdtpPacket[27] = (packet.payload.rrc >> 8) & 0xFF;
  pdtpPacket[28] = (packet.payload.rrc & 0xFF);

  // 5. Sequence Number (2 bytes)
  pdtpPacket[29] = (sequenceNumber >> 8);
  pdtpPacket[30] = sequenceNumber & 0xFF;
  
  // 6. CRC (1 byte)
  pdtpPacket[31] = CRC;

  Wire.beginTransmission(0x82); // 0x82 is the Big Endian value of 130, the SPU address
  Wire.write(pdtpPacket, 32);
  Wire.endTransmission();

  sequenceNumber = sequenceNumber + 1;
  delay(10);
};

/** << 32 byte wide packet >>
 * Status Byte - 1 byte
 * Dest Address - 1 byte
 * Src Address - 1 byte
 * Payload - 26 bytes
 * Seq Num - 2 bytes
 * CRC - 1 byte
 */
void pdtpRx() {
  byte rxPkt[3];
  
  // Wire.requestFrom(0x82, 3); // Request data from SPU
  while (1 < Wire.available()) {
    for (int b = 0; b < 3; b++) {
      rxPkt[b] = Wire.read();
    }
  }
  delay(100);

  Serial.println("");Serial.println("");
  Serial.println("#########");
  Serial.println("# BYTES #");
  Serial.println("#########");
  for (int i = 0; i < 3; i++) {
    Serial.print(rxPkt[i]);
    if (i != 3) {
      Serial.print(",");
    }
  }
  Serial.println("");

  // Extract the PDTP packet attributes
  // char* message;
  // message[0] = rxPkt[3];
  // message[1] = rxPkt[4];
  // message[2] = rxPkt[5];
  // byte statusByte = rxPkt[0];
  // byte destAddr = rxPkt[1];
  // byte srcAddr = rxPkt[2];
  // byte crc = rxPkt[31];

  // Serial.println("");Serial.println("");
  // Serial.println("#######");
  // Serial.println("# MSG #");
  // Serial.println("#######");

  // Serial.println(message);

  // TODO - Implement check here to decode/determine how to proceed
  // (i.e. INFO messages don't need to be extracted)
  // Also, primary flight computer will never need to RX sensor data, update
  // to only "process" commands
  // -- switch statement based on "command" passed in can call various auxiliary functions
  // int16_t tmp = ((int16_t)rxPkt[3] << 8) | rxPkt[4]; 
  // int16_t kpa = ((int16_t)rxPkt[5] << 8) | rxPkt[6]; 
  // int16_t alt = ((int16_t)rxPkt[7] << 8) | rxPkt[8]; 
  // int16_t hum = ((int16_t)rxPkt[9] << 8) | rxPkt[10]; 
  // int16_t yaw = ((int16_t)rxPkt[11] << 8) | rxPkt[12]; 
  // int16_t pch = ((int16_t)rxPkt[13] << 8) | rxPkt[14]; 
  // int16_t rol = ((int16_t)rxPkt[15] << 8) | rxPkt[16]; 
  // int16_t acx = ((int16_t)rxPkt[17] << 8) | rxPkt[18]; 
  // int16_t acy = ((int16_t)rxPkt[19] << 8) | rxPkt[20]; 
  // int16_t acz = ((int16_t)rxPkt[21] << 8) | rxPkt[22]; 
  // int16_t yrc = ((int16_t)rxPkt[23] << 8) | rxPkt[24]; 
  // int16_t prc = ((int16_t)rxPkt[25] << 8) | rxPkt[26]; 
  // int16_t rrc = ((int16_t)rxPkt[27] << 8) | rxPkt[28]; 
  // int16_t sn =  ((int16_t)rxPkt[29] << 8) | rxPkt[30];

  // Serial.println("PDTP Payload contents:");
  // Serial.println("----------------------");
  // Serial.print(tmp); Serial.print(",");
  // Serial.print(kpa); Serial.print(",");
  // Serial.print(alt); Serial.print(",");
  // Serial.print(hum); Serial.print(",");
  // Serial.print(yaw); Serial.print(",");
  // Serial.print(pch); Serial.print(",");
  // Serial.print(rol); Serial.print(",");
  // Serial.print(acx); Serial.print(",");
  // Serial.print(acy); Serial.print(",");
  // Serial.print(acz); Serial.print(",");
  // Serial.print(yrc); Serial.print(",");
  // Serial.print(prc); Serial.print(",");
  // Serial.print(rrc); Serial.print(",");
  // Serial.println(sn);
}
