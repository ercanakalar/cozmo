#pragma once
// Single-channel hall encoder (N20 motor encoder, channel A).
// Direction is not decoded: the drive controller knows which way it commands
// the motor, which saves two GPIOs on the pin-starved ESP32.
#include <Arduino.h>

namespace drivers {

class WheelEncoder {
 public:
  void begin(int pin);
  uint32_t ticks() const { return ticks_; }
  // Ticks seen since the previous call.
  uint32_t takeDelta();

 private:
  static void IRAM_ATTR onEdge(void* arg);
  volatile uint32_t ticks_ = 0;
  volatile uint32_t lastEdgeUs_ = 0;
  uint32_t lastRead_ = 0;
};

}  // namespace drivers
