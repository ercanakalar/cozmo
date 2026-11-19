#pragma once
// Background tasks moving audio between the robot and the PC server:
//   mic task:     I2S microphone -> UDP packets -> server:5005
//   speaker task: UDP packets from server (port 5007) -> I2S speaker queue
#include <Arduino.h>
#include <WiFiUdp.h>

#include "drivers/I2sMicrophone.h"
#include "drivers/I2sSpeaker.h"

namespace net {

class AudioStreamer {
 public:
  struct Config {
    uint16_t micPort;      // server port receiving mic audio
    uint16_t speakerPort;  // local port receiving TTS audio
    int frameSamples;
    bool gateEnabled;
    int gateRms;
    uint32_t gateHangMs;
  };

  // Returns current server IP (0.0.0.0 = unknown); polled by the mic task.
  using ServerIpFn = IPAddress (*)();
  // Extra flag bits (motors moving etc.) added to each mic packet.
  using FlagsFn = uint16_t (*)();

  void begin(drivers::I2sMicrophone* mic, drivers::I2sSpeaker* speaker, const Config& config,
             ServerIpFn serverIp, FlagsFn flags);

  void setMicEnabled(bool on) { micEnabled_ = on; }
  int micRms() const { return micRms_; }
  uint32_t packetsSent() const { return sent_; }
  uint32_t packetsReceived() const { return received_; }

 private:
  static void micTaskEntry(void* arg);
  static void speakerTaskEntry(void* arg);
  void micTask();
  void speakerTask();

  drivers::I2sMicrophone* mic_ = nullptr;
  drivers::I2sSpeaker* speaker_ = nullptr;
  Config config_ = {};
  ServerIpFn serverIp_ = nullptr;
  FlagsFn flags_ = nullptr;
  volatile bool micEnabled_ = true;
  volatile int micRms_ = 0;
  volatile uint32_t sent_ = 0;
  volatile uint32_t received_ = 0;
};

}  // namespace net
