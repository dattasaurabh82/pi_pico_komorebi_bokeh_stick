// SPIKE: sky layer on the Pico 2 W with DVI live (stage 2 of the sky plan).
// Uses main/sky.{h,cpp} + main/sky_core.{h,cpp} via symlinks in this folder,
// so the spike tests the real code, not a copy. WiFi credentials come from
// the EEPROM record the main sketch's portal saved (same layout).
//
// Screen: moving ball = DVI alive; top rows = model L/W/M/F as bar
// lengths; bottom bar = weather valid.
// Serial 115200: status every 60 s and after every fetch. Commands:
//   n  status now          w  fetch weather now
//   b  bad host (DNS fail) p  bad path (404)    g  good host+path again
// Refresh interval accelerated to 2 min. Frame-stall meter prints the
// longest gap between loop() iterations at each status line.
#include <PicoDVI.h>
#include <WiFi.h>
#include <EEPROM.h>
#include "sky_core.h"
#include "sky.h"

DVIGFX8 display(DVI_RES_320x240p60, true, pico_sock_cfg);
SkyClient skyc;

// same record as main/wifi_portal.cpp
struct Creds { uint32_t magic; char ssid[33]; char pass[65]; uint32_t check; };
static uint32_t credsChecksum(const Creds& c) {
  uint32_t h = 2166136261u; const uint8_t* p = (const uint8_t*)&c;
  for (size_t i = 0; i < offsetof(Creds, check); i++) h = (h ^ p[i]) * 16777619u;
  return h;
}

static uint32_t maxGapMs = 0, lastLoopMs = 0, lastStatusMs = 0;

// The CYW43 SPI runs off a PIO with a fixed divisor of 2, set at bus init
// (sysclk 150 MHz). PicoDVI then raises sysclk to 252 MHz, so the SPI runs
// 1.7x over spec: lossy link, slow/failed TCP connects. Halve it before
// the first WiFi call. Driver is prebuilt with CYW43_PIO_CLOCK_DIV_DYNAMIC.
extern "C" void cyw43_set_pio_clkdiv_int_frac8(uint32_t clock_div_int, uint8_t clock_div_frac8);

void setup() {
  cyw43_set_pio_clkdiv_int_frac8(3, 0);
  Serial.begin(115200);
  for (uint32_t t = millis(); !Serial && millis() - t < 3000;) delay(10);

  EEPROM.begin(256);
  Creds c; EEPROM.get(0, c);
  bool have = (c.magic == 0x4B4F4D31 && c.check == credsChecksum(c) && c.ssid[0]);
  Serial.printf("[spike] creds %s\n", have ? c.ssid : "NONE (portal never saved)");
  if (have) {
    WiFi.mode(WIFI_STA);
    WiFi.setTimeout(7500);
    WiFi.begin(c.ssid, c.pass[0] ? c.pass : nullptr);
    for (uint32_t t = millis(); !WiFi.connected() && millis() - t < 15000;) delay(50);
    Serial.printf("[spike] wifi %s ip %s\n", WiFi.connected() ? "up" : "DOWN", WiFi.localIP().toString().c_str());
    skyc.setCredentials(c.ssid, c.pass);
  }
  uint32_t t0 = millis();
  skyc.bootSync(10000);                       // pre-DVI, blocking, capped
  Serial.printf("[spike] bootSync took %lu ms\n", millis() - t0);
  skyc.setRefreshMs(120000);                  // 2 min for the test

  if (!display.begin()) { pinMode(LED_BUILTIN, OUTPUT); for (;;) digitalWrite(LED_BUILTIN, (millis() / 500) & 1); }
  for (int i = 0; i < 256; i++) display.setColor(i, i, i, i);
  display.swap(false, true);
  lastLoopMs = millis();
}

void loop() {
  uint32_t now = millis();
  uint32_t gap = now - lastLoopMs; lastLoopMs = now;
  if (gap > maxGapMs) maxGapMs = gap;

  skyc.tick();

  while (Serial.available()) {
    char ch = Serial.read();
    if (ch == 'n') { skyc.logStatus(Serial); Serial.printf("[spike] max loop gap %lu ms\n", maxGapMs); maxGapMs = 0; }
    if (ch == 'w') { Serial.println("[spike] forcing fetch"); skyc.requestRefresh(); }
    if (ch == 'b') { Serial.println("[spike] bad host"); skyc.setWeatherHost("nope.invalid"); skyc.requestRefresh(); }
    if (ch == 'p') { Serial.println("[spike] bad path"); skyc.setWeatherPathPrefix("/v1/nope"); skyc.requestRefresh(); }
    if (ch == 'g') { Serial.println("[spike] good again"); skyc.setWeatherHost("api.open-meteo.com"); skyc.setWeatherPathPrefix("/v1/forecast"); skyc.requestRefresh(); }
    if (ch == 'd') {                        // DNS only
      IPAddress ip; uint32_t t0 = millis();
      int r = WiFi.hostByName("api.open-meteo.com", ip);
      Serial.printf("[probe] dns r=%d %s in %lu ms\n", r, ip.toString().c_str(), millis() - t0);
    }
    if (ch == 'i') {                        // TCP connect by IP only (no DNS)
      WiFiClient c; uint32_t t0 = millis();
      int r = c.connect(IPAddress(8, 8, 8, 8), 53);   // any TCP listener; google dns speaks tcp/53
      Serial.printf("[probe] tcp 8.8.8.8:53 r=%d in %lu ms\n", r, millis() - t0);
      c.stop();
    }
    if (ch == 's') {                        // raw status
      Serial.printf("[probe] wifi status %d, local %s, gw %s, dns %s\n", WiFi.status(),
        WiFi.localIP().toString().c_str(), WiFi.gatewayIP().toString().c_str(), WiFi.dnsIP().toString().c_str());
    }
  }

  if (now - lastStatusMs > 60000) {
    lastStatusMs = now;
    skyc.logStatus(Serial);
    Serial.printf("[spike] max loop gap %lu ms, wifi %d\n", maxGapMs, WiFi.status());
    maxGapMs = 0;
  }

  float t = now / 1000.0f;
  sky::Vector v = skyc.vector();
  display.fillScreen(0);
  int cx = 160 + (int)(100 * sinf(t * 1.3f));
  display.fillCircle(cx, 140, 30, 200);
  float bars[4] = { v.light, v.warmth, v.motion, v.foliage };
  for (int i = 0; i < 4; i++) display.fillRect(10, 10 + i * 12, (int)(bars[i] * 300), 8, v.known ? 255 : 80);
  if (skyc.haveWeather()) display.fillRect(10, 224, 300, 6, 255);
  display.swap();
}
