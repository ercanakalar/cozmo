#pragma once
// Minimal Arduino stand-ins so firmware face code compiles on a PC.
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <strings.h>
#define PROGMEM
#define PI 3.14159265358979f
inline uint8_t pgm_read_byte(const uint8_t* p) { return *p; }
extern uint32_t g_millis;
inline uint32_t millis() { return g_millis; }
inline uint32_t esp_random() { return 12345; }
template <class T, class L, class H> T constrain(T v, L lo, H hi) { return v < lo ? lo : (v > hi ? hi : v); }
// Some libcs already declare strlcpy, others don't: always use our own.
inline size_t cozmo_strlcpy(char* d, const char* s, size_t n) { size_t l = strlen(s); if (n) { size_t c = l >= n ? n - 1 : l; memcpy(d, s, c); d[c] = 0; } return l; }
#define strlcpy cozmo_strlcpy
using std::min; using std::max;
