// sky.cpp — see sky.h.
#include "sky.h"
#include <WiFi.h>
#include <time.h>

static constexpr uint32_t JOB_TIMEOUT_MS = 8000;
static constexpr float    MAX_FRESH_H    = 6.0f;

// ---------- clock ----------
bool SkyClient::haveTime() const { return time(nullptr) > 1600000000; } // after 2020
time_t SkyClient::localNow() const {
  int off = wx_.valid ? wx_.utc_offset_s : (loc_.valid ? loc_.utc_offset_s : 0);
  return time(nullptr) + off;
}
sky::SunPos SkyClient::sun() const {
  sky::SunPos p; p.elevation_deg = -90; p.azimuth_deg = 0;
  if (!haveTime() || !loc_.valid) return p;
  return sky::sunPosition(loc_.lat, loc_.lon, time(nullptr));
}
float SkyClient::weatherAgeHours() const {
  if (!wx_.valid) return 1e9f;
  return (millis() - wxFetchedMs_) / 3600000.0f;
}

// ---------- model ----------
sky::Vector SkyClient::vector() const {
  bool ht = haveTime() && loc_.valid;
  int doy = 172;
  if (ht) { time_t ln = localNow(); struct tm t; gmtime_r(&ln, &t); doy = t.tm_yday + 1; }
  sky::Vector v = sky::computeVector(sun(), doy, loc_.valid && loc_.lat < 0, wx_, ht, wx_.valid);
  return sky::decay(v, wx_.valid ? weatherAgeHours() : 0, MAX_FRESH_H);
}

// ---------- jobs ----------
void SkyClient::buildWeatherPath(char* out, size_t cap) const {
  snprintf(out, cap,
    "%s?latitude=%.4f&longitude=%.4f"
    "&current=cloud_cover,wind_speed_10m,wind_direction_10m,wind_gusts_10m,"
    "precipitation,weather_code,is_day,temperature_2m"
    "&daily=sunrise,sunset,daylight_duration&timezone=auto&forecast_days=1",
    wxPathPrefix_, loc_.lat, loc_.lon);
}

bool SkyClient::startJob(Job j) {
  if (st_ == St::Reading) return false;
  const char* host = (j == Job::Weather) ? wxHost_ : ipHost_;
  char path[320];
  if (j == Job::Weather) buildWeatherPath(path, sizeof path);
  else snprintf(path, sizeof path, "%s", ipPath_);

  uint32_t t0 = millis();
  len_ = 0;
  bool ok = client_.connect(host, 80);       // the one blocking call
  lastConnectMs_ = millis() - t0;
  if (!ok) { job_ = j; finishJob(false); return false; }
  client_.printf("GET %s HTTP/1.0\r\nHost: %s\r\nUser-Agent: pi_pico_komorebi\r\nConnection: close\r\n\r\n",
                 path, host);
  job_ = j; st_ = St::Reading; len_ = 0; jobStartMs_ = millis();
  return true;
}

void SkyClient::serviceJob() {
  if (st_ != St::Reading) return;
  // read a slice per frame
  int n;
  while ((n = client_.available()) > 0 && len_ < BUF - 1) {
    int r = client_.read((uint8_t*)buf_ + len_, min((size_t)n, BUF - 1 - len_));
    if (r <= 0) break;
    len_ += r;
    if (len_ >= BUF - 1) break;
    if (n > 256) break;                        // cap per-frame work
  }
  bool closed = !client_.connected() && client_.available() == 0;
  bool timeout = millis() - jobStartMs_ > JOB_TIMEOUT_MS;
  if (!closed && !timeout && len_ < BUF - 1) return;
  client_.stop();
  buf_[len_] = 0;
  if (timeout && len_ == 0) { finishJob(false); return; }

  // split headers / body, require 200
  char* body = strstr(buf_, "\r\n\r\n");
  bool ok = false;
  if (body && strncmp(buf_, "HTTP/1.", 7) == 0 && strstr(buf_, " 200 ") && strstr(buf_, " 200 ") < body) {
    body += 4;
    size_t blen = len_ - (body - buf_);
    if (job_ == Job::Weather) { sky::Weather w; ok = sky::parseWeather(body, blen, &w); if (ok) { wx_ = w; wxFetchedMs_ = millis(); } }
    else                      { sky::Location l; ok = sky::parseLocation(body, blen, &l); if (ok) loc_ = l; }
  }
  if (!ok) {                                  // say what came back, first line only
    char line[80]; size_t n = 0;
    while (n < sizeof(line) - 1 && n < len_ && buf_[n] != '\r' && buf_[n] != '\n') { line[n] = buf_[n]; n++; }
    line[n] = 0;
    Serial.printf("[sky] reply: %s%s\n", line, timeout ? " (timeout)" : "");
  }
  finishJob(ok);
}

void SkyClient::finishJob(bool ok) {
  st_ = ok ? St::Done : St::Failed;
  if (job_ == Job::Weather) {
    nextTryMs_ = millis() + (ok ? refreshMs_ : retryMs_);
    consecFail_ = ok ? 0 : consecFail_ + 1;
  }
  Serial.printf("[sky] %s %s (%u B, connect %lu ms)\n",
                job_ == Job::Weather ? "weather" : "location", ok ? "ok" : "FAILED",
                (unsigned)len_, (unsigned long)lastConnectMs_);
  job_ = Job::None;
  st_ = St::Idle;
}

void SkyClient::reassociate(const char* why) {
  Serial.printf("[sky] wifi reassociate (%s)\n", why);
  WiFi.disconnect();
  delay(50);
  WiFi.beginNoBlock(ssid_, pass_[0] ? pass_ : nullptr);   // never blocks the frame loop
  consecFail_ = 0;
}

bool SkyClient::runBlocking(Job j, uint32_t capMs) {
  uint32_t t0 = millis();
  if (!startJob(j)) return false;
  while (st_ == St::Reading && millis() - t0 < capMs) { serviceJob(); delay(5); }
  if (st_ == St::Reading) { client_.stop(); finishJob(false); }
  return (j == Job::Weather) ? wx_.valid : loc_.valid;
}

// ---------- public ----------
extern "C" void cyw43_set_pio_clkdiv_int_frac8(uint32_t clock_div_int, uint8_t clock_div_frac8);
void SkyClient::prepareRadioForDvi() { cyw43_set_pio_clkdiv_int_frac8(3, 0); }

void SkyClient::setCredentials(const char* ssid, const char* pass) {
  strncpy(ssid_, ssid ? ssid : "", 32); strncpy(pass_, pass ? pass : "", 64);
}

bool SkyClient::bootSync(uint32_t capMs) {
  uint32_t t0 = millis();
  if (WiFi.status() != WL_CONNECTED) { Serial.println("[sky] no wifi, neutral"); return false; }
  NTP.begin("pool.ntp.org", "time.nist.gov");
  runBlocking(Job::Location, capMs / 3);
  NTP.waitSet(capMs - (millis() - t0) > 2000 ? capMs - (millis() - t0) : 2000);
  Serial.printf("[sky] ntp %s\n", haveTime() ? "set" : "NOT set");
  if (loc_.valid) runBlocking(Job::Weather, capMs > (millis() - t0) ? capMs - (millis() - t0) : 2000);
  nextTryMs_ = millis() + (wx_.valid ? refreshMs_ : retryMs_);
  logStatus(Serial);
  return haveTime();
}

void SkyClient::tick() {
  uint32_t now = millis();

  // WiFi watchdog: the core does not auto-reconnect a dropped STA link.
  if (WiFi.status() != WL_CONNECTED) {
    if (downSinceMs_ == 0) { downSinceMs_ = now; Serial.printf("[sky] wifi down (status %d)\n", WiFi.status()); }
    else if (now - downSinceMs_ > 30000 && ssid_[0]) {
      reassociate("link down");
      downSinceMs_ = now;                              // next attempt in 30 s if still down
    }
    if (st_ == St::Reading) { client_.stop(); finishJob(false); }
    return;
  }
  if (downSinceMs_) { downSinceMs_ = 0; Serial.printf("[sky] wifi back, ip %s\n", WiFi.localIP().toString().c_str()); due_ = true; }

  if (st_ == St::Reading) { serviceJob(); return; }
  if (due_ || (int32_t)(now - nextTryMs_) >= 0) {
    due_ = false;
    if (consecFail_ >= 2 && ssid_[0]) {               // associated but not routed: start over
      reassociate("2 failed fetches while connected");
      nextTryMs_ = now + 60000;
      return;
    }
    if (!loc_.valid) { startJob(Job::Location); nextTryMs_ = now + retryMs_; return; }
    startJob(Job::Weather);
  }
}

void SkyClient::logStatus(Print& out) const {
  sky::Vector v = vector();
  sky::SunPos s = sun();
  time_t ln = localNow(); struct tm t; gmtime_r(&ln, &t);
  out.printf("[sky] time %s %04d-%02d-%02d %02d:%02d local | loc %s %.3f,%.3f %s | wx %s age %.2fh cloud %.0f%% wind %.1f/%.1f km/h dir %.0f | sun el %.1f az %.0f | model L%.2f W%.2f M%.2f F%.2f%s | heap %u\n",
    haveTime() ? "ok" : "none", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min,
    loc_.valid ? "ok" : "none", loc_.lat, loc_.lon, loc_.city,
    wx_.valid ? "ok" : "none", wx_.valid ? weatherAgeHours() : 0.0f, wx_.cloud_pct, wx_.wind_kmh, wx_.gust_kmh, wx_.wind_dir_deg,
    s.elevation_deg, s.azimuth_deg, v.light, v.warmth, v.motion, v.foliage, v.known ? "" : " (neutral)",
    rp2040.getFreeHeap());
}
