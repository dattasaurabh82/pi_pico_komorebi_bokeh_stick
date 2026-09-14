// sky_core.cpp — see sky_core.h. Pure functions only.
#include "sky_core.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

namespace sky {

// ---------- JSON ----------
// Locate `"key":` inside [p, p+len). Returns pointer just after the colon.
static const char* findKey(const char* p, size_t len, const char* key) {
  size_t klen = strlen(key);
  const char* end = p + len;
  for (const char* s = p; s + klen + 3 <= end; s++) {
    if (s[0] == '"' && memcmp(s + 1, key, klen) == 0 && s[1 + klen] == '"') {
      const char* q = s + klen + 2;
      while (q < end && (*q == ' ' || *q == '\t' || *q == '\n' || *q == '\r')) q++;
      if (q < end && *q == ':') return q + 1;
    }
  }
  return nullptr;
}

static const char* skipWs(const char* q, const char* end) {
  while (q < end && (*q == ' ' || *q == '\t' || *q == '\n' || *q == '\r')) q++;
  return q;
}

const char* jsonObject(const char* json, size_t jsonLen, const char* key, size_t* outLen) {
  const char* end = json + jsonLen;
  const char* q = findKey(json, jsonLen, key);
  if (!q) return nullptr;
  q = skipWs(q, end);
  if (q >= end || *q != '{') return nullptr;
  const char* start = q + 1;
  int depth = 1; bool inStr = false;
  for (const char* s = start; s < end; s++) {
    char c = *s;
    if (inStr) { if (c == '\\') s++; else if (c == '"') inStr = false; continue; }
    if (c == '"') inStr = true;
    else if (c == '{') depth++;
    else if (c == '}') { if (--depth == 0) { *outLen = (size_t)(s - start); return start; } }
  }
  return nullptr;   // truncated
}

bool jsonNumber(const char* json, size_t len, const char* key, float* out) {
  const char* end = json + len;
  const char* q = findKey(json, len, key);
  if (!q) return false;
  q = skipWs(q, end);
  if (q >= end) return false;
  char buf[32]; size_t n = 0;
  while (q < end && n < sizeof(buf) - 1 &&
         ((*q >= '0' && *q <= '9') || *q == '-' || *q == '+' || *q == '.' || *q == 'e' || *q == 'E'))
    buf[n++] = *q++;
  if (n == 0) return false;
  buf[n] = 0;
  char* e = nullptr;
  float v = strtof(buf, &e);
  if (e == buf) return false;
  *out = v;
  return true;
}

bool jsonString(const char* json, size_t len, const char* key, char* out, size_t cap) {
  const char* end = json + len;
  const char* q = findKey(json, len, key);
  if (!q || cap == 0) return false;
  q = skipWs(q, end);
  if (q < end && *q == '[') q = skipWs(q + 1, end);   // first array element
  if (q >= end || *q != '"') return false;
  q++;
  size_t n = 0;
  while (q < end && *q != '"') {
    if (n + 1 >= cap) return false;
    out[n++] = *q++;
  }
  if (q >= end) return false;   // unterminated
  out[n] = 0;
  return true;
}

// ---------- time ----------
bool parseIsoMinutes(const char* s, int* dateYmd, int* minutesOfDay) {
  // YYYY-MM-DDTHH:MM
  if (!s || strlen(s) < 16) return false;
  static const int digits[12] = {0,1,2,3,5,6,8,9,11,12,14,15};
  for (int i = 0; i < 12; i++) if (s[digits[i]] < '0' || s[digits[i]] > '9') return false;
  if (s[4] != '-' || s[7] != '-' || s[10] != 'T' || s[13] != ':') return false;
  int y = atoi(s), mo = atoi(s + 5), d = atoi(s + 8), h = atoi(s + 11), mi = atoi(s + 14);
  if (mo < 1 || mo > 12 || d < 1 || d > 31 || h > 23 || mi > 59) return false;
  if (dateYmd) *dateYmd = y * 10000 + mo * 100 + d;
  if (minutesOfDay) *minutesOfDay = h * 60 + mi;
  return true;
}

int dayOfYear(int year, int month, int day) {
  static const int cum[12] = {0,31,59,90,120,151,181,212,243,273,304,334};
  if (month < 1 || month > 12) return 1;
  int doy = cum[month - 1] + day;
  bool leap = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
  if (leap && month > 2) doy++;
  return doy;
}

// ---------- sun ----------
static inline double rad(double d) { return d * M_PI / 180.0; }
static inline double deg(double r) { return r * 180.0 / M_PI; }
static inline double wrap360(double a) { a = fmod(a, 360.0); return a < 0 ? a + 360.0 : a; }

SunPos sunPosition(double lat, double lon, time_t utc) {
  // NOAA solar calculator equations (Meeus). Time as Julian centuries J2000.
  double jd = (double)utc / 86400.0 + 2440587.5;
  double T  = (jd - 2451545.0) / 36525.0;
  double L0 = wrap360(280.46646 + T * (36000.76983 + T * 0.0003032));
  double M  = wrap360(357.52911 + T * (35999.05029 - 0.0001537 * T));
  double e  = 0.016708634 - T * (0.000042037 + 0.0000001267 * T);
  double C  = sin(rad(M)) * (1.914602 - T * (0.004817 + 0.000014 * T))
            + sin(rad(2 * M)) * (0.019993 - 0.000101 * T)
            + sin(rad(3 * M)) * 0.000289;
  double trueLong = L0 + C;
  double omega = 125.04 - 1934.136 * T;
  double lambda = trueLong - 0.00569 - 0.00478 * sin(rad(omega));
  double eps0 = 23.0 + (26.0 + (21.448 - T * (46.815 + T * (0.00059 - T * 0.001813))) / 60.0) / 60.0;
  double eps  = eps0 + 0.00256 * cos(rad(omega));
  double decl = asin(sin(rad(eps)) * sin(rad(lambda)));
  double y = tan(rad(eps / 2)); y *= y;
  double eqTime = 4.0 * deg(y * sin(2 * rad(L0)) - 2 * e * sin(rad(M))
                          + 4 * e * y * sin(rad(M)) * cos(2 * rad(L0))
                          - 0.5 * y * y * sin(4 * rad(L0))
                          - 1.25 * e * e * sin(2 * rad(M)));            // minutes
  double minutesUtc = (double)(utc % 86400) / 60.0;
  double tst = minutesUtc + eqTime + 4.0 * lon;                          // true solar time
  double ha = tst / 4.0 - 180.0;                                         // hour angle, deg
  if (ha < -180) ha += 360;
  double phi = rad(lat);
  double cosZ = sin(phi) * sin(decl) + cos(phi) * cos(decl) * cos(rad(ha));
  if (cosZ > 1) cosZ = 1; if (cosZ < -1) cosZ = -1;
  double z = acos(cosZ);
  double el = 90.0 - deg(z);
  double az;
  double denom = cos(phi) * sin(z);
  if (fabs(denom) < 1e-9) az = 180.0;
  else {
    double a = (sin(phi) * cos(z) - sin(decl)) / denom;
    if (a > 1) a = 1; if (a < -1) a = -1;
    az = deg(acos(a));
    az = (ha > 0) ? wrap360(az + 180.0) : wrap360(540.0 - az);
  }
  SunPos p; p.elevation_deg = (float)el; p.azimuth_deg = (float)az;
  return p;
}

// ---------- parsers ----------
bool parseWeather(const char* json, size_t len, Weather* w) {
  Weather r;
  size_t clen = 0;
  const char* cur = jsonObject(json, len, "current", &clen);
  if (!cur) return false;
  float f;
  if (!jsonNumber(cur, clen, "cloud_cover", &f)) return false;      r.cloud_pct = f;
  if (!jsonNumber(cur, clen, "wind_speed_10m", &f)) return false;   r.wind_kmh = f;
  if (jsonNumber(cur, clen, "wind_gusts_10m", &f)) r.gust_kmh = f; else r.gust_kmh = r.wind_kmh;
  if (jsonNumber(cur, clen, "wind_direction_10m", &f)) r.wind_dir_deg = f;
  if (jsonNumber(cur, clen, "precipitation", &f)) r.precip_mm = f;
  if (jsonNumber(cur, clen, "temperature_2m", &f)) r.temp_c = f;
  if (jsonNumber(cur, clen, "weather_code", &f)) r.weather_code = (int)f;
  if (jsonNumber(cur, clen, "is_day", &f)) r.is_day = (int)f;
  if (jsonNumber(json, len, "utc_offset_seconds", &f)) r.utc_offset_s = (int)f;
  size_t dlen = 0;
  const char* daily = jsonObject(json, len, "daily", &dlen);
  if (daily) {
    char s[24];
    if (jsonString(daily, dlen, "sunrise", s, sizeof(s))) parseIsoMinutes(s, nullptr, &r.sunrise_min);
    if (jsonString(daily, dlen, "sunset",  s, sizeof(s))) parseIsoMinutes(s, nullptr, &r.sunset_min);
  }
  // sanity
  if (r.cloud_pct < 0 || r.cloud_pct > 100 || r.wind_kmh < 0 || r.wind_kmh > 400) return false;
  r.valid = true;
  *w = r;
  return true;
}

bool parseLocation(const char* json, size_t len, Location* l) {
  Location r;
  char st[16];
  if (!jsonString(json, len, "status", st, sizeof(st)) || strcmp(st, "success") != 0) return false;
  float f;
  if (!jsonNumber(json, len, "lat", &f) || f < -90 || f > 90) return false;   r.lat = f;
  if (!jsonNumber(json, len, "lon", &f) || f < -180 || f > 180) return false; r.lon = f;
  if (jsonNumber(json, len, "offset", &f)) r.utc_offset_s = (int)f;
  jsonString(json, len, "city", r.city, sizeof(r.city));
  r.valid = true;
  *l = r;
  return true;
}

// ---------- model ----------
static inline float clamp01(float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }
static inline float smooth(float a, float b, float x) {   // smoothstep a..b
  float t = clamp01((x - a) / (b - a));
  return t * t * (3 - 2 * t);
}

Vector computeVector(const SunPos& sun, int doy, bool south,
                     const Weather& w, bool haveTime, bool haveWeather) {
  Vector v;
  if (!haveTime && !haveWeather) return v;          // all 0.5, known=false
  v.known = true;

  float cloud = haveWeather ? w.cloud_pct / 100.0f : 0.3f;   // 30% = average
  if (haveWeather) {
    // crispness of the outside light: 0.3 cloud = average (0.5); rain flattens further
    v.crisp = cloud <= 0.3f ? 0.5f + (0.3f - cloud) / 0.3f * 0.5f
                            : 0.5f - (cloud - 0.3f) / 0.7f * 0.5f;
    if (w.precip_mm > 0.1f) v.crisp -= 0.15f;
    v.crisp = clamp01(v.crisp);
  }
  if (haveTime) {
    float e = sun.elevation_deg;
    // daylight 0 at civil dusk (-6), 1 from 30 deg up; clouds take up to 70%
    float day = smooth(-6.0f, 30.0f, e);
    v.light = clamp01(day * (1.0f - 0.7f * cloud));
    // outside light warmth: absent at night, golden near horizon, neutral
    // high; overcast light is bluish, so cloud takes up to 40% off
    v.warmth = (e <= 0) ? 0.0f : (0.3f + 0.7f * (1.0f - smooth(0.0f, 35.0f, e))) * (1.0f - 0.4f * cloud);
    // foliage by season, min ~Jan 20, max ~Jul 20 (flipped south)
    float ph = (doy - 20) / 365.0f * 6.2831853f;
    v.foliage = 0.5f - 0.5f * cosf(ph);
    if (south) v.foliage = 1.0f - v.foliage;
  } else if (haveWeather) {
    // no clock: is_day is all we know about the sun
    v.light = w.is_day ? clamp01(0.7f * (1.0f - 0.7f * cloud)) : 0.05f;
    v.warmth = w.is_day ? 0.5f : 0.0f;
  }
  if (haveWeather) {
    float wind = w.wind_kmh > 0.7f * w.gust_kmh ? w.wind_kmh : 0.7f * w.gust_kmh;
    v.motion = smooth(0.0f, 40.0f, wind);           // 40 km/h+ = storm
  }
  return v;
}

Vector decay(const Vector& v, float ageHours, float maxFreshHours) {
  if (!v.known || ageHours <= maxFreshHours) return v;
  float k = 1.0f - (ageHours - maxFreshHours) / maxFreshHours;   // 1 -> 0 over another maxFresh
  if (k <= 0) { Vector n; return n; }
  Vector r = v;
  r.light   = 0.5f + (v.light   - 0.5f) * k;
  r.warmth  = 0.5f + (v.warmth  - 0.5f) * k;
  r.motion  = 0.5f + (v.motion  - 0.5f) * k;
  r.foliage = 0.5f + (v.foliage - 0.5f) * k;
  r.crisp   = 0.5f + (v.crisp   - 0.5f) * k;
  return r;
}

Offsets mapOffsets(const Vector& v, int sign, float influence, const OffsetRanges& r) {
  Offsets o;
  if (!v.known) return o;
  float k = (sign < 0 ? -1.0f : 1.0f) * influence * 2.0f;   // (v-0.5) spans -0.5..0.5
  o.exposure = k * (v.light   - 0.5f) * r.exposure;
  o.warmth   = k * (v.warmth  - 0.5f) * r.warmth;
  o.breeze   = k * (v.motion  - 0.5f) * r.breeze;
  o.density  = k * (v.foliage - 0.5f) * r.density;
  o.contrast = k * (v.crisp   - 0.5f) * r.contrast;
  return o;
}

} // namespace sky
