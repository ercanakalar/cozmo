// Host-side preview of the robot's eyes: compiles the real firmware sources
// (face/Face.cpp, drivers/Ssd1306.cpp) against tiny Arduino/FreeRTOS stubs
// and dumps the 128x64 framebuffer of every expression to faces.txt.
// Build + render: tools/face_preview/run.sh
#include <cstdio>
#define private public  // read the OLED framebuffer directly
#include "drivers/Ssd1306.h"
#undef private
#include "face/Face.h"

uint32_t g_millis = 0;

static void settle(face::Face& f, int frames = 40) {
  for (int i = 0; i < frames; ++i) {
    g_millis += 40;
    f.render(g_millis);
  }
}

static void dump(FILE* out, const char* name, const drivers::Ssd1306& oled) {
  fprintf(out, "%s\n", name);
  for (int y = 0; y < 64; ++y) {
    for (int x = 0; x < 128; ++x) fputc(oled.buffer_[(y >> 3) * 128 + x] & (1 << (y & 7)) ? '1' : '0', out);
    fputc('\n', out);
  }
}

int main() {
  drivers::Ssd1306 oled;
  TwoWire wire;
  oled.begin(wire, 0x3C);
  face::Face f;
  f.begin(&oled);
  f.setAutoBlink(false);
  FILE* out = fopen("faces.txt", "w");
  for (int e = 0; e < static_cast<int>(face::Expression::kCount); ++e) {
    f.setExpression(static_cast<face::Expression>(e));
    settle(f);
    dump(out, face::expressionName(static_cast<face::Expression>(e)), oled);
  }
  f.setExpression(face::Expression::kNeutral);
  f.setLook(-1, -0.5f);
  settle(f);
  dump(out, "look_left_up", oled);
  f.setLook(0, 0);
  f.setSleeping(true);
  settle(f);
  dump(out, "sleeping", oled);
  fclose(out);
  return 0;
}
