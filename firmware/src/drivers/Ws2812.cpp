#include "Ws2812.h"

namespace drivers {

namespace {
// RMT clock = 80 MHz / 2 = 40 MHz -> 25 ns per tick.
constexpr uint8_t kClockDiv = 2;
constexpr uint16_t kT0H = 16;  // 0.40 us
constexpr uint16_t kT0L = 34;  // 0.85 us
constexpr uint16_t kT1H = 32;  // 0.80 us
constexpr uint16_t kT1L = 18;  // 0.45 us
}  // namespace

bool Ws2812::begin(int pin, int count, rmt_channel_t channel) {
  channel_ = channel;
  count_ = std::min(count, kMaxLeds);
  rmt_config_t config = RMT_DEFAULT_CONFIG_TX(static_cast<gpio_num_t>(pin), channel_);
  config.clk_div = kClockDiv;
  config.mem_block_num = 2;
  if (rmt_config(&config) != ESP_OK) return false;
  if (rmt_driver_install(channel_, 0, 0) != ESP_OK) return false;
  ok_ = true;
  fill({0, 0, 0});
  show();
  return true;
}

void Ws2812::set(int index, Rgb color) {
  if (index >= 0 && index < count_) pixels_[index] = color;
}

void Ws2812::fill(Rgb color) {
  for (int i = 0; i < count_; ++i) pixels_[i] = color;
}

void Ws2812::show() {
  if (!ok_) return;
  int n = 0;
  for (int i = 0; i < count_; ++i) {
    // WS2812 expects GRB order, MSB first.
    const uint8_t bytes[3] = {
        static_cast<uint8_t>(pixels_[i].g * brightness_ / 255),
        static_cast<uint8_t>(pixels_[i].r * brightness_ / 255),
        static_cast<uint8_t>(pixels_[i].b * brightness_ / 255),
    };
    for (uint8_t byte : bytes) {
      for (int bit = 7; bit >= 0; --bit) {
        const bool one = byte & (1u << bit);
        items_[n].level0 = 1;
        items_[n].duration0 = one ? kT1H : kT0H;
        items_[n].level1 = 0;
        items_[n].duration1 = one ? kT1L : kT0L;
        ++n;
      }
    }
  }
  rmt_write_items(channel_, items_, n, true);  // the >50 us idle low latches the data
}

}  // namespace drivers
