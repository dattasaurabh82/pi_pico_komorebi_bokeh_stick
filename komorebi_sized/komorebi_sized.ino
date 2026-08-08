// komorebi_sized — variant B: anchored breathing dapples in 3 size classes
// with slight ellipticity. Evolves komorebi/ v2; diff against it to compare.

#include <PicoDVI.h>

DVIGFX8 display(DVI_RES_320x240p60, true, pico_sock_cfg);

#define W 320
#define H 240
#define N_DAPPLES 16
#define SPR 96

static uint8_t sprite[SPR * SPR];

struct Dapple {
  float bx, by;
  float ax1, ay1, ax2, ay2;
  float f1, f2, p1, p2;
  float bri, bf, bp;
  int ow, oh; // rendered size (px) — ellipticity = ow != oh
};
static Dapple d[N_DAPPLES];

static float frand(float lo, float hi) {
  return lo + (hi - lo) * (random(10000) / 10000.0f);
}

// Stronger boot entropy: stir 16 floating-ADC reads with micros().
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

  for (int y = 0; y < SPR; y++) { // soft radial sprite
    for (int x = 0; x < SPR; x++) {
      float dx = (x - SPR * 0.5f + 0.5f) / (SPR * 0.5f);
      float dy = (y - SPR * 0.5f + 0.5f) / (SPR * 0.5f);
      float r = sqrtf(dx * dx + dy * dy);
      float v = (r >= 1.0f) ? 0.0f : (1.0f - r);
      v = v * v * (3.0f - 2.0f * v);
      sprite[y * SPR + x] = (uint8_t)(v * 255.0f);
    }
  }

  randomSeed(entropy_seed());

  // Size classes: 3 large pools, 5 mid, 8 small sparkles between them.
  for (int i = 0; i < N_DAPPLES; i++) {
    float base;
    if (i < 3)      base = frand(110, 150); // large
    else if (i < 8) base = frand(65, 95);   // mid
    else            base = frand(30, 50);   // small
    float ecc = frand(-0.22f, 0.22f);       // squash one axis
    d[i].ow = (int)(base * (1.0f + ecc));
    d[i].oh = (int)(base * (1.0f - ecc));
    d[i].bx = frand(20, W - 20);
    d[i].by = frand(20, H - 20);
    d[i].ax1 = frand(3, 8);  d[i].ay1 = frand(2, 6);
    d[i].ax2 = frand(1, 4);  d[i].ay2 = frand(1, 3);
    d[i].f1 = frand(0.003f, 0.010f);
    d[i].f2 = frand(0.008f, 0.025f);
    d[i].p1 = frand(0, 6.283f); d[i].p2 = frand(0, 6.283f);
    // Small dapples: dimmer but livelier (leaf-tip flutter).
    if (i < 8) { d[i].bri = frand(0.40f, 0.80f); d[i].bf = frand(0.010f, 0.030f); }
    else       { d[i].bri = frand(0.25f, 0.55f); d[i].bf = frand(0.025f, 0.060f); }
    d[i].bp = frand(0, 6.283f);
  }
}

// Scaled additive blit: sample the SPR sprite at (ow x oh), clipped, saturating.
static void blit_add_scaled(uint8_t *buf, int cx, int cy, int bscale,
                            int ow, int oh) {
  int stepx = (SPR << 8) / ow, stepy = (SPR << 8) / oh;
  int x0 = cx - ow / 2, y0 = cy - oh / 2;
  int ox0 = max(0, -x0), oy0 = max(0, -y0);
  int ox1 = min(ow, W - x0), oy1 = min(oh, H - y0);
  for (int oy = oy0; oy < oy1; oy++) {
    const uint8_t *srow = sprite + ((oy * stepy) >> 8) * SPR;
    uint8_t *row = buf + (y0 + oy) * W + x0;
    int sxf = ox0 * stepx;
    for (int ox = ox0; ox < ox1; ox++, sxf += stepx) {
      int v = row[ox] + ((srow[sxf >> 8] * bscale) >> 8);
      row[ox] = (v > 255) ? 255 : (uint8_t)v;
    }
  }
}

void loop() {
  float t = millis() / 1000.0f;
  float wind = sinf(t * 0.020f) + 0.5f * sinf(t * 0.053f + 1.7f);
  uint8_t *buf = display.getBuffer();
  memset(buf, 0, W * H);
  for (int i = 0; i < N_DAPPLES; i++) {
    float x = d[i].bx + d[i].ax1 * sinf(t * d[i].f1 * 6.283f + d[i].p1)
                      + d[i].ax2 * sinf(t * d[i].f2 * 6.283f + d[i].p2)
                      + 2.0f * wind;
    float y = d[i].by + d[i].ay1 * cosf(t * d[i].f1 * 5.1f + d[i].p2)
                      + d[i].ay2 * sinf(t * d[i].f2 * 7.3f + d[i].p1);
    float b = d[i].bri * (0.55f + 0.35f * sinf(t * d[i].bf * 6.283f + d[i].bp)
                                 - 0.12f * wind * wind);
    if (b < 0.05f) b = 0.05f;
    blit_add_scaled(buf, (int)x, (int)y, (int)(b * 256), d[i].ow, d[i].oh);
  }
  display.swap();
}
