#include "ServoPwm.h"

namespace drivers {

void ServoPwm::begin(int pin, int ledcChannel) {
  pin_ = pin;
  channel_ = ledcChannel;
  ledcSetup(channel_, kFreq, kBits);
  ledcAttachPin(pin_, channel_);
  ledcWrite(channel_, 0);
}

void ServoPwm::writeMicroseconds(int us) {
  us = constrain(us, 500, 2500);
  lastUs_ = us;
  constexpr uint32_t kPeriodUs = 1000000 / kFreq;
  const uint32_t duty = static_cast<uint32_t>(us) * ((1u << kBits) - 1) / kPeriodUs;
  ledcWrite(channel_, duty);
}

void ServoPwm::release() { ledcWrite(channel_, 0); }

}  // namespace drivers
