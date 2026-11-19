#pragma once
// Capacitive "pet me" sensor: a piece of copper tape inside the head shell,
// wired to an ESP32 touch pin. Self-calibrating baseline.
#include <Arduino.h>

namespace drivers {

class TouchPad {
 public:
  void begin(int pin);
  void update();  // call ~20 Hz
  bool touched() const { return touched_; }
  uint32_t touchedForMs() const { return touched_ ? millis() - touchStartMs_ : 0; }

 private:
  int pin_ = -1;
  float baseline_ = 0.0f;
  bool touched_ = false;
  uint32_t touchStartMs_ = 0;
};

}  // namespace drivers
