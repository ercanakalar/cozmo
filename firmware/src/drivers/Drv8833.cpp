#include "Drv8833.h"

namespace drivers {

void HBridgeChannel::begin(int pinIn1, int pinIn2, int ledcChannel1, int ledcChannel2,
                           uint32_t pwmHz, bool invert) {
  ch1_ = ledcChannel1;
  ch2_ = ledcChannel2;
  invert_ = invert;
  ledcSetup(ch1_, pwmHz, kBits);
  ledcSetup(ch2_, pwmHz, kBits);
  ledcAttachPin(pinIn1, ch1_);
  ledcAttachPin(pinIn2, ch2_);
  set(0.0f);
}

void HBridgeChannel::set(float power) {
  power = constrain(power, -1.0f, 1.0f);
  power_ = power;
  if (invert_) power = -power;
  if (fabsf(power) < 0.01f) {  // coast
    ledcWrite(ch1_, 0);
    ledcWrite(ch2_, 0);
    return;
  }
  // Slow decay: hold one side high, PWM the other side with inverted duty.
  const uint32_t low = static_cast<uint32_t>((1.0f - fabsf(power)) * kMax);
  if (power > 0) {
    ledcWrite(ch1_, kMax);
    ledcWrite(ch2_, low);
  } else {
    ledcWrite(ch1_, low);
    ledcWrite(ch2_, kMax);
  }
}

void HBridgeChannel::brake() {
  power_ = 0.0f;
  ledcWrite(ch1_, kMax);
  ledcWrite(ch2_, kMax);
}

void Drv8833::begin(const Pins& pins, uint32_t pwmHz, bool invertLeft, bool invertRight) {
  // LEDC channels 0..3 (timers 0 and 1). Servos use channels 4..5.
  left_.begin(pins.leftIn1, pins.leftIn2, 0, 1, pwmHz, invertLeft);
  right_.begin(pins.rightIn1, pins.rightIn2, 2, 3, pwmHz, invertRight);
}

void Drv8833::set(float left, float right) {
  left_.set(left);
  right_.set(right);
}

void Drv8833::brake() {
  left_.brake();
  right_.brake();
}

}  // namespace drivers
