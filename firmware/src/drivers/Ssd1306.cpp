#include "Ssd1306.h"

#include <cmath>

namespace drivers {

namespace {

// Classic 5x7 column font for ASCII 0x20..0x5A.
const uint8_t kFont5x7[][5] PROGMEM = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x5F, 0x00, 0x00}, {0x00, 0x07, 0x00, 0x07, 0x00},
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, {0x24, 0x2A, 0x7F, 0x2A, 0x12}, {0x23, 0x13, 0x08, 0x64, 0x62},
    {0x36, 0x49, 0x56, 0x20, 0x50}, {0x00, 0x08, 0x07, 0x03, 0x00}, {0x00, 0x1C, 0x22, 0x41, 0x00},
    {0x00, 0x41, 0x22, 0x1C, 0x00}, {0x2A, 0x1C, 0x7F, 0x1C, 0x2A}, {0x08, 0x08, 0x3E, 0x08, 0x08},
    {0x00, 0x80, 0x70, 0x30, 0x00}, {0x08, 0x08, 0x08, 0x08, 0x08}, {0x00, 0x00, 0x60, 0x60, 0x00},
    {0x20, 0x10, 0x08, 0x04, 0x02}, {0x3E, 0x51, 0x49, 0x45, 0x3E}, {0x00, 0x42, 0x7F, 0x40, 0x00},
    {0x72, 0x49, 0x49, 0x49, 0x46}, {0x21, 0x41, 0x49, 0x4D, 0x33}, {0x18, 0x14, 0x12, 0x7F, 0x10},
    {0x27, 0x45, 0x45, 0x45, 0x39}, {0x3C, 0x4A, 0x49, 0x49, 0x31}, {0x41, 0x21, 0x11, 0x09, 0x07},
    {0x36, 0x49, 0x49, 0x49, 0x36}, {0x46, 0x49, 0x49, 0x29, 0x1E}, {0x00, 0x00, 0x14, 0x00, 0x00},
    {0x00, 0x40, 0x34, 0x00, 0x00}, {0x00, 0x08, 0x14, 0x22, 0x41}, {0x14, 0x14, 0x14, 0x14, 0x14},
    {0x00, 0x41, 0x22, 0x14, 0x08}, {0x02, 0x01, 0x59, 0x09, 0x06}, {0x3E, 0x41, 0x5D, 0x59, 0x4E},
    {0x7C, 0x12, 0x11, 0x12, 0x7C}, {0x7F, 0x49, 0x49, 0x49, 0x36}, {0x3E, 0x41, 0x41, 0x41, 0x22},
    {0x7F, 0x41, 0x41, 0x41, 0x3E}, {0x7F, 0x49, 0x49, 0x49, 0x41}, {0x7F, 0x09, 0x09, 0x09, 0x01},
    {0x3E, 0x41, 0x41, 0x51, 0x73}, {0x7F, 0x08, 0x08, 0x08, 0x7F}, {0x00, 0x41, 0x7F, 0x41, 0x00},
    {0x20, 0x40, 0x41, 0x3F, 0x01}, {0x7F, 0x08, 0x14, 0x22, 0x41}, {0x7F, 0x40, 0x40, 0x40, 0x40},
    {0x7F, 0x02, 0x1C, 0x02, 0x7F}, {0x7F, 0x04, 0x08, 0x10, 0x7F}, {0x3E, 0x41, 0x41, 0x41, 0x3E},
    {0x7F, 0x09, 0x09, 0x09, 0x06}, {0x3E, 0x41, 0x51, 0x21, 0x5E}, {0x7F, 0x09, 0x19, 0x29, 0x46},
    {0x26, 0x49, 0x49, 0x49, 0x32}, {0x03, 0x01, 0x7F, 0x01, 0x03}, {0x3F, 0x40, 0x40, 0x40, 0x3F},
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, {0x3F, 0x40, 0x38, 0x40, 0x3F}, {0x63, 0x14, 0x08, 0x14, 0x63},
    {0x03, 0x04, 0x78, 0x04, 0x03}, {0x61, 0x59, 0x49, 0x4D, 0x43},
};

const uint8_t kInitSequence[] = {
    0xAE,        // display off
    0xD5, 0x80,  // clock divide
    0xA8, 0x3F,  // multiplex 64
    0xD3, 0x00,  // display offset
    0x40,        // start line 0
    0x8D, 0x14,  // charge pump on (ignored by SH1106)
    0x20, 0x00,  // horizontal addressing (SSD1306)
    0xA1,        // segment remap
    0xC8,        // COM scan direction
    0xDA, 0x12,  // COM pins
    0x81, 0xCF,  // contrast
    0xD9, 0xF1,  // pre-charge
    0xDB, 0x40,  // VCOM detect
    0xA4,        // resume from RAM
    0xA6,        // normal (not inverted)
    0x2E,        // no scrolling
    0xAF,        // display on
};

}  // namespace

bool Ssd1306::begin(TwoWire& wire, uint8_t address, Controller controller) {
  wire_ = &wire;
  address_ = address;
  controller_ = controller;
  wire_->beginTransmission(address_);
  present_ = (wire_->endTransmission() == 0);
  if (!present_) return false;
  commands(kInitSequence, sizeof(kInitSequence));
  clear();
  flush();
  return true;
}

void Ssd1306::command(uint8_t c) {
  wire_->beginTransmission(address_);
  wire_->write(0x00);
  wire_->write(c);
  wire_->endTransmission();
}

void Ssd1306::commands(const uint8_t* list, size_t n) {
  for (size_t i = 0; i < n; ++i) command(list[i]);
}

void Ssd1306::setContrast(uint8_t value) {
  if (!present_) return;
  command(0x81);
  command(value);
}

void Ssd1306::setPower(bool on) {
  if (!present_) return;
  command(on ? 0xAF : 0xAE);
}

void Ssd1306::flush() {
  if (!present_) return;
  constexpr int kChunk = 32;  // stays well below the 128 byte Wire buffer
  if (controller_ == Controller::kSsd1306) {
    const uint8_t window[] = {0x21, 0, kWidth - 1, 0x22, 0, kHeight / 8 - 1};
    commands(window, sizeof(window));
    for (size_t i = 0; i < sizeof(buffer_); i += kChunk) {
      wire_->beginTransmission(address_);
      wire_->write(0x40);
      wire_->write(buffer_ + i, kChunk);
      wire_->endTransmission();
    }
    return;
  }
  // SH1106: 132 column RAM, page addressing only, 2 column offset.
  for (int page = 0; page < kHeight / 8; ++page) {
    command(0xB0 + page);
    command(0x02);
    command(0x10);
    const uint8_t* row = buffer_ + page * kWidth;
    for (int i = 0; i < kWidth; i += kChunk) {
      wire_->beginTransmission(address_);
      wire_->write(0x40);
      wire_->write(row + i, kChunk);
      wire_->endTransmission();
    }
  }
}

void Ssd1306::setPixel(int x, int y, bool on) {
  if (x < 0 || y < 0 || x >= kWidth || y >= kHeight) return;
  uint8_t& byte = buffer_[(y >> 3) * kWidth + x];
  const uint8_t bit = 1u << (y & 7);
  if (on) {
    byte |= bit;
  } else {
    byte &= ~bit;
  }
}

void Ssd1306::hline(int x0, int x1, int y, bool on) {
  if (y < 0 || y >= kHeight) return;
  if (x0 > x1) std::swap(x0, x1);
  x0 = std::max(x0, 0);
  x1 = std::min(x1, kWidth - 1);
  for (int x = x0; x <= x1; ++x) setPixel(x, y, on);
}

void Ssd1306::fillRect(int x, int y, int w, int h, bool on) {
  for (int yy = y; yy < y + h; ++yy) hline(x, x + w - 1, yy, on);
}

void Ssd1306::fillRoundRect(int x, int y, int w, int h, int radius, bool on) {
  if (w <= 0 || h <= 0) return;
  radius = std::min(radius, std::min(w, h) / 2);
  for (int row = 0; row < h; ++row) {
    int inset = 0;
    const int fromEdge = std::min(row, h - 1 - row);
    if (fromEdge < radius) {
      const float dy = radius - fromEdge - 0.5f;
      inset = radius - static_cast<int>(sqrtf(std::max(0.0f, radius * radius - dy * dy)) + 0.5f);
    }
    hline(x + inset, x + w - 1 - inset, y + row, on);
  }
}

void Ssd1306::fillCircle(int cx, int cy, int r, bool on) { fillEllipse(cx, cy, r, r, on); }

void Ssd1306::fillEllipse(int cx, int cy, int rx, int ry, bool on) {
  if (rx <= 0 || ry <= 0) return;
  for (int dy = -ry; dy <= ry; ++dy) {
    // Sample at the pixel centre so the extreme rows are not 1 px spikes.
    const float fy = std::max(0.0f, fabsf(static_cast<float>(dy)) - 0.5f);
    const float t = 1.0f - (fy * fy) / static_cast<float>(ry * ry);
    const int half = static_cast<int>(rx * sqrtf(std::max(0.0f, t)) + 0.5f);
    hline(cx - half, cx + half, cy + dy, on);
  }
}

void Ssd1306::fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, bool on) {
  // Scanline fill: sort by y, then interpolate the long edge vs the two short ones.
  if (y0 > y1) { std::swap(x0, x1); std::swap(y0, y1); }
  if (y1 > y2) { std::swap(x1, x2); std::swap(y1, y2); }
  if (y0 > y1) { std::swap(x0, x1); std::swap(y0, y1); }
  if (y0 == y2) {
    hline(std::min({x0, x1, x2}), std::max({x0, x1, x2}), y0, on);
    return;
  }
  for (int y = y0; y <= y2; ++y) {
    const float a = x0 + (x2 - x0) * static_cast<float>(y - y0) / (y2 - y0);
    float b;
    if (y < y1) {
      b = (y1 == y0) ? x1 : x0 + (x1 - x0) * static_cast<float>(y - y0) / (y1 - y0);
    } else {
      b = (y2 == y1) ? x1 : x1 + (x2 - x1) * static_cast<float>(y - y1) / (y2 - y1);
    }
    hline(static_cast<int>(lroundf(a)), static_cast<int>(lroundf(b)), y, on);
  }
}

void Ssd1306::drawText(int x, int y, const char* text, int scale, bool on) {
  for (const char* p = text; *p; ++p) {
    char c = *p;
    if (c >= 'a' && c <= 'z') c -= 32;
    if (c < 0x20 || c > 0x5A) c = '?';
    const uint8_t* glyph = kFont5x7[c - 0x20];
    for (int col = 0; col < 5; ++col) {
      const uint8_t bits = pgm_read_byte(&glyph[col]);
      for (int row = 0; row < 8; ++row) {
        if (bits & (1u << row)) fillRect(x + col * scale, y + row * scale, scale, scale, on);
      }
    }
    x += 6 * scale;
  }
}

int Ssd1306::textWidth(const char* text, int scale) {
  return static_cast<int>(strlen(text)) * 6 * scale - scale;
}

}  // namespace drivers
