#pragma once
// WiFi + server discovery + TCP control channel (newline-delimited JSON).
//
//  * The PC server broadcasts "COZMO_SERVER <tcp_port>" on UDP 5006 once a
//    second. The robot learns the server IP from the sender address.
//  * The robot opens a TCP connection to the server and keeps it alive.
//    Commands arrive as one JSON object per line; telemetry/events go back
//    the same way.
#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiUdp.h>

namespace net {

class Link {
 public:
  using MessageHandler = void (*)(JsonDocument& msg);

  void begin(const char* ssid, const char* pass, const char* fixedServerIp, uint16_t discoveryPort,
             uint16_t controlPort, MessageHandler handler);
  void update(uint32_t nowMs);

  bool wifiUp() const { return WiFi.status() == WL_CONNECTED; }
  bool connected() { return client_.connected(); }
  bool hasServer() const { return serverIp_ != IPAddress(0, 0, 0, 0); }
  IPAddress serverIp() const { return serverIp_; }
  uint32_t lastRxMs() const { return lastRxMs_; }

  bool send(const JsonDocument& msg);

 private:
  void pollDiscovery();
  void connectIfNeeded(uint32_t now);
  void readLines();

  const char* ssid_ = nullptr;
  const char* pass_ = nullptr;
  uint16_t controlPort_ = 0;
  MessageHandler handler_ = nullptr;

  WiFiUDP beacon_;
  WiFiClient client_;
  IPAddress serverIp_ = IPAddress(0, 0, 0, 0);
  bool fixedServer_ = false;

  uint32_t lastWifiAttemptMs_ = 0;
  uint32_t lastConnectAttemptMs_ = 0;
  uint32_t lastRxMs_ = 0;
  bool wasConnected_ = false;

  static constexpr size_t kLineMax = 2048;
  char line_[kLineMax];
  size_t lineLen_ = 0;
};

}  // namespace net
