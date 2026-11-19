#include "I2sMicrophone.h"

#include <cmath>

namespace drivers {

bool I2sMicrophone::begin(i2s_port_t port, const Pins& pins, uint32_t sampleRate, int gainShift,
                          bool rightSlot) {
  port_ = port;
  gainShift_ = gainShift;

  i2s_config_t config = {};
  config.mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_RX);
  config.sample_rate = sampleRate;
  config.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT;  // INMP441 sends 24 bits in a 32 bit slot
  config.channel_format = rightSlot ? I2S_CHANNEL_FMT_ONLY_RIGHT : I2S_CHANNEL_FMT_ONLY_LEFT;
  config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  config.dma_buf_count = 6;
  config.dma_buf_len = 256;
  config.use_apll = false;

  i2s_pin_config_t pinConfig = {};
  pinConfig.mck_io_num = I2S_PIN_NO_CHANGE;
  pinConfig.bck_io_num = pins.bclk;
  pinConfig.ws_io_num = pins.ws;
  pinConfig.data_out_num = I2S_PIN_NO_CHANGE;
  pinConfig.data_in_num = pins.data;

  if (i2s_driver_install(port_, &config, 0, nullptr) != ESP_OK) return false;
  if (i2s_set_pin(port_, &pinConfig) != ESP_OK) return false;
  i2s_zero_dma_buffer(port_);
  ok_ = true;
  return true;
}

size_t I2sMicrophone::read(int16_t* out, size_t count, TickType_t timeout) {
  if (!ok_) return 0;
  size_t produced = 0;
  double sumSquares = 0.0;
  while (produced < count) {
    const size_t want = std::min(count - produced, sizeof(raw_) / sizeof(raw_[0]));
    size_t bytesRead = 0;
    if (i2s_read(port_, raw_, want * sizeof(int32_t), &bytesRead, timeout) != ESP_OK) break;
    const size_t got = bytesRead / sizeof(int32_t);
    if (got == 0) break;
    for (size_t i = 0; i < got; ++i) {
      // DC blocker: y[n] = x[n] - x[n-1] + R * y[n-1]
      const float x = static_cast<float>(raw_[i] >> gainShift_);
      const float y = x - dcPrevIn_ + 0.995f * dcPrevOut_;
      dcPrevIn_ = x;
      dcPrevOut_ = y;
      const int32_t s = static_cast<int32_t>(std::max(-32768.0f, std::min(32767.0f, y)));
      out[produced + i] = static_cast<int16_t>(s);
      sumSquares += static_cast<double>(s) * s;
    }
    produced += got;
  }
  if (produced > 0) lastRms_ = static_cast<int>(sqrt(sumSquares / produced));
  return produced;
}

}  // namespace drivers
