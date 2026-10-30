/******************************************************************************************************/
/*********************************** Payload Data Transmit Protocol ***********************************/
/******************************************************************************************************/
/** << 32 byte wide packet >>
 * FS Byte - 1 byte
 * Dest Address - 1 byte
 * Src Address - 1 byte
 * Payload - 26 bytes
 * Seq Num - 1 byte
 * FCSN - 2 bytes
 * @param packet
 */
void pdtpTx() {
  // TODO - Update SPU PDTP functions to support the operations the computer can execute (i.e. no sensor data)
  byte pdtpPacket[32]; // Initialize the data packet as an array of bytes
  
  byte sourceAddress = 130; // SPU
  byte destAddress = 115; // MPU
  byte pad = 0;
  char message[3] = "ACK";

  // Now, calculate the CRC for the payload byte array
  // uint8_t FCSN = 0;
  // uint8_t FCSN = calculateCRC16(message, sizeof(message));
  
  // Populate pdtpPacket
  // 1. FS Byte (1 byte)
  // pdtpPacket[0] = statusByte;

  // 2. Destination Address (1 byte)
  // pdtpPacket[1] = destAddress;

  // 3. Source Address (1 byte)
  // pdtpPacket[2] = sourceAddress;

  // // 4. Payload (26 bytes)
  // TODO - Calculate pad based on 26 bytes - message size in bytes
  // pdtpPacket[3] = message[0];
  // pdtpPacket[4] = message[1];
  // pdtpPacket[5] = message[2]; 
  // pdtpPacket[6] = pad;
  // pdtpPacket[7] = pad; 
  // pdtpPacket[8] = pad;
  // pdtpPacket[9] = pad; 
  // pdtpPacket[10] = pad;
  // pdtpPacket[11] = pad; 
  // pdtpPacket[12] = pad;
  // pdtpPacket[13] = pad; 
  // pdtpPacket[14] = pad;
  // pdtpPacket[15] = pad; 
  // pdtpPacket[16] = pad;
  // pdtpPacket[17] = pad; 
  // pdtpPacket[18] = pad;
  // pdtpPacket[19] = pad; 
  // pdtpPacket[20] = pad;
  // pdtpPacket[21] = pad; 
  // pdtpPacket[22] = pad;
  // pdtpPacket[23] = pad; 
  // pdtpPacket[24] = pad;
  // pdtpPacket[25] = pad; 
  // pdtpPacket[26] = pad;
  // pdtpPacket[27] = pad; 
  // pdtpPacket[28] = pad;

  // // // 5. Sequence Number (2 bytes)
  // // sequenceNumber = sequenceNumber++;
  // pdtpPacket[29] = 0;
  // pdtpPacket[30] = 0;
  // // pdtpPacket[29] = (sequenceNumber >> 8);
  // // pdtpPacket[30] = sequenceNumber & 0xFF;
  
  // // 6. FCSN (1 byte)
  // pdtpPacket[31] = 0;

  Wire.beginTransmission(0x73); // 0x73 is the Big Endian value of 115, the MPU address
  Wire.write(message, 3);
  Wire.endTransmission();

  delay(10);
};

/** << 32 byte wide packet >>
 * FS Byte - 1 byte
 * Dest Address - 1 byte
 * Src Address - 1 byte
 * Payload - 26 bytes
 * Seq Num - 1 byte
 * FCSN - 2 bytes
 */
void pdtpRx(PDTP_Packet& packet) {
  uint8_t rxPkt[32];
  while (1 < Wire.available()) {
    for (int b = 0; b < 32; b++) {
      rxPkt[b] = Wire.read();
    }
  }

  // Extract the PDTP packet attributes
  uint8_t statusByte = rxPkt[0];
  uint8_t destAddr = rxPkt[1];
  uint8_t srcAddr = rxPkt[2];
  uint8_t crc = rxPkt[31];

  packet.payload.tmp = ((int16_t)rxPkt[3] << 8) | rxPkt[4]; 
  packet.payload.kpa = ((int16_t)rxPkt[5] << 8) | rxPkt[6]; 
  packet.payload.alt = ((int16_t)rxPkt[7] << 8) | rxPkt[8]; 
  packet.payload.hum = ((int16_t)rxPkt[9] << 8) | rxPkt[10]; 
  packet.payload.yaw = ((int16_t)rxPkt[11] << 8) | rxPkt[12]; 
  packet.payload.pch = ((int16_t)rxPkt[13] << 8) | rxPkt[14]; 
  packet.payload.rol = ((int16_t)rxPkt[15] << 8) | rxPkt[16]; 
  packet.payload.acx = ((int16_t)rxPkt[17] << 8) | rxPkt[18]; 
  packet.payload.acy = ((int16_t)rxPkt[19] << 8) | rxPkt[20]; 
  packet.payload.acz = ((int16_t)rxPkt[21] << 8) | rxPkt[22]; 
  packet.payload.yrc = ((int16_t)rxPkt[23] << 8) | rxPkt[24]; 
  packet.payload.prc = ((int16_t)rxPkt[25] << 8) | rxPkt[26]; 
  packet.payload.rrc = ((int16_t)rxPkt[27] << 8) | rxPkt[28]; 
  int16_t sn =  ((int16_t)rxPkt[29] << 8) | rxPkt[30];

  decodeCRC8(packet.bytes, crc);

  String pdtpTxt = "";
  pdtpTxt += String(packet.payload.tmp); pdtpTxt += ",";
  pdtpTxt += String(packet.payload.kpa); pdtpTxt += ",";
  pdtpTxt += String(packet.payload.alt); pdtpTxt += ",";
  pdtpTxt += String(packet.payload.hum); pdtpTxt += ",";
  pdtpTxt += String(packet.payload.yaw); pdtpTxt += ",";
  pdtpTxt += String(packet.payload.pch); pdtpTxt += ",";
  pdtpTxt += String(packet.payload.rol); pdtpTxt += ",";
  pdtpTxt += String(packet.payload.acx); pdtpTxt += ",";
  pdtpTxt += String(packet.payload.acy); pdtpTxt += ",";
  pdtpTxt += String(packet.payload.acz); pdtpTxt += ",";
  pdtpTxt += String(packet.payload.yrc); pdtpTxt += ",";
  pdtpTxt += String(packet.payload.prc); pdtpTxt += ",";
  pdtpTxt += String(packet.payload.rrc); pdtpTxt += ",";
  pdtpTxt += String(sn);

  // Serial.println("PDTP Payload contents:");
  // Serial.println("----------------------");
  File dataFile = SD.open("pdtp.txt", FILE_WRITE);

  if (dataFile) {
    dataFile.println(pdtpTxt);
    dataFile.close();
    Serial.println(pdtpTxt);
  } else {
    Serial.println("Error opening datalogger output file!!");
    Serial.println(pdtpTxt);
  }

  if (TELEMETRY == "ACTIVE") {
    Serial.println("Transmiting packet over RF");
    rf95.send((uint8_t *)rxPkt, 32);

    Serial.println("Waiting for packet to complete");
    delay(100);
    // rf95.waitPacketSent();
  }
}


void pdtp_rx() {
  pdtpRx(pldPkt);
}
