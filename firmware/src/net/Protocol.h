#pragma once
// Wire protocol shared with the PC server (server/cozmo_server/protocol.py).
// Full description: docs/PROTOCOL.md
#include <cstdint>

namespace net {

// UDP audio packet header, little endian, followed by `samples` int16 PCM
// mono samples at 16 kHz.
#pragma pack(push, 1)
struct AudioHeader {
  char magic[4];     // "CZMU" robot->server mic, "CZSP" server->robot speaker
  uint32_t seq;      // increments per packet
  uint16_t samples;  // number of int16 samples following the header
  uint16_t flags;    // see AudioFlags
};
#pragma pack(pop)
static_assert(sizeof(AudioHeader) == 12, "header must be 12 bytes");

enum AudioFlags : uint16_t {
  kFlagSpeakerActive = 1 << 0,  // mic: robot speaker was playing (echo likely)
  kFlagMotorsActive = 1 << 1,   // mic: treads/servos moving (noise likely)
  kFlagEndOfUtterance = 1 << 0, // speaker: last packet of a TTS clip
};

constexpr char kMicMagic[4] = {'C', 'Z', 'M', 'U'};
constexpr char kSpeakerMagic[4] = {'C', 'Z', 'S', 'P'};
constexpr char kBeaconPrefix[] = "COZMO_SERVER";
constexpr uint16_t kMaxSpeakerSamples = 700;  // keeps packets under one MTU

constexpr const char* kFirmwareVersion = "0.1.0";

}  // namespace net
