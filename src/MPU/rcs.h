/***********************************************************************************************/
/*********************************** REACTION CONTROL SYSTEM ***********************************/
/***********************************************************************************************/
void rcs(float *ypr) {
  imu.getEvent(&a, &g, &temp);
  // Convert Yaw/Pitch/Roll to readable values   
  ypr[0] = g.gyro.z * 180 / M_PI;
  ypr[1] = g.gyro.x * 180 / M_PI;
  ypr[2] = g.gyro.y * 180 / M_PI;

  // Add Yaw/Pitch/Roll values to Kalman Filter(s)
  ypr[0] = yawKalmanFilter.updateEstimate(ypr[0]);
  ypr[1] = pitchKalmanFilter.updateEstimate(ypr[1]);
  ypr[2] = rollKalmanFilter.updateEstimate(ypr[2]);

  // {/********************************/}
  // {/* !! SOMETHING IS WRONG HERE !!*/}
  // {/********************************/}
  if (RCS) {
    // If the vehicle is actively in flight OR if we're doing debug testing, run the RCS program.
    if (vehicleState == ASCENT || String(debugState) == "ON") {
      // Map the values of the MPU6050 sensor from -90 to 90 to values suitable for the servo control from 0 to 180
      int servo0Value = map(ypr[2], -90, 90, 0, 180);
      int servo1Value = map(ypr[2], -90, 90, 0, 180);
      int servo2Value = map(ypr[2], -90, 90, 0, 180);
      int servo3Value = map(ypr[2], -90, 90, 0, 180);

      // Control the servos according to the MPU6050 orientation
      servo0.write(servo0Value);
      servo1.write(servo1Value);
      servo2.write(servo2Value);
      servo3.write(servo3Value);
    }
  }
}