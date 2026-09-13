// SPIKE A: does CYW43 WiFi (PIO SPI, fixed clock div 2) work while PicoDVI
// is live, i.e. sysclk forced to 252 MHz, core1 gone, pio0 taken?
// Test the HARDER order: DVI first, then WiFi.
//
// Serial 115200 reports: sysclk, scan results, AP status, page hits.
// Screen: moving ball (DVI alive), one square per network found (top row,
// max 16), a bar at the bottom if the AP is up, a tick per HTTP hit.
// Phone: join "komorebi-spike" (pwd komorebi), open http://192.168.4.1/
#include <PicoDVI.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include "hardware/clocks.h"

DVIGFX8 display(DVI_RES_320x240p60, true, pico_sock_cfg);
WebServer server(80);
DNSServer dns;

static int      nets  = -1;
static bool     apUp  = false;
static uint32_t hits  = 0;
static uint32_t lastReport = 0;
static String   scanHtml;

static void handleRoot() {
  hits++;
  String h = "<html><body style='font-family:sans-serif'><h2>komorebi spike A</h2>";
  h += "<p>sysclk " + String(clock_get_hz(clk_sys) / 1000000) + " MHz, uptime ";
  h += String(millis() / 1000) + " s, hits " + String(hits) + "</p><ol>";
  h += scanHtml + "</ol></body></html>";
  server.send(200, "text/html", h);
}

static void handleNotFound() { // captive-portal style redirect
  server.sendHeader("Location", "http://192.168.4.1/", true);
  server.send(302, "text/plain", "");
}

void setup() {
  Serial.begin(115200);
  if (!display.begin()) {
    pinMode(LED_BUILTIN, OUTPUT);
    for (;;) digitalWrite(LED_BUILTIN, (millis() / 500) & 1);
  }
  for (int i = 0; i < 256; i++) display.setColor(i, i, i, i);
  display.swap(false, true);

  delay(2000); // give the host serial a chance to attach
  Serial.printf("[A] DVI up, sysclk %lu Hz\n", clock_get_hz(clk_sys));

  Serial.println("[A] scanning...");
  uint32_t t0 = millis();
  nets = WiFi.scanNetworks();
  Serial.printf("[A] scan done in %lu ms, %d networks\n", millis() - t0, nets);
  for (int i = 0; i < nets; i++) {
    Serial.printf("    %2d  %-32s  %4ld dBm  enc %u\n", i, WiFi.SSID(i),
                  (long)WiFi.RSSI(i), WiFi.encryptionType(i));
    scanHtml += "<li>" + String(WiFi.SSID(i)) + " (" + String(WiFi.RSSI(i)) + " dBm)</li>";
  }

  apUp = WiFi.softAP("komorebi-spike", "komorebi");
  Serial.printf("[A] softAP %s, ip %s\n", apUp ? "UP" : "FAILED",
                WiFi.softAPIP().toString().c_str());
  dns.start(53, "*", WiFi.softAPIP());
  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("[A] server up. join komorebi-spike / komorebi, open http://192.168.4.1/");
}

void loop() {
  uint32_t now = millis();
  float t = now / 1000.0f;

  server.handleClient();
  dns.processNextRequest();

  display.fillScreen(0);
  int cx = 160 + (int)(100 * sinf(t * 1.3f));
  display.fillCircle(cx, 130, 30, 200);                 // DVI alive
  for (int i = 0; i < min(nets, 16); i++)
    display.fillRect(10 + i * 18, 10, 12, 12, 255);     // networks found
  if (apUp) display.fillRect(10, 220, 300, 8, 255);     // AP up
  for (uint32_t i = 0; i < min(hits, 20u); i++)
    display.fillRect(10 + i * 14, 200, 8, 8, 160);      // http hits
  display.swap();

  if (now - lastReport > 10000) {
    lastReport = now;
    Serial.printf("[A] t=%lus sysclk=%lu nets=%d ap=%d hits=%lu heap=%u sta=%d\n",
                  now / 1000, clock_get_hz(clk_sys), nets, apUp, hits,
                  rp2040.getFreeHeap(), WiFi.status());
  }
}
