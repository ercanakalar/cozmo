#pragma once
// DRV8833 dual H-bridge driver for the two tread motors.
// Uses slow-decay PWM (one input held high, the other PWM'd) which gives much
// better low speed torque on small N20 gear motors than fast decay.
#include <Arduino.h>

namespace drivers {

class HBridgeChannel {
 public:
  void begin(int pinIn1, int pinIn2, int ledcChannel1, int ledcChannel2, uint32_t pwmHz,
             bool invert);
  // power in [-1, 1]. 0 = coast.
  void set(float power);
  void brake();
  float power() const { return power_; }

 private:
  static constexpr int kBits = 10;
  static constexpr uint32_t kMax = (1u << kBits) - 1;
  int ch1_ = 0;
  int ch2_ = 0;
  bool invert_ = false;
  float power_ = 0.0f;
};

class Drv8833 {
 public:
  struct Pins {
    int leftIn1, leftIn2, rightIn1, rightIn2;
  };
  void begin(const Pins& pins, uint32_t pwmHz, bool invertLeft, bool invertRight);
  void set(float left, float right);
  void brake();
  void coast() { set(0.0f, 0.0f); }
  HBridgeChannel& left() { return left_; }
  HBridgeChannel& right() { return right_; }

 private:
  HBridgeChannel left_;
  HBridgeChannel right_;
};

}  // namespace drivers
