/********************************************************************************************************************************************************************/
/********************************************************************************************************************************************************************/
/********************************************************************* Sensor Data Assimilation *********************************************************************/
/********************************************************************************************************************************************************************/
/********************************************************************************************************************************************************************/
// XYZ Position Calculations
int calculateYawDisplacement(float yaw) {
  // Filter noise from accelerometer
  // float filteredX = xKalmanFilter.updateEstimate(accelX);
//  xVel = xVel + (filteredX * missionTime);
//  xPos = calculateDistance(xPos, xVel, filteredX, missionTime);
  // Calculate rate of change in acceleration
//  float dxAccel = (filteredX - xAccel);
//  if (dxAccel == 0) return xPos; // If the acceleration hasn't changed, then we are not moving
//
//  dxAccel = dxAccel * 9.8; // Cancel out gravity?
//
//  xPos = 0.5 * dxAccel * sq(missionTime);
  // xPos = int(trunc(xKalmanFilter.updateEstimate(accelX)));
  float deltaYaw = yaw * 180 / M_PI;
  xPos = deltaYawFilter.updateEstimate(ypr[0] - deltaYaw);
  return xPos;
}

int calculatePitchDisplacement(float pitch) {
  // yPos = yKalmanFilter.updateEstimate(accelY)));
//  yPos = calculateDistance(yPos, accelY, millis());
  float deltaPitch = pitch * 180 / M_PI;
  yPos = deltaPitchFilter.updateEstimate(ypr[1] - deltaPitch);
  return yPos;
}

int calculateRollDisplacement(float roll) {
  // zPos = zKalmanFilter.updateEstimate(accelZ)));
//  zPos = calculateDistance(zPos, accelZ, millis());
  float deltaRoll = roll * 180 / M_PI;
  zPos = deltaRollFilter.updateEstimate(ypr[2] - deltaRoll);
  return zPos;
}

int calculateAltitude(float alt, float accel) {
  // Set altitude to accelerometer 
  altitude = int(trunc(altitudeFilter.updateEstimate(alt)));
  return altitude;
}

//? [4/22/2026] - idk, we can come back to this once literally everything else works :p
// float calculateDistance(float pos, float velocity, float accel, float t) {
//   // Using the second integration of the acceleration function:
//   float distance = pos + (velocity * missionTime) + (0.5 * accel * sq(missionTime));
//   return distance;
//}

// void getVehicleAltitude() {
//   if (missionTime - prevDeployTime >= 3000) {
//     prevDeployTime = missionTime;
//     altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);
//     if (altitude > lastAlt) {
//       lastAlt = altitude;
//     } else if (altitude < lastAlt && altitude - lastAlt <= -10) {
//       apogee = altitude;
//       delay(150);
//       deploy();
//     } 
//   }
// }
