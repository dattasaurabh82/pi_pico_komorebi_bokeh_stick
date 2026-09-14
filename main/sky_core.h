// sky_core.h — the pure part of the sky layer: no Arduino, no lwIP, no
// clock. Everything here is a function of its arguments, so the same file
// is compiled and tested on a Mac (tests/sky_host) and on the Pico.
//
// The model: the outside sky collapses to four numbers on 0..1 where 0.5
// means "an average day". The engine (later) adds sign * k * (v - 0.5)
// around the encoder values. Unknown = 0.5, so no data changes nothing.
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <time.h>

namespace sky {

// ---------- tiny JSON readers (flat key scan inside one object) ----------
// Find `"key":{ ... }` and return a pointer to the byte after '{' plus the
// length up to the matching '}'. nullptr if absent.
const char* jsonObject(const char* json, size_t jsonLen, const char* key, size_t* outLen);
// `"key": <number>` inside [json, json+len). false if absent/malformed.
bool jsonNumber(const char* json, size_t len, const char* key, float* out);
// `"key": "string"` or `"key": ["string", ...]` (first element). false if absent.
bool jsonString(const char* json, size_t len, const char* key, char* out, size_t cap);

// ---------- time helpers ----------
// "2026-09-14T06:38" (Open-Meteo local time) -> minutes since local midnight.
bool parseIsoMinutes(const char* s, int* dateYmd /*20260914*/, int* minutesOfDay);
int  dayOfYear(int year, int month, int day);   // 1..366

// ---------- sun (NOAA / Meeus, good to ~0.1 deg, no refraction) ----------
struct SunPos { float elevation_deg; float azimuth_deg; };
SunPos sunPosition(double lat_deg, double lon_deg, time_t utc);

// ---------- what a fetch yields ----------
struct Weather {
  bool  valid = false;
  float cloud_pct = 0, wind_kmh = 0, gust_kmh = 0, wind_dir_deg = 0;
  float precip_mm = 0, temp_c = 0;
  int   weather_code = 0, is_day = 0;
  int   utc_offset_s = 0;
  int   sunrise_min = -1, sunset_min = -1;   // local minutes of day
};
struct Location { bool valid = false; float lat = 0, lon = 0; int utc_offset_s = 0; char city[32] = {0}; };

bool parseWeather(const char* json, size_t len, Weather* w);
bool parseLocation(const char* json, size_t len, Location* l);

// ---------- the model ----------
struct Vector {
  float light   = 0.5f;  // 0 dark night .. 1 blazing clear noon
  float warmth  = 0.5f;  // 0 cold/absent light .. 1 golden hour
  float motion  = 0.5f;  // 0 still .. 1 storm
  float foliage = 0.5f;  // 0 bare winter .. 1 dense summer
  bool  known   = false; // true if anything above is not a default
};
// haveTime: sun and doy are real. haveWeather: w is real (valid).
Vector computeVector(const SunPos& sun, int doy, bool southernHemisphere,
                     const Weather& w, bool haveTime, bool haveWeather);
// Past maxFreshHours, blend toward 0.5; fully neutral at 2x maxFreshHours.
Vector decay(const Vector& v, float ageHours, float maxFreshHours);

// ---------- what the light does with it ----------
// Signed offsets around the user's encoder values. sign = -1 complement
// (push away from the sky), +1 mirror (follow it). influence scales all.
// Neutral vector (0.5 everywhere) gives all-zero offsets in both modes.
struct Offsets {
  float warmth   = 0;   // points on the 0..100 warmth scale
  float breeze   = 0;   // points on the 0..100 breeze scale
  float density  = 0;   // dapples
  float exposure = 0;   // palette gain, +0.30 = 30% brighter mid-tones
};
struct OffsetRanges { float warmth, breeze, density, exposure; };  // full swing at |v-0.5| = 0.5
Offsets mapOffsets(const Vector& v, int sign, float influence, const OffsetRanges& r);

} // namespace sky
