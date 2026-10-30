/*******************/
/** -== Clock ==- **/
/*******************/
void clk() {
//  unsigned long currentTime = millis();
  if (clockEnable == true) {
    if (currentTime - clockTime >= (clockPeriod/2)) { // Divide clock period by half to include high and low clock cycles
      clockTime = currentTime;
      if (clockState == CLK_LO) {
        clockState = CLK_HI;
        // Serial.println("SETTING CLK HIGH");
      } else {
        clockState = CLK_LO;
        // Serial.println("SETTING CLK LOW");
      }
    }
  }
}
