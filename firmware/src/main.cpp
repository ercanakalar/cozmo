// Cozmo replica - ESP32 firmware entry point.
//
// Task layout:
//   core 1  loop()        control loop @ ~100 Hz (commands, drive, servos, personality)
//   core 1  "display"     IMU + 25 fps eye rendering (owns the I2C bus)
//   core 0  WiFi stack, "mic" (I2S -> UDP), "spk_rx" (UDP -> queue), "speaker" (queue -> I2S)
#include <Arduino.h>

#include "Robot.h"

void setup() {
  Serial.begin(115200);
  delay(50);
  Serial.println();
  Serial.println("Cozmo replica firmware starting");
  Robot::instance().begin();
}

void loop() {
  Robot::instance().loop();
  delay(10);
}
