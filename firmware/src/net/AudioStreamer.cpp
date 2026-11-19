#include "AudioStreamer.h"

#include <WiFi.h>

#include "Protocol.h"

namespace net {

void AudioStreamer::begin(drivers::I2sMicrophone* mic, drivers::I2sSpeaker* speaker,
                          const Config& config, ServerIpFn serverIp, FlagsFn flags) {
  mic_ = mic;
  speaker_ = speaker;
  config_ = config;
  serverIp_ = serverIp;
  flags_ = flags;
  xTaskCreatePinnedToCore(&AudioStreamer::micTaskEntry, "mic", 6144, this, 4, nullptr, 0);
  xTaskCreatePinnedToCore(&AudioStreamer::speakerTaskEntry, "spk_rx", 6144, this, 4, nullptr, 0);
}

void AudioStreamer::micTaskEntry(void* arg) { static_cast<AudioStreamer*>(arg)->micTask(); }
void AudioStreamer::speakerTaskEntry(void* arg) { static_cast<AudioStreamer*>(arg)->speakerTask(); }

void AudioStreamer::micTask() {
  WiFiUDP udp;
  udp.begin(0);
  const size_t n = static_cast<size_t>(config_.frameSamples);
  uint8_t* packet = static_cast<uint8_t*>(malloc(sizeof(AudioHeader) + n * sizeof(int16_t)));
  auto* header = reinterpret_cast<AudioHeader*>(packet);
  auto* pcm = reinterpret_cast<int16_t*>(packet + sizeof(AudioHeader));
  memcpy(header->magic, kMicMagic, 4);
  uint32_t seq = 0;
  uint32_t gateOpenUntil = 0;

  for (;;) {
    const size_t got = mic_->read(pcm, n, pdMS_TO_TICKS(100));
    if (got != n) continue;  // mic not ready yet
    micRms_ = mic_->lastRms();

    if (!micEnabled_ || WiFi.status() != WL_CONNECTED) continue;
    const IPAddress server = serverIp_ ? serverIp_() : IPAddress(0, 0, 0, 0);
    if (server == IPAddress(0, 0, 0, 0)) continue;

    if (config_.gateEnabled) {
      const uint32_t now = millis();
      if (micRms_ > config_.gateRms) gateOpenUntil = now + config_.gateHangMs;
      if (static_cast<int32_t>(gateOpenUntil - now) < 0) continue;
    }

    header->seq = seq++;
    header->samples = static_cast<uint16_t>(n);
    uint16_t flags = flags_ ? flags_() : 0;
    if (speaker_ && speaker_->active()) flags |= kFlagSpeakerActive;
    header->flags = flags;

    udp.beginPacket(server, config_.micPort);
    udp.write(packet, sizeof(AudioHeader) + n * sizeof(int16_t));
    if (udp.endPacket()) sent_ = sent_ + 1;
  }
}

void AudioStreamer::speakerTask() {
  WiFiUDP udp;
  bool bound = false;
  constexpr size_t kMax = sizeof(AudioHeader) + kMaxSpeakerSamples * sizeof(int16_t);
  uint8_t* packet = static_cast<uint8_t*>(malloc(kMax));

  for (;;) {
    if (WiFi.status() != WL_CONNECTED) {
      bound = false;
      vTaskDelay(pdMS_TO_TICKS(200));
      continue;
    }
    if (!bound) {
      udp.stop();
      bound = udp.begin(config_.speakerPort);
    }
    const int size = udp.parsePacket();
    if (size <= 0) {
      vTaskDelay(pdMS_TO_TICKS(2));
      continue;
    }
    const int len = udp.read(packet, kMax);
    if (len < static_cast<int>(sizeof(AudioHeader))) continue;
    const auto* header = reinterpret_cast<const AudioHeader*>(packet);
    if (memcmp(header->magic, kSpeakerMagic, 4) != 0) continue;
    const size_t samples = std::min<size_t>(header->samples, (len - sizeof(AudioHeader)) / 2);
    // Block a little if the queue is full: the server paces at real time,
    // so this only absorbs WiFi jitter.
    speaker_->write(reinterpret_cast<const int16_t*>(packet + sizeof(AudioHeader)), samples,
                    pdMS_TO_TICKS(50));
    received_ = received_ + 1;
  }
}

}  // namespace net
