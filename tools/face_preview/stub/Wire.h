#pragma once
#include <cstdint>
#include <cstddef>
class TwoWire { public:
  void beginTransmission(uint8_t) {}
  uint8_t endTransmission(bool = true) { return 0; }
  size_t write(uint8_t) { return 1; }
  size_t write(const uint8_t*, size_t n) { return n; }
};
