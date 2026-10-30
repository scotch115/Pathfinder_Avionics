/********************/
/** Health Monitor **/
/********************/
/*************************************************************************************/
/* While power is on, generate various audio and visual indications of vehicle state */
/*************************************************************************************/
/**
 * @note -== Health Monitor ==-
 * @note STATES:
 * @note - HEALTHY/SIGNAL GOOD
 * @note - QUEASY/SIGNAL WEAK
 * @note - ERROR/SENSOR FAILURE/NO SIGNAL
 */
void HM(unsigned long clockReadInterval, int pitch, bool single) {
  if (clockEnable == true) {
    if (hmTime - prevTime >= clockReadInterval) {
      prevTime = hmTime;
      if (clockState == CLK_HI) {
        blep(BUZZER_PIN, pitch, 50);
      } else {
        if (!single) {
          blep(BUZZER_PIN, (pitch-100), 50);
        }
      }
    }
  } else {
      blep(BUZZER_PIN, pitch, 50);
  }

  // TODO - Implement some sort of check within the health monitor to detect the health states above

  /** 
    * If flight computer has not launched or changed state after two minutes of being active,
    * set the launch state to 'IDLE'
    */
  if (missionTime >= 120000 && vehicleState == ARMED) {
    Serial.println("Setting state to IDLE");
    vehicleState = IDLE;
  }

  /**
    * If flight computer has not launched or changed state after 5 minutes of being active,
    * set the launch state to 'IDLE'
    */
  if (missionTime >= 300000 && vehicleState == IDLE) {
    Serial.println("Setting state to DEEPSLEEP");
    vehicleState = DEEPSLEEP;
  }
}
