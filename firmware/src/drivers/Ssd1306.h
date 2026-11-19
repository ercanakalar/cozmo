#pragma once
// Minimal SSD1306 / SH1106 128x64 I2C OLED driver with a 1-bit framebuffer.
// No external library: init sequence, framebuffer flush and a few filled
// primitives that the procedural eye renderer needs.
#include <Arduino.h>
#include <Wire.h>

namespace drivers {

class Ssd1306 {
 public:
  static constexpr int kWidth = 128;
  static constexpr int kHeight = 64;

  enum class Controller { kSsd1306, kSh1106 };

  bool begin(TwoWire& wire, uint8_t address, Controller controller = Controller::kSsd1306);
  bool present() const { return present_; }

  void clear() { memset(buffer_, 0, sizeof(buffer_)); }
  // Sends the framebuffer to the panel (~23 ms at 400 kHz I2C).
  void flush();

  void setContrast(uint8_t value);
  void setPower(bool on);

  // Drawing primitives. `on` = lit pixel. Everything is clipped.
  void setPixel(int x, int y, bool on);
  void hline(int x0, int x1, int y, bool on);
  void fillRect(int x, int y, int w, int h, bool on);
  void fillRoundRect(int x, int y, int w, int h, int radius, bool on);
  void fillCircle(int cx, int cy, int r, bool on);
  void fillEllipse(int cx, int cy, int rx, int ry, bool on);
  void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, bool on);
  // 5x7 font, upper-case ASCII 32..90 (lower case is mapped to upper case).
  void drawText(int x, int y, const char* text, int scale = 1, bool on = true);
  static int textWidth(const char* text, int scale = 1);

 private:
  void command(uint8_t c);
  void commands(const uint8_t* list, size_t n);

  TwoWire* wire_ = nullptr;
  uint8_t address_ = 0x3C;
  Controller controller_ = Controller::kSsd1306;
  bool present_ = false;
  uint8_t buffer_[kWidth * kHeight / 8];
};

}  // namespace drivers
