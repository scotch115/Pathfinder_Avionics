/*********************************************************************************************************************************************************************/
/*********************************************************************************************************************************************************************/
/************************************************************************ Auxillary Functions ************************************************************************/
/*********************************************************************************************************************************************************************/
/*********************************************************************************************************************************************************************/
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
