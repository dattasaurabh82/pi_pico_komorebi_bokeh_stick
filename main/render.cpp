// render.cpp — table builds + the two hot loops. Comments explain the
// fixed-point choices; if you change SPRITE_SIZE or the vignette margins,
// nothing else needs touching.
#include <Arduino.h>
#include "render.h"
#include "config.h"

namespace fx {

static uint8_t sprite[SPRITE_SIZE * SPRITE_SIZE];
static uint8_t vigx[SCREEN_W], vigy[SCREEN_H];

static inline float smoothstep01(float v) { return v * v * (3.0f - 2.0f * v); }

void init() {
  // Soft radial sprite: smoothstep falloff = bright core, wide penumbra.
  const float half = SPRITE_SIZE * 0.5f;
  for (int y = 0; y < SPRITE_SIZE; y++) {
    for (int x = 0; x < SPRITE_SIZE; x++) {
      float dx = (x - half + 0.5f) / half;
      float dy = (y - half + 0.5f) / half;
      float r = sqrtf(dx * dx + dy * dy);
      float v = (r >= 1.0f) ? 0.0f : smoothstep01(1.0f - r);
      sprite[y * SPRITE_SIZE + x] = (uint8_t)(v * 255.0f);
    }
  }
  // Vignette: full brightness in the middle, smoothstep to hard black
  // over the outer margin.
  for (int x = 0; x < SCREEN_W; x++) {
    int e = min(x, SCREEN_W - 1 - x);
    float v = (e >= VIGNETTE_MARGIN_X) ? 1.0f
              : smoothstep01((float)e / VIGNETTE_MARGIN_X);
    vigx[x] = (uint8_t)(v * 255.0f);
  }
  for (int y = 0; y < SCREEN_H; y++) {
    int e = min(y, SCREEN_H - 1 - y);
    float v = (e >= VIGNETTE_MARGIN_Y) ? 1.0f
              : smoothstep01((float)e / VIGNETTE_MARGIN_Y);
    vigy[y] = (uint8_t)(v * 255.0f);
  }
}

void blitAddScaled(uint8_t* buf, int cx, int cy, int bscale, int ow, int oh) {
  if (ow < 2 || oh < 2 || bscale <= 0) return;
  // 8.8 fixed-point steps through the source sprite: one add + one shift
  // per output pixel instead of a divide.
  int stepx = (SPRITE_SIZE << 8) / ow;
  int stepy = (SPRITE_SIZE << 8) / oh;
  int x0 = cx - ow / 2, y0 = cy - oh / 2;
  int ox0 = max(0, -x0), oy0 = max(0, -y0);
  int ox1 = min(ow, SCREEN_W - x0), oy1 = min(oh, SCREEN_H - y0);
  for (int oy = oy0; oy < oy1; oy++) {
    const uint8_t* srow = sprite + ((oy * stepy) >> 8) * SPRITE_SIZE;
    uint8_t* row = buf + (y0 + oy) * SCREEN_W + x0;
    int sxf = ox0 * stepx;
    for (int ox = ox0; ox < ox1; ox++, sxf += stepx) {
      int v = row[ox] + ((srow[sxf >> 8] * bscale) >> 8);
      row[ox] = (v > 255) ? 255 : (uint8_t)v;
    }
  }
}

void vignettePass(uint8_t* buf) {
  for (int y = 0; y < SCREEN_H; y++) {
    int vy = vigy[y];
    uint8_t* row = buf + y * SCREEN_W;
    if (vy == 0) { memset(row, 0, SCREEN_W); continue; }
    for (int x = 0; x < SCREEN_W; x++) {
      int v = (vigx[x] * vy) >> 8;
      // (v + 1) keeps the fully-lit center at ~identity after the shift.
      row[x] = (uint8_t)((row[x] * (v + 1)) >> 8);
    }
  }
}

} // namespace fx
