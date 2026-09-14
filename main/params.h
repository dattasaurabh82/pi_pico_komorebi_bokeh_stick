// params.h — the three user parameters and which one the encoder is
// currently editing. Pure state, no hardware; header-only on purpose.
//
// NOTE on persistence: intentionally none for now. If revisited, the
// plan of record is an external I2C FRAM/EEPROM (see context/RESUME.md).
// Writing the RP2040's own flash while PicoDVI runs kills the video
// core — proven in tests/eeprom_dvi_spike. Do not "just add EEPROM".
#pragma once
#include <stdint.h>
#include "config.h"

enum class Param : uint8_t { Warmth = 0, Breeze = 1, Density = 2, Contrast = 3 };
constexpr uint8_t PARAM_COUNT = 4;

struct Params {
  int warmth  = WARMTH_DEFAULT;   // 0..100: cool daylight -> low amber
  int breeze  = BREEZE_DEFAULT;   // 0..100: near-still -> lively
  int density = DENSITY_DEFAULT;  // DENSITY_MIN..DENSITY_MAX dapples shown
  int contrast = CONTRAST_DEFAULT;// 0..100: flat, soft, big pools -> crisp, small, bright

  // Apply one encoder detent to the selected parameter, capped to limits.
  // Returns true if the value actually changed (at a cap it may not).
  bool adjust(Param p, int detents) {
    int* v; int lo, hi, step;
    switch (p) {
      case Param::Warmth:  v = &warmth;  lo = 0; hi = 100; step = PARAM_STEP; break;
      case Param::Breeze:  v = &breeze;  lo = 0; hi = 100; step = PARAM_STEP; break;
      case Param::Contrast:v = &contrast;lo = 0; hi = 100; step = PARAM_STEP; break;
      default:             v = &density; lo = DENSITY_MIN; hi = DENSITY_MAX; step = 1; break;
    }
    int before = *v;
    *v += detents * step;
    if (*v < lo) *v = lo;
    if (*v > hi) *v = hi;
    return *v != before;
  }
};
