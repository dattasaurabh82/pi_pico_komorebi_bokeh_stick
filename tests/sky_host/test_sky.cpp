// Host tests for main/sky_core.{h,cpp}. Build + run:
//   cd tests/sky_host && ./run.sh
// No hardware, no Arduino. Fixtures are real responses captured 2026-09-14.
#include "../../main/sky_core.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <fstream>
#include <sstream>
#include <ctime>

static int fails = 0, checks = 0;
#define CHECK(cond, ...) do { checks++; if (!(cond)) { fails++; printf("  FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)
#define NEAR(a, b, tol, ...) CHECK(fabs((double)(a) - (double)(b)) <= (tol), __VA_ARGS__)

static std::string slurp(const char* path) {
  std::ifstream f(path); std::stringstream ss; ss << f.rdbuf(); return ss.str();
}
static time_t utc(int y, int mo, int d, int h, int mi) {
  struct tm t = {}; t.tm_year = y - 1900; t.tm_mon = mo - 1; t.tm_mday = d; t.tm_hour = h; t.tm_min = mi;
  return timegm(&t);
}

int main() {
  using namespace sky;
  std::string om = slurp("fixtures/open_meteo_berlin_2026-09-14.json");
  std::string ip = slurp("fixtures/ip_api_2026-09-14.json");
  std::string ipf = slurp("fixtures/ip_api_fail.json");
  std::string e404 = slurp("fixtures/open_meteo_404.html");
  CHECK(om.size() > 700 && ip.size() > 50, "fixtures loaded (%zu, %zu)", om.size(), ip.size());

  // ---- parse the real weather response ----
  printf("weather parse\n");
  Weather w;
  CHECK(parseWeather(om.c_str(), om.size(), &w), "parseWeather on real fixture");
  NEAR(w.cloud_pct, 98, 0.01, "cloud %f", w.cloud_pct);
  NEAR(w.wind_kmh, 9.5, 0.01, "wind %f", w.wind_kmh);
  NEAR(w.gust_kmh, 20.2, 0.01, "gust %f", w.gust_kmh);
  NEAR(w.wind_dir_deg, 295, 0.01, "dir %f", w.wind_dir_deg);
  CHECK(w.weather_code == 3 && w.is_day == 0, "code %d is_day %d", w.weather_code, w.is_day);
  CHECK(w.utc_offset_s == 7200, "offset %d", w.utc_offset_s);
  CHECK(w.sunrise_min == 6 * 60 + 38, "sunrise %d", w.sunrise_min);
  CHECK(w.sunset_min == 19 * 60 + 24, "sunset %d", w.sunset_min);
  // the "time" trap: current_units also has "time"; must not leak into current
  size_t cl; const char* cur = jsonObject(om.c_str(), om.size(), "current", &cl);
  char ts[24]; CHECK(cur && jsonString(cur, cl, "time", ts, sizeof ts) && strcmp(ts, "2026-09-14T02:30") == 0, "current.time = %s", ts);

  // ---- parse location ----
  printf("location parse\n");
  Location l;
  CHECK(parseLocation(ip.c_str(), ip.size(), &l), "parseLocation real");
  NEAR(l.lat, 52.5312, 0.001, "lat %f", l.lat);
  NEAR(l.lon, 13.3878, 0.001, "lon %f", l.lon);
  CHECK(l.utc_offset_s == 7200 && strcmp(l.city, "Berlin") == 0, "offset %d city %s", l.utc_offset_s, l.city);
  CHECK(!parseLocation(ipf.c_str(), ipf.size(), &l), "ip-api status=fail rejected");

  // ---- malformed inputs: must fail cleanly, never crash ----
  printf("malformed inputs\n");
  CHECK(!parseWeather(e404.c_str(), e404.size(), &w), "404 body rejected");
  CHECK(!parseWeather("", 0, &w), "empty rejected");
  for (size_t cut = 1; cut < om.size(); cut += 37) {   // every truncation point
    std::string t = om.substr(0, cut); Weather tw;
    bool ok = parseWeather(t.c_str(), t.size(), &tw);
    if (ok) CHECK(tw.cloud_pct == 98 && tw.wind_kmh == 9.5f, "truncated@%zu parsed but wrong", cut);
  }
  std::string reordered = "{\"daily\":{\"sunrise\":[\"2026-09-14T06:38\"],\"sunset\":[\"2026-09-14T19:24\"]},\"utc_offset_seconds\":7200,\"current\":{\"wind_speed_10m\":3.0,\"cloud_cover\":10,\"is_day\":1}}";
  CHECK(parseWeather(reordered.c_str(), reordered.size(), &w) && w.cloud_pct == 10 && w.gust_kmh == 3.0f && w.sunrise_min == 398, "reordered keys ok");
  std::string missing = "{\"current\":{\"wind_speed_10m\":3.0}}";
  CHECK(!parseWeather(missing.c_str(), missing.size(), &w), "missing cloud_cover rejected");
  std::string garbage = "{\"current\":{\"cloud_cover\":\"abc\",\"wind_speed_10m\":3.0}}";
  CHECK(!parseWeather(garbage.c_str(), garbage.size(), &w), "non-numeric cloud rejected");
  std::string range = "{\"current\":{\"cloud_cover\":250,\"wind_speed_10m\":3.0}}";
  CHECK(!parseWeather(range.c_str(), range.size(), &w), "out-of-range cloud rejected");
  float f; CHECK(!jsonNumber("{\"a\":", 5, "a", &f), "number at EOF rejected");
  char sb[8]; CHECK(!jsonString("{\"a\":\"toolongstring\"}", 20, "a", sb, sizeof sb), "string overflow rejected");

  // ---- sun position vs Open-Meteo sunrise/sunset (local 06:38 / 19:24, UTC+2) and NOAA ----
  printf("sun position\n");
  double lat = 52.56, lon = 13.39;
  SunPos rise = sunPosition(lat, lon, utc(2026, 9, 14, 4, 38));
  SunPos set  = sunPosition(lat, lon, utc(2026, 9, 14, 17, 24));
  SunPos noon = sunPosition(lat, lon, utc(2026, 9, 14, 11, 1));   // solar noon ~ midpoint
  SunPos mid  = sunPosition(lat, lon, utc(2026, 9, 14, 0, 0));
  NEAR(rise.elevation_deg, -0.83, 1.0, "sunrise elev %.2f (expect ~ -0.83)", rise.elevation_deg);
  NEAR(set.elevation_deg,  -0.83, 1.0, "sunset elev %.2f (expect ~ -0.83)", set.elevation_deg);
  NEAR(rise.azimuth_deg,  84, 4, "sunrise az %.1f (expect ~84)", rise.azimuth_deg);
  NEAR(set.azimuth_deg,  276, 4, "sunset az %.1f (expect ~276)", set.azimuth_deg);
  NEAR(noon.azimuth_deg, 180, 3, "noon az %.1f", noon.azimuth_deg);
  NEAR(noon.elevation_deg, 40.6, 1.0, "noon elev %.1f (90-52.56+decl 3.1)", noon.elevation_deg);
  CHECK(mid.elevation_deg < -30, "midnight elev %.1f", mid.elevation_deg);
  // solstices, Berlin, ~solar noon 11:00 UTC: 90-52.56+-23.44
  NEAR(sunPosition(lat, lon, utc(2026, 6, 21, 11, 5)).elevation_deg, 60.9, 1.0, "june noon");
  NEAR(sunPosition(lat, lon, utc(2026, 12, 21, 11, 0)).elevation_deg, 14.0, 1.0, "dec noon");
  // southern hemisphere sanity: Sydney Dec noon ~ 90-33.87+23.44 = 79.6, sun to the south (az ~180) or north? (south lat: az near 0/360 at noon in Dec is wrong; sun is south only if decl < lat) -> just elevation
  NEAR(sunPosition(-33.87, 151.21, utc(2026, 12, 21, 1, 55)).elevation_deg, 79.6, 1.5, "sydney dec noon");

  // ---- calendar ----
  CHECK(dayOfYear(2026, 1, 1) == 1 && dayOfYear(2026, 12, 31) == 365 && dayOfYear(2024, 12, 31) == 366 && dayOfYear(2026, 9, 14) == 257, "dayOfYear");
  int ymd, mins; CHECK(parseIsoMinutes("2026-09-14T06:38", &ymd, &mins) && ymd == 20260914 && mins == 398, "iso parse");
  CHECK(!parseIsoMinutes("2026-09-14", nullptr, nullptr) && !parseIsoMinutes("2026-13-14T06:38", nullptr, nullptr), "iso reject");

  // ---- model ----
  printf("model\n");
  Weather clear; clear.valid = true; clear.cloud_pct = 0; clear.wind_kmh = 5; clear.gust_kmh = 8;
  Weather over  = clear; over.cloud_pct = 100;
  Weather storm = clear; storm.wind_kmh = 55; storm.gust_kmh = 90;
  SunPos high; high.elevation_deg = 45; high.azimuth_deg = 180;
  SunPos low;  low.elevation_deg = 5;   low.azimuth_deg = 250;
  SunPos night; night.elevation_deg = -30; night.azimuth_deg = 0;
  Vector v;
  v = computeVector(high, 172, false, clear, true, true);
  CHECK(v.known && v.light > 0.95f && v.warmth < 0.35f && v.motion < 0.1f && v.foliage > 0.9f, "clear summer noon: L%.2f W%.2f M%.2f F%.2f", v.light, v.warmth, v.motion, v.foliage);
  v = computeVector(high, 172, false, over, true, true);
  CHECK(v.light > 0.25f && v.light < 0.35f, "overcast noon light %.2f (~0.3)", v.light);
  CHECK(v.crisp < 0.01f && v.warmth < 0.25f, "overcast noon: flat (%.2f) and cooler (%.2f)", v.crisp, v.warmth);
  { Vector c = computeVector(high, 172, false, clear, true, true); CHECK(c.crisp > 0.99f, "clear: crisp %.2f", c.crisp); }
  { Weather avg = clear; avg.cloud_pct = 30; Vector a = computeVector(high, 172, false, avg, true, true); NEAR(a.crisp, 0.5, 0.01, "30%% cloud = average crisp %.2f", a.crisp); }
  { Weather rain = over; rain.precip_mm = 2; Vector rv = computeVector(high, 172, false, rain, true, true); CHECK(rv.crisp == 0.0f, "rain clamps flat"); }
  v = computeVector(low, 300, false, clear, true, true);
  CHECK(v.warmth > 0.85f && v.light < 0.6f && v.light > 0.1f, "golden hour: W%.2f L%.2f", v.warmth, v.light);
  v = computeVector(night, 20, false, clear, true, true);
  CHECK(v.light < 0.02f && v.warmth < 0.01f && v.foliage < 0.02f, "winter night: L%.2f W%.2f F%.2f", v.light, v.warmth, v.foliage);
  v = computeVector(high, 172, false, storm, true, true);
  CHECK(v.motion > 0.95f, "storm motion %.2f", v.motion);
  v = computeVector(high, 172, true, clear, true, true);
  CHECK(v.foliage < 0.1f, "southern hemisphere june = winter foliage %.2f", v.foliage);
  v = computeVector(high, 172, false, clear, false, false);
  CHECK(!v.known && v.light == 0.5f && v.warmth == 0.5f && v.motion == 0.5f && v.foliage == 0.5f, "no data = neutral");
  v = computeVector(high, 172, false, over, false, true);
  CHECK(v.known && v.foliage == 0.5f && v.light < 0.5f, "weather only: no season, dim under cloud L%.2f", v.light);

  // ---- decay ----
  printf("decay\n");
  Vector fresh = computeVector(night, 20, false, clear, true, true);
  Vector d3 = decay(fresh, 3, 6), d9 = decay(fresh, 9, 6), d12 = decay(fresh, 12, 6), d20 = decay(fresh, 20, 6);
  CHECK(d3.light == fresh.light, "fresh within 6 h unchanged");
  NEAR(d9.light, 0.5 + (fresh.light - 0.5) * 0.5, 0.001, "half decayed at 9 h: %.3f", d9.light);
  CHECK(!d12.known && d12.light == 0.5f && !d20.known, "neutral at 12 h and beyond");

  // ---- offsets ----
  printf("offsets\n");
  OffsetRanges R = {25, 25, 5, 0.30f, 25};
  Vector nightV = computeVector(night, 20, false, clear, true, true);     // winter night, still
  Offsets oc = mapOffsets(nightV, -1, 1.0f, R), omr = mapOffsets(nightV, +1, 1.0f, R);
  CHECK(oc.exposure > 0.28f && oc.warmth > 24 && oc.density > 4.5f, "complement winter night: brighter +%.2f warmer +%.0f denser +%.1f", oc.exposure, oc.warmth, oc.density);
  CHECK(omr.exposure < -0.28f && omr.warmth < -24 && omr.density < -4.5f, "mirror winter night: dimmer, cooler, sparser");
  NEAR(oc.exposure, -omr.exposure, 1e-5, "modes are exact opposites");
  Offsets os = mapOffsets(computeVector(high, 172, false, storm, true, true), -1, 1.0f, R);
  CHECK(os.breeze < -20, "complement storm: calmer %.0f", os.breeze);
  Offsets og = mapOffsets(computeVector(high, 172, false, over, true, true), -1, 1.0f, R);
  CHECK(og.contrast > 24 && og.warmth > 5, "complement grey noon: crisper +%.0f, warmer +%.0f", og.contrast, og.warmth);
  Offsets ogm = mapOffsets(computeVector(high, 172, false, over, true, true), +1, 1.0f, R);
  CHECK(ogm.contrast < -24 && ogm.warmth < -5, "mirror grey noon: flatter %.0f, cooler %.0f", ogm.contrast, ogm.warmth);
  Offsets on = mapOffsets(computeVector(high, 172, false, clear, false, false), -1, 1.0f, R);
  CHECK(on.exposure == 0 && on.warmth == 0 && on.breeze == 0 && on.density == 0 && on.contrast == 0, "neutral -> zero offsets");
  Offsets oh = mapOffsets(nightV, -1, 0.5f, R);
  NEAR(oh.warmth, oc.warmth * 0.5f, 1e-4, "influence scales linearly");

  printf("\n%d checks, %d failed\n", checks, fails);
  return fails ? 1 : 0;
}
