// wifi_portal.cpp — see wifi_portal.h for the one rule that shapes this file.
#include "wifi_portal.h"
#include "config.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <EEPROM.h>
#include "hardware/watchdog.h"

// SDK uses watchdog scratch[4..7] for its own reboot magic; [0] is ours.
static constexpr uint32_t PORTAL_FLAG  = 0x504F5254; // "PORT"
static constexpr uint32_t CREDS_MAGIC  = 0x4B4F4D31; // "KOM1"
static constexpr int      EEPROM_BYTES = 256;

uint32_t WifiPortal::checksum(const Creds& c) {
  uint32_t h = 2166136261u;
  const uint8_t* p = (const uint8_t*)&c;
  for (size_t i = 0; i < offsetof(Creds, check); i++) h = (h ^ p[i]) * 16777619u;
  return h;
}

bool WifiPortal::load(Creds& c) {
  EEPROM.get(0, c);
  if (c.magic != CREDS_MAGIC || c.check != checksum(c)) return false;
  c.ssid[32] = 0; c.pass[64] = 0;
  return c.ssid[0] != 0;
}

void WifiPortal::save(const Creds& c) {
  EEPROM.put(0, c);
  EEPROM.commit();
}

bool WifiPortal::tryConnect(const Creds& c) {
  WiFi.mode(WIFI_STA);
  WiFi.setTimeout(WIFI_CONNECT_TIMEOUT_MS / 2); // begin() blocks up to 2x this
  WiFi.begin(c.ssid, c.pass[0] ? c.pass : nullptr);
  uint32_t t0 = millis();
  while (!WiFi.connected() && millis() - t0 < WIFI_CONNECT_TIMEOUT_MS) delay(50);
  return WiFi.connected();
}

// ---------- portal ----------
static WebServer server(80);
static DNSServer dns;
static String    netList;      // <option>s from the scan
static bool      saved = false;
static WifiPortal::Creds* pending = nullptr;

static const char PAGE_HEAD[] =
  "<!doctype html><html><head><meta name=viewport content='width=device-width,initial-scale=1'>"
  "<title>komorebi</title><style>body{font-family:sans-serif;max-width:22em;margin:2em auto;padding:0 1em}"
  "select,input,button{width:100%;font-size:1em;padding:.5em;margin:.3em 0 1em;box-sizing:border-box}"
  "button{background:#333;color:#fff;border:0}</style></head><body>";

static void handleRoot() {
  String h = PAGE_HEAD;
  h += "<h2>komorebi</h2><p>Choose the WiFi network this piece should join.</p>"
       "<form method=POST action=/save>"
       "<label>Network</label><select name=ssid>" + netList + "</select>"
       "<label>Password</label><input type=password name=pass autocomplete=off>"
       "<button type=submit>Save and restart</button></form>"
       "<p style='color:#888'>Nothing chosen within " + String(WIFI_PORTAL_TIMEOUT_MS / 60000) +
       " minutes: the light starts without WiFi.</p></body></html>";
  server.send(200, "text/html", h);
}

static void handleSave() {
  String ssid = server.arg("ssid"), pass = server.arg("pass");
  if (ssid.length() == 0 || ssid.length() > 32 || pass.length() > 63 ||
      (pass.length() > 0 && pass.length() < 8)) {
    server.send(400, "text/html", String(PAGE_HEAD) +
      "<p>Password must be empty (open network) or 8 to 63 characters.</p><a href=/>Back</a></body></html>");
    return;
  }
  memset(pending, 0, sizeof(*pending));
  pending->magic = CREDS_MAGIC;
  strncpy(pending->ssid, ssid.c_str(), 32);
  strncpy(pending->pass, pass.c_str(), 64);
  pending->check = WifiPortal::checksum(*pending);
  server.send(200, "text/html", String(PAGE_HEAD) +
    "<h2>Saved</h2><p>Restarting and joining <b>" + ssid + "</b>. You can close this page.</p></body></html>");
  saved = true;
}

static void handleNotFound() { // captive-portal redirect (iOS / Android probes land here)
  server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/", true);
  server.send(302, "text/plain", "");
}

bool WifiPortal::runPortal() {
  // Scan while still in STA mode, build the option list sorted by signal.
  WiFi.mode(WIFI_STA);
  int n = WiFi.scanNetworks();
  int idx[32]; int m = 0;
  for (int i = 0; i < n && m < 32; i++) if (WiFi.SSID(i) && WiFi.SSID(i)[0]) idx[m++] = i;
  for (int a = 0; a < m; a++) for (int b = a + 1; b < m; b++)
    if (WiFi.RSSI(idx[b]) > WiFi.RSSI(idx[a])) { int t = idx[a]; idx[a] = idx[b]; idx[b] = t; }
  netList = "";
  for (int k = 0; k < m; k++) {
    String s = WiFi.SSID(idx[k]);
    if (netList.indexOf("value='" + s + "'") >= 0) continue; // dedupe multi-AP SSIDs
    netList += "<option value='" + s + "'>" + s + " (" + String(WiFi.RSSI(idx[k])) + " dBm)</option>";
  }
  if (netList.length() == 0) netList = "<option value=''>(no networks found)</option>";

  Creds c;
  pending = &c;
  saved = false;
  WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS);
  dns.start(53, "*", WiFi.softAPIP());
  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.onNotFound(handleNotFound);
  server.begin();

  uint32_t t0 = millis();
  while (!saved && millis() - t0 < WIFI_PORTAL_TIMEOUT_MS) {
    dns.processNextRequest();
    server.handleClient();
    delay(2);
  }
  server.stop();
  dns.stop();
  if (saved) {
    delay(500);                 // let the "Saved" page finish sending
    WiFi.softAPdisconnect(true);
    save(c);                    // DVI is not running: safe
    delay(100);
    rp2040.reboot();
  }
  WiFi.softAPdisconnect(true);  // timeout: go dark and let the art start
  return false;
}

// ---------- entry points ----------
bool WifiPortal::boot() {
  EEPROM.begin(EEPROM_BYTES);
  bool forcePortal = (watchdog_hw->scratch[0] == PORTAL_FLAG);
  watchdog_hw->scratch[0] = 0;

  Creds c;
  bool have = load(c);
  if (forcePortal && have) {           // long-press: forget stored network
    Creds blank; memset(&blank, 0, sizeof(blank));
    save(blank);                       // pre-DVI: safe
    have = false;
  }
  if (have && tryConnect(c)) { connected_ = true; return true; }
  runPortal();                         // reboots on save, returns on timeout
  connected_ = false;
  return false;
}

void WifiPortal::requestPortalAndReboot() {
  watchdog_hw->scratch[0] = PORTAL_FLAG;
  rp2040.reboot();
  for (;;) {}
}
