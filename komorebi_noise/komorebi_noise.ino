// komorebi_noise — variant C: a different style. No discrete blobs; a slowly
// evolving value-noise field pushed through a soft threshold. Light shapes
// morph / merge / split like real canopy gaps, instead of translating.

#include <PicoDVI.h>

DVIGFX8 display(DVI_RES_320x240p60, true, pico_sock_cfg);

#define W 320
#define H 240
#define CELL 16              // grid cell in px
#define GW (W / CELL + 1)    // 21 grid points
#define GH (H / CELL + 1)    // 16

static uint8_t g[GW * GH];   // current field values 0..255
static uint8_t lut[256];     // soft-threshold mapping, rebuilt each frame
// Per gridpoint temporal params: two slow sines each.
static float gf1[GW * GH], gf2[GW * GH], gp1[GW * GH], gp2[GW * GH];

static float frand(float lo, float hi) {
  return lo + (hi - lo) * (random(10000) / 10000.0f);
}

static uint32_t entropy_seed() {
  uint32_t s = micros();
  for (int i = 0; i < 16; i++) {
    s = (s << 3) ^ (s >> 5) ^ (uint32_t)analogRead(A0) ^ (s * 2654435761u);
    delayMicroseconds(37 + (s & 63));
  }
  return s;
}

void setup() {
  if (!display.begin()) {
    pinMode(LED_BUILTIN, OUTPUT);
    for (;;) digitalWrite(LED_BUILTIN, (millis() / 500) & 1);
  }
  for (int i = 0; i < 256; i++) { // pale warm-white
    float t = i / 255.0f;
    display.setColor(i, (uint8_t)(255 * powf(t, 0.75f)),
                        (uint8_t)(242 * powf(t, 0.82f)),
                        (uint8_t)(205 * powf(t, 1.05f)));
  }
  display.swap(false, true);

  randomSeed(entropy_seed());
  // Each gridpoint breathes with its own two incommensurate slow periods
  // (~45–500 s) → the field never repeats, shapes morph organically.
  for (int i = 0; i < GW * GH; i++) {
    gf1[i] = frand(0.002f, 0.012f);
    gf2[i] = frand(0.006f, 0.022f);
    gp1[i] = frand(0, 6.283f);
    gp2[i] = frand(0, 6.283f);
  }
}

void loop() {
  float t = millis() / 1000.0f;

  // 1) Update the coarse field (336 points, cheap).
  for (int i = 0; i < GW * GH; i++) {
    float v = 0.5f + 0.30f * sinf(t * gf1[i] * 6.283f + gp1[i])
                   + 0.20f * sinf(t * gf2[i] * 6.283f + gp2[i]);
    g[i] = (uint8_t)(v * 255.0f);
  }

  // 2) Soft threshold LUT: light where field > th; wide smooth penumbra.
  //    Threshold itself gusts slowly = clouds/wind opening & closing gaps.
  float th = 0.58f + 0.06f * sinf(t * 0.013f) + 0.03f * sinf(t * 0.031f + 2.1f);
  float edge = 0.16f; // penumbra width
  for (int i = 0; i < 256; i++) {
    float x = (i / 255.0f - th) / edge;
    x = (x < 0) ? 0 : ((x > 1) ? 1 : x);
    x = x * x * (3.0f - 2.0f * x);          // smoothstep
    lut[i] = (uint8_t)(powf(x, 1.25f) * 235.0f); // cap under palette white
  }

  // 3) Bilinear-upsample field → pixels, through the LUT.
  uint8_t *buf = display.getBuffer();
  for (int y = 0; y < H; y++) {
    int gy = y / CELL, fy = y % CELL;
    const uint8_t *r0 = g + gy * GW;
    const uint8_t *r1 = g + (gy + 1) * GW;
    uint8_t *row = buf + y * W;
    for (int gx = 0; gx < GW - 1; gx++) {
      int a = r0[gx] * (CELL - fy) + r1[gx] * fy;         // 0..255*16
      int b = r0[gx + 1] * (CELL - fy) + r1[gx + 1] * fy;
      uint8_t *p = row + gx * CELL;
      for (int fx = 0; fx < CELL; fx++) {
        int val = (a * (CELL - fx) + b * fx) >> 8;        // 0..255
        p[fx] = lut[val];
      }
    }
  }
  display.swap();
}
