// sky.h — the Arduino side of the sky layer: fetches, clock, staleness.
// Pure math lives in sky_core.h. This file talks to the network.
//
// Usage: after WiFi is up and BEFORE display.begin(): sky.bootSync().
// Then sky.tick() every frame. sky.vector() is always safe to read: with
// no data it is 0.5 everywhere (neutral), so the light stays as it is.
//
// Network calls are plain HTTP/1.0 with Connection: close (no TLS, no
// chunking). The only blocking call in tick() is the TCP connect, a few
// hundred ms once per refresh. The body is read a little per frame.
#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include "sky_core.h"

class SkyClient {
public:
  // Blocking boot sequence, capped: location (ip-api), NTP, first weather.
  // Returns true if at least the clock is set.
  bool bootSync(uint32_t capMs = 10000);

  // Per-frame service: hourly refresh, WiFi reconnect, NTP background.
  void tick();

  // The model, already decayed for staleness. Never blocks.
  sky::Vector vector() const;

  bool haveTime() const;
  bool haveLocation() const { return loc_.valid; }
  bool haveWeather() const { return wx_.valid; }
  const sky::Location& location() const { return loc_; }
  const sky::Weather&  weather()  const { return wx_; }
  sky::SunPos sun() const;                       // from clock + location
  float weatherAgeHours() const;
  time_t localNow() const;                       // epoch + utc offset (0 offset if unknown)
  void logStatus(Print& out) const;
  uint32_t lastConnectMs() const { return lastConnectMs_; }

  // WiFi reconnect needs the credentials; give them once at boot.
  void setCredentials(const char* ssid, const char* pass);

  // Test hooks (spike only): refresh interval, override host/path.
  void setRefreshMs(uint32_t ms) { refreshMs_ = ms; if (wx_.valid) nextTryMs_ = wxFetchedMs_ + ms; }
  void setWeatherHost(const char* h) { wxHost_ = h; }
  void setWeatherPathPrefix(const char* p) { wxPathPrefix_ = p; }
  void requestRefresh() { due_ = true; }

  // Call FIRST in setup(), before any WiFi use (portal included). The CYW43
  // SPI runs off a PIO divisor fixed at bus init; PicoDVI later raises sysclk
  // to 252 MHz, which would push that SPI to 63 MHz (chip max 50). Divisor 3
  // keeps it at 42 MHz with DVI live, 25 MHz at boot. Proven 2026-09-14.
  static void prepareRadioForDvi();

private:
  enum class Job : uint8_t { None, Location, Weather };
  enum class St  : uint8_t { Idle, Reading, Done, Failed };

  bool startJob(Job j);                          // DNS + connect + send (blocking part)
  void serviceJob();                             // read a slice, parse when complete
  void finishJob(bool ok);
  void reassociate(const char* why);             // disconnect + non-blocking begin
  bool runBlocking(Job j, uint32_t capMs);       // boot helper: start + service until done
  void buildWeatherPath(char* out, size_t cap) const;

  sky::Location loc_;
  sky::Weather  wx_;
  uint32_t wxFetchedMs_ = 0;                     // millis() of last good weather
  uint32_t nextTryMs_   = 0;
  uint32_t refreshMs_   = 3600000;               // 1 h
  uint32_t retryMs_     = 300000;                // 5 min after a failure
  bool     due_         = false;
  uint8_t  consecFail_  = 0;

  // in-flight job
  WiFiClient client_;
  Job  job_ = Job::None;
  St   st_  = St::Idle;
  uint32_t jobStartMs_ = 0;
  uint32_t lastConnectMs_ = 0;                   // how long the last connect() blocked
  static constexpr size_t BUF = 2048;
  char   buf_[BUF];
  size_t len_ = 0;

  // wifi
  char ssid_[33] = {0}, pass_[65] = {0};
  uint32_t downSinceMs_ = 0;
  // Keepalive: with DVI live the STA link goes deaf after minutes of idle
  // (RF noise from the TMDS pairs next to the antenna, RSSI ~ -70 dBm here).
  // One UDP byte to the gateway every 15 s keeps it usable. Proven 2026-09-14.
  WiFiUDP  keep_;
  uint32_t lastKeepMs_ = 0;
  static constexpr uint32_t KEEPALIVE_MS = 15000;

  const char* wxHost_ = "api.open-meteo.com";
  const char* wxPathPrefix_ = "/v1/forecast";
  const char* ipHost_ = "ip-api.com";
  const char* ipPath_ = "/json/?fields=status,message,lat,lon,city,timezone,offset";
};
