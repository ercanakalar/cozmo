#include "I2sSpeaker.h"

#include <cmath>

namespace drivers {

bool I2sSpeaker::begin(i2s_port_t port, const Pins& pins, uint32_t sampleRate, size_t bufferBytes) {
  port_ = port;
  sampleRate_ = sampleRate;

  i2s_config_t config = {};
  config.mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_TX);
  config.sample_rate = sampleRate;
  config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  config.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;  // we duplicate mono to both slots
  config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  config.dma_buf_count = 6;
  config.dma_buf_len = 256;
  config.use_apll = false;
  config.tx_desc_auto_clear = true;  // output silence on underrun instead of a buzz

  i2s_pin_config_t pinConfig = {};
  pinConfig.mck_io_num = I2S_PIN_NO_CHANGE;
  pinConfig.bck_io_num = pins.bclk;
  pinConfig.ws_io_num = pins.lrc;
  pinConfig.data_out_num = pins.din;
  pinConfig.data_in_num = I2S_PIN_NO_CHANGE;

  if (i2s_driver_install(port_, &config, 0, nullptr) != ESP_OK) return false;
  if (i2s_set_pin(port_, &pinConfig) != ESP_OK) return false;
  i2s_zero_dma_buffer(port_);

  queue_ = xStreamBufferCreate(bufferBytes, sizeof(int16_t));
  if (queue_ == nullptr) return false;
  xTaskCreatePinnedToCore(&I2sSpeaker::taskEntry, "speaker", 4096, this, 5, nullptr, 0);
  return true;
}

size_t I2sSpeaker::write(const int16_t* samples, size_t count, TickType_t timeout) {
  if (queue_ == nullptr) return 0;
  const size_t sent = xStreamBufferSend(queue_, samples, count * sizeof(int16_t), timeout);
  return sent / sizeof(int16_t);
}

void I2sSpeaker::tone(float startHz, float endHz, uint32_t durationMs, float amplitude) {
  const size_t total = sampleRate_ * durationMs / 1000;
  const size_t fade = std::min<size_t>(total / 4, sampleRate_ / 200);  // <= 5 ms
  int16_t block[128];
  float phase = 0.0f;
  size_t n = 0;
  while (n < total) {
    const size_t len = std::min<size_t>(128, total - n);
    for (size_t i = 0; i < len; ++i, ++n) {
      const float t = static_cast<float>(n) / total;
      const float freq = startHz + (endHz - startHz) * t;
      phase += 2.0f * PI * freq / sampleRate_;
      if (phase > 2.0f * PI) phase -= 2.0f * PI;
      float env = 1.0f;
      if (n < fade) env = static_cast<float>(n) / fade;
      if (total - n < fade) env = static_cast<float>(total - n) / fade;
      // Slightly square-ish sine sounds more "robotic" on tiny speakers.
      const float s = sinf(phase);
      const float shaped = 0.8f * s + 0.2f * (s > 0 ? 1.0f : -1.0f);
      block[i] = static_cast<int16_t>(shaped * env * amplitude * 32767.0f);
    }
    write(block, len, pdMS_TO_TICKS(200));
  }
}

void I2sSpeaker::silence(uint32_t durationMs) {
  int16_t zeros[128] = {};
  size_t total = sampleRate_ * durationMs / 1000;
  while (total > 0) {
    const size_t len = std::min<size_t>(128, total);
    write(zeros, len, pdMS_TO_TICKS(200));
    total -= len;
  }
}

void I2sSpeaker::flushQueue() {
  if (queue_ != nullptr) xStreamBufferReset(queue_);
}

void I2sSpeaker::taskEntry(void* arg) { static_cast<I2sSpeaker*>(arg)->run(); }

void I2sSpeaker::run() {
  int16_t mono[256];
  int16_t stereo[512];
  for (;;) {
    const size_t bytes = xStreamBufferReceive(queue_, mono, sizeof(mono), pdMS_TO_TICKS(20));
    const size_t count = bytes / sizeof(int16_t);
    if (count == 0) {
      level_ = level_ * 0.8f;
      continue;
    }
    const float vol = volume_;
    float peak = 0.0f;
    for (size_t i = 0; i < count; ++i) {
      const int16_t s = static_cast<int16_t>(mono[i] * vol);
      stereo[2 * i] = s;
      stereo[2 * i + 1] = s;
      peak = std::max(peak, fabsf(static_cast<float>(mono[i])));
    }
    level_ = 0.6f * level_ + 0.4f * (peak / 32768.0f);
    lastPlayMs_ = millis();
    size_t written = 0;
    i2s_write(port_, stereo, count * 2 * sizeof(int16_t), &written, portMAX_DELAY);
  }
}

}  // namespace drivers
