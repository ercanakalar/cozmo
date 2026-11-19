#include "WheelEncoder.h"

namespace drivers {

void WheelEncoder::begin(int pin) {
  // GPIO34..39 have no internal pull-ups: fit a 10k resistor to 3V3 there.
  pinMode(pin, pin >= 34 ? INPUT : INPUT_PULLUP);
  attachInterruptArg(digitalPinToInterrupt(pin), &WheelEncoder::onEdge, this, RISING);
}

void IRAM_ATTR WheelEncoder::onEdge(void* arg) {
  auto* self = static_cast<WheelEncoder*>(arg);
  const uint32_t now = micros();
  // Reject glitches faster than any real edge (~ 25 kHz).
  if (now - self->lastEdgeUs_ < 40) return;
  self->lastEdgeUs_ = now;
  self->ticks_ = self->ticks_ + 1;
}

uint32_t WheelEncoder::takeDelta() {
  const uint32_t now = ticks_;
  const uint32_t delta = now - lastRead_;
  lastRead_ = now;
  return delta;
}

}  // namespace drivers
