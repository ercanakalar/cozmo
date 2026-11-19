#pragma once
// MAX98357A I2S class-D amplifier driver.
// Audio (network TTS or locally synthesised chirps) is pushed into a FreeRTOS
// stream buffer; a background task drains it into the I2S DMA.
#include <Arduino.h>
#include <driver/i2s.h>
#include <freertos/stream_buffer.h>

namespace drivers {

class I2sSpeaker {
 public:
  struct Pins {
    int bclk;
    int lrc;
    int din;
  };

  bool begin(i2s_port_t port, const Pins& pins, uint32_t sampleRate, size_t bufferBytes = 32768);

  // Queue mono int16 samples. Returns number of samples accepted.
  size_t write(const int16_t* samples, size_t count, TickType_t timeout = 0);
  // Synthesise a tone sweep (sine with soft attack/release) into the queue.
  void tone(float startHz, float endHz, uint32_t durationMs, float amplitude = 0.5f);
  void silence(uint32_t durationMs);
  void flushQueue();

  void setVolume(float volume) { volume_ = constrain(volume, 0.0f, 1.0f); }
  float volume() const { return volume_; }
  // True while audio was played in the last ~150 ms.
  bool active() const { return millis() - lastPlayMs_ < 150; }
  // Output envelope 0..1, used by the face to "talk" with the eyes.
  float level() const { return level_; }
  uint32_t sampleRate() const { return sampleRate_; }

 private:
  static void taskEntry(void* arg);
  void run();

  i2s_port_t port_ = I2S_NUM_1;
  uint32_t sampleRate_ = 16000;
  StreamBufferHandle_t queue_ = nullptr;
  volatile float volume_ = 0.7f;
  volatile float level_ = 0.0f;
  volatile uint32_t lastPlayMs_ = 0;
};

}  // namespace drivers
