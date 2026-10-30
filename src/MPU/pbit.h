/***********************************************************************************************/
/*********************************** Power-on Built In Test   **********************************/
/***********************************************************************************************/
void preCheck() {
  vehicleState = STARTUP;

  Serial.println(); Serial.println();
  Serial.println("Beginning Pre-Flight Software check.");
  Serial.println();
  delay(2000);
  Serial.print("Flight Computer State: ");Serial.println(vehicleState);

  // WIP
  // delay(200);
  // continuity(LEDPIN, 1);
  // delay(200);
  // continuity(BUZZER_PIN, 0);

  pinMode(LEDPIN, OUTPUT);

  // Play bootup sequence sound
  if (!mute) {
    for (unsigned int i = 0; i < numTones; i++) {
      tone(BUZZER_PIN, tones[i]);
      delay(50);
    }
    noTone(BUZZER_PIN);
  };
  delay(2000);

  Serial.print("Let's see if the BME280 Barometer/Altimeter is connected. Standby");
  delay(500);
  Serial.print(".");
  delay(500);
  Serial.print(".");
  delay(500);
  Serial.println(".");

  // Initialize and test BME280
  if (!bme.begin()) { // I2C address is 0x76
    Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
       vehicleState = ERROR;
  } else {
    String bmeMsg = "Found the BME280 sensor! Here's some data...";
    // TODO - IF we want to save these messages to the SD card, we can implement
    // a Wire.beginTransmission here, but we don't need that for now
    Serial.println(bmeMsg);
    delay(2000);

    blep(BUZZER_PIN, 523, 50);

    Serial.print("Temperature = "); Serial.print(bme.readTemperature()); Serial.println(" *C");
    Serial.print("Pressure = "); Serial.print((bme.readPressure() / 100.0F)); Serial.println(" Pa"); // Need to update this to kPa to not break PDTP
    Serial.print("Approx Altitude = "); Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA)); Serial.println(" m");
    Serial.print("Humidity = "); Serial.print(bme.readHumidity()); Serial.println(" %");
    Serial.println();
    // }
  }

  Serial.print("Now we'll look for the MPU6050 IMU. Standby");
  delay(500);
  Serial.print(".");
  delay(500);
  Serial.print(".");
  delay(500);
  Serial.println(".");

  // Initialize and test Intertial Measurement Unit (IMU) [MPU6050]
  if (!imu.begin()) { // I2C address is 0x68; can be set to 0x69
    Serial.println("Could not find a valid MPU6050 sensor, check wiring, address, sensor ID!");
    vehicleState = ERROR;
  } else {
    String imuMsg = "Found it! MPU6050 Connection Successful.";
    Serial.println(imuMsg); // TODO - Same as above wrt SD card


    blep(BUZZER_PIN, 523, 50);
    // TODO - Need to add error detection
    // TODO II - Check calibration/settings below
    imu.setAccelerometerRange(MPU6050_RANGE_8_G);
    Serial.print("Accelerometer range set to: ");
    switch (imu.getAccelerometerRange()) {
      case MPU6050_RANGE_2_G:
        Serial.println("+-2G");
        break;
      case MPU6050_RANGE_4_G:
        Serial.println("+-4G");
        break;
      case MPU6050_RANGE_8_G:
        Serial.println("+-8G");
        break;
      case MPU6050_RANGE_16_G:
        Serial.println("+-16G");
        break;
    }
    imu.setGyroRange(MPU6050_RANGE_500_DEG);
    Serial.print("Gyro range set to: ");
    switch (imu.getGyroRange()) {
      case MPU6050_RANGE_250_DEG:
        Serial.println("+- 250 deg/s");
        break;
      case MPU6050_RANGE_500_DEG:
        Serial.println("+- 500 deg/s");
        break;
      case MPU6050_RANGE_1000_DEG:
        Serial.println("+- 1000 deg/s");
        break;
      case MPU6050_RANGE_2000_DEG:
        Serial.println("+- 2000 deg/s");
        break;
    }
    imu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    Serial.print("Filter bandwidth set to: ");
    switch (imu.getFilterBandwidth()) {
      case MPU6050_BAND_260_HZ:
        Serial.println("260 Hz");
        break;
      case MPU6050_BAND_184_HZ:
        Serial.println("184 Hz");
        break;
      case MPU6050_BAND_94_HZ:
        Serial.println("94 Hz");
        break;
      case MPU6050_BAND_44_HZ:
        Serial.println("44 Hz");
        break;
      case MPU6050_BAND_21_HZ:
        Serial.println("21 Hz");
        break;
      case MPU6050_BAND_10_HZ:
        Serial.println("10 Hz");
        break;
      case MPU6050_BAND_5_HZ:
        Serial.println("5 Hz");
        break;
    }

    Serial.println(); Serial.println();
    Serial.println("Here's a little bit of data!");
    for (int i = 0; i < 20; i++) {
      imu.getEvent(&a, &g, &temp);

      /* Print out the values */
      Serial.print("Acceleration X: ");
      Serial.print(a.acceleration.x);
      Serial.print(", Y: ");
      Serial.print(a.acceleration.y);
      Serial.print(", Z: ");
      Serial.print(a.acceleration.z);
      Serial.println(" m/s^2");

      Serial.print("Rotation X: ");
      Serial.print(g.gyro.x);
      Serial.print(", Y: ");
      Serial.print(g.gyro.y);
      Serial.print(", Z: ");
      Serial.print(g.gyro.z);
      Serial.println(" rad/s");
 
      Serial.print("Temperature: ");
      Serial.print(temp.temperature);
      Serial.println(" degC");
      delay(10);
      Serial.println(); Serial.println();
    }
    blep(BUZZER_PIN, 523, 50);
  }

  if (String(DEFLECTION_TEST) == "ACTIVE") {
    delay(2000);
    Serial.println("Now let's test the fins! Connecting to servos to perform Deflection Test, standby...");
    Serial.println();Serial.println();
    deflectionTest(); //? [WIP]
    blep(BUZZER_PIN, 523, 50);
  }

  //? [4/23/2026] - Disabling all RF-related functions until basic functions have been fixed
  //   // If TELEMETRY value is not accurately set, abort the flight program.
  //   if ((TELEMETRY != "ACTIVE") || (TELEMETRY != "LOCAL") || (TELEMETRY != "INACTIVE")) {
  //     vehicleState = "ERROR";
  //   }

  //   if (TELEMETRY == "ACTIVE") {
  //     Serial.println();
  //     Serial.println("LoRa Radio module found!");

  //     // Reset LoRa Module
  //     digitalWrite(RFM95_RST, LOW);
  //     delay(10);
  //     digitalWrite(RFM95_RST, HIGH);
  //     delay(10);

  //     while (!rf95.init()) {
  //       Serial.println("LoRa radio init failed");
  //       Serial.println("Uncomment '#define SERIAL_DEBUG' in RH_RF95.cpp for detailed debug info");
  //       while (1);
  //     }
  //     Serial.println("LoRa radio initialized!");

  //     if (!rf95.setFrequency(RF95_FREQ)) {
  //       Serial.println("setFrequency failed");
  //       while (1);
  //     }
  //     Serial.print("Frequency set to: "); Serial.println(RF95_FREQ);

  //     rf95.setTxPower(23, false);
  //   }

  // //  if (TELEMETRY == "LOCAL") {
  // //    Serial.println();
  // //    Serial.print("Telemtry set to 'LOCAL'. Disabling LoRa Module.");
  // //    SPI.begin();
  // //    pinMode(RFM95_CS, OUTPUT);
  // //    digitalWrite(RFM95_CS, HIGH);
  // //    delay(100);
  // //    Serial.print(".");
  // //    delay(500);
  // //    Serial.print(".");
  // //    delay(500);
  // //    Serial.println(".");
  // //
  // //    /*******************************************************************************************/
  // //    /* ! WIP - Dynamically change communication method as signal strength increases/decreases? */
  // //    /*******************************************************************************************/
  // //    Serial.print("LoRa Module disabled! Testing if WIFI Module is connected and operational.");
  // //    WiFi.setPins(SPIWIFI_SS, SPIWIFI_ACK, ESP32_RESETN, ESP32_GPIO0, &SPIWIFI);
  // //    delay(1000);
  // //    Serial.print(".");
  // //    delay(1000);
  // //    Serial.println(".");
  // //    while (WiFi.status() == WL_NO_MODULE) {
  // //      Serial.println();
  // //      Serial.println("Could not communicate with WiFi module, check system!");
  // //      while (true);
  // //      vehicleState = "ERROR";
  // //    }
  // //    Serial.println("WiFi Module detected!");
  // //    Serial.println("Attempting to connect to network...");
  // //    unsigned wStatus;
  // //
  // //    do {
  // //      wStatus = WiFi.begin(WIFI_SSID, PW);
  // //      delay(100);
  // //    } while (wStatus != WL_CONNECTED);
  // //    // print the SSID of the network you're attached to:
  // //    Serial.print(F("SSID: "));
  // //    Serial.println(WiFi.SSID());
  // //
  // //    // print your board's IP address:
  // //    IPAddress ip = WiFi.localIP();
  // //    Serial.print(F("IP Address: "));
  // //    Serial.println(ip);
  // //
  // //    // print the received signal strength:
  // //    long rssi = WiFi.RSSI();
  // //    Serial.print(F("signal strength (RSSI):"));
  // //    Serial.print(rssi);
  // //    Serial.println(F(" dBm"));
  // //    Udp.begin(localPort);
  // //
  // //    Udp.beginPacket(remoteIp, localPort);
  // //    if (wStatus) {
  // //      Udp.write("INFO - WiFi Module Detected!");
  // //    }
  // //    Udp.endPacket();
  // //  }

  //   if (TELEMETRY == "INACTIVE") {
  //     Serial.println();
  //     Serial.println("Telemetry set to INACTIVE. LoRa and WiFi modules not in use.");
  //     Serial.println();
  //   }
  
  if (vehicleState != ERROR) {
    // String readyMsg = "Flight Computer is Armed and Ready!";
    // char rMessage[200];
    // int r = 0;
    // while((r<200)) {
    //   rMessage[r]=readyMsg[r];
    //   r++;
    // }

    if (STARTLAUNCHSTATE != DEBUG) {
      vehicleState = ARMED;
    } else {
      vehicleState = STARTLAUNCHSTATE;
    }

    Serial.println();
    Serial.print("Flight Computer State: ");Serial.println(vehicleState);

    // Serial.println();Serial.println();
    // Serial.println(rMessage);
    // Serial.println();Serial.println();

      // sendPacket(rMessage);
  }

  blep(BUZZER_PIN, 1760, 500);

  //  delay(1000);

  // Prepare Pyro Channel 1 (PYRO_1) for chute charge ignition
  //   if (PYRO1) {
  //     pinMode(PYRO_1, OUTPUT);
  //     digitalWrite(PYRO_1, LOW);
  //   }
  // }
//  readyForLaunch();
}
