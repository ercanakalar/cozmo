#pragma once
// INMP441 (or any 24-bit I2S MEMS) microphone driver on an ESP32 I2S port.
// Delivers 16 kHz mono int16 PCM with DC offset removed.
#include <Arduino.h>
#include <driver/i2s.h>

namespace drivers {

class I2sMicrophone {
 public:
  struct Pins {
    int bclk;
    int ws;
    int data;
  };

  bool begin(i2s_port_t port, const Pins& pins, uint32_t sampleRate, int gainShift);
  // Blocks until `count` samples are read (or timeout). Returns samples read.
  size_t read(int16_t* out, size_t count, TickType_t timeout = portMAX_DELAY);
  // RMS of the last read() block, 0..32767.
  int lastRms() const { return lastRms_; }

 private:
  i2s_port_t port_ = I2S_NUM_0;
  int gainShift_ = 11;
  int32_t raw_[256];
  // One-pole DC blocking filter state.
  float dcPrevIn_ = 0.0f;
  float dcPrevOut_ = 0.0f;
  volatile int lastRms_ = 0;
  bool ok_ = false;
};

}  // namespace drivers
