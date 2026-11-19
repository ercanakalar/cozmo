#include "Link.h"

#include "Protocol.h"

namespace net {

void Link::begin(const char* ssid, const char* pass, const char* fixedServerIp,
                 uint16_t discoveryPort, uint16_t controlPort, MessageHandler handler) {
  ssid_ = ssid;
  pass_ = pass;
  controlPort_ = controlPort;
  handler_ = handler;

  if (fixedServerIp != nullptr && fixedServerIp[0] != '\0') {
    fixedServer_ = serverIp_.fromString(fixedServerIp);
  }

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);  // modem sleep adds 100+ ms of latency to audio
  WiFi.setHostname("cozmo");
  WiFi.begin(ssid_, pass_);
  lastWifiAttemptMs_ = millis();
  beacon_.begin(discoveryPort);
}

void Link::update(uint32_t now) {
  if (!wifiUp()) {
    if (now - lastWifiAttemptMs_ > 10000) {
      log_i("WiFi reconnecting...");
      WiFi.disconnect();
      WiFi.begin(ssid_, pass_);
      lastWifiAttemptMs_ = now;
    }
    return;
  }
  pollDiscovery();
  connectIfNeeded(now);
  readLines();
}

void Link::pollDiscovery() {
  int size = beacon_.parsePacket();
  while (size > 0) {
    char buf[64] = {};
    beacon_.read(buf, sizeof(buf) - 1);
    if (!fixedServer_ && strncmp(buf, kBeaconPrefix, strlen(kBeaconPrefix)) == 0) {
      const IPAddress ip = beacon_.remoteIP();
      const int port = atoi(buf + strlen(kBeaconPrefix));
      if (port > 0) controlPort_ = static_cast<uint16_t>(port);
      if (ip != serverIp_) {
        serverIp_ = ip;
        log_i("Discovered server at %s:%u", ip.toString().c_str(), controlPort_);
      }
    }
    size = beacon_.parsePacket();
  }
}

void Link::connectIfNeeded(uint32_t now) {
  const bool up = client_.connected();
  if (wasConnected_ && !up) log_w("Server connection lost");
  wasConnected_ = up;
  if (up || !hasServer() || now - lastConnectAttemptMs_ < 2000) return;
  lastConnectAttemptMs_ = now;
  client_.stop();
  if (!client_.connect(serverIp_, controlPort_, 1000)) return;
  client_.setNoDelay(true);
  lineLen_ = 0;
  lastRxMs_ = now;
  wasConnected_ = true;
  log_i("Connected to server");

  JsonDocument hello;
  hello["type"] = "hello";
  hello["name"] = WiFi.getHostname();
  hello["fw"] = kFirmwareVersion;
  hello["mac"] = WiFi.macAddress();
  send(hello);
}

void Link::readLines() {
  while (client_.connected() && client_.available() > 0) {
    const int c = client_.read();
    if (c < 0) break;
    if (c == '\n') {
      line_[lineLen_] = '\0';
      lastRxMs_ = millis();
      if (lineLen_ > 0 && handler_ != nullptr) {
        JsonDocument doc;
        const DeserializationError err = deserializeJson(doc, line_, lineLen_);
        if (err) {
          log_w("Bad JSON from server: %s", err.c_str());
        } else {
          handler_(doc);
        }
      }
      lineLen_ = 0;
    } else if (lineLen_ < kLineMax - 1) {
      line_[lineLen_++] = static_cast<char>(c);
    } else {
      lineLen_ = 0;  // overlong line: drop it
    }
  }
}

bool Link::send(const JsonDocument& msg) {
  if (!client_.connected()) return false;
  char buf[512];
  const size_t n = serializeJson(msg, buf, sizeof(buf) - 1);
  if (n == 0 || n >= sizeof(buf) - 1) return false;
  buf[n] = '\n';
  return client_.write(reinterpret_cast<const uint8_t*>(buf), n + 1) == n + 1;
}

}  // namespace net
