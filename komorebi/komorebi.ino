// komorebi v1 — procedural dappled light for Pico + DVI Sock → pico projector.
// DVIGFX8 double-buffered, warm palette, additive soft-sprite dapples.
// Judge ONLY with the projector slightly defocused.

#include <PicoDVI.h>

DVIGFX8 display(DVI_RES_320x240p60, true, pico_sock_cfg);

#define W 320
#define H 240
#define N_DAPPLES 12
#define SPR 96 // soft blob sprite is SPR x SPR

static uint8_t sprite[SPR * SPR]; // radial falloff, computed at boot

struct Dapple {
  float bx, by;             // anchor position
  float ax1, ay1, ax2, ay2; // drift amplitudes (two superposed sines)
  float f1, f2;             // drift frequencies (Hz, very low)
  float p1, p2;             // phases
  float bri;                // peak brightness 0..1
  float bf, bp;             // brightness wobble freq/phase
};
static Dapple d[N_DAPPLES];

static float frand(float lo, float hi) {
  return lo + (hi - lo) * (random(10000) / 10000.0f);
}

void setup() {
  if (!display.begin()) { // RAM alloc failed → blink LED forever
    pinMode(LED_BUILTIN, OUTPUT);
    for (;;) digitalWrite(LED_BUILTIN, (millis() / 500) & 1);
  }
  // Pale warm-white palette (sunlight through leaves, not campfire).
  for (int i = 0; i < 256; i++) {
    float t = i / 255.0f;
    display.setColor(i, (uint8_t)(255 * powf(t, 0.75f)),
                        (uint8_t)(242 * powf(t, 0.82f)),
                        (uint8_t)(205 * powf(t, 1.05f)));
  }
  display.swap(false, true); // palette into both buffers

  // Soft radial sprite: smoothstep falloff → wide penumbra.
  for (int y = 0; y < SPR; y++) {
    for (int x = 0; x < SPR; x++) {
      float dx = (x - SPR * 0.5f + 0.5f) / (SPR * 0.5f);
      float dy = (y - SPR * 0.5f + 0.5f) / (SPR * 0.5f);
      float r = sqrtf(dx * dx + dy * dy);
      float v = (r >= 1.0f) ? 0.0f : (1.0f - r);
      v = v * v * (3.0f - 2.0f * v); // smoothstep
      sprite[y * SPR + x] = (uint8_t)(v * 255.0f);
    }
  }

  // Dapples: anchors spread over screen, slow non-repeating drift
  // (two incommensurate sine periods each, 20–90 s).
  randomSeed(analogRead(A0) * 65537 + micros());
  for (int i = 0; i < N_DAPPLES; i++) {
    d[i].bx = frand(30, W - 30);
    d[i].by = frand(30, H - 30);
    d[i].ax1 = frand(3, 8);  d[i].ay1 = frand(2, 6);
    d[i].ax2 = frand(1, 4);  d[i].ay2 = frand(1, 3);
    d[i].f1 = frand(0.003f, 0.010f); // 100–330 s: barely-perceptible drift
    d[i].f2 = frand(0.008f, 0.025f); // small tremble on top
    d[i].p1 = frand(0, 6.283f); d[i].p2 = frand(0, 6.283f);
    d[i].bri = frand(0.35f, 0.85f);
    d[i].bf = frand(0.010f, 0.035f); d[i].bp = frand(0, 6.283f);
  }
}

// Additive blit of the soft sprite at (cx,cy), brightness 0..256, clipped.
static void blit_add(uint8_t *buf, int cx, int cy, int bscale) {
  int x0 = cx - SPR / 2, y0 = cy - SPR / 2;
  int sx0 = max(0, -x0), sy0 = max(0, -y0);
  int sx1 = min(SPR, W - x0), sy1 = min(SPR, H - y0);
  for (int sy = sy0; sy < sy1; sy++) {
    uint8_t *row = buf + (y0 + sy) * W + x0;
    const uint8_t *srow = sprite + sy * SPR;
    for (int sx = sx0; sx < sx1; sx++) {
      int v = row[sx] + ((srow[sx] * bscale) >> 8);
      row[sx] = (v > 255) ? 255 : (uint8_t)v;
    }
  }
}

void loop() {
  float t = millis() / 1000.0f;
  // Shared "wind": a gentle common lean, gusting very slowly.
  float wind = sinf(t * 0.020f) + 0.5f * sinf(t * 0.053f + 1.7f);

  uint8_t *buf = display.getBuffer();
  memset(buf, 0, W * H);

  for (int i = 0; i < N_DAPPLES; i++) {
    float x = d[i].bx + d[i].ax1 * sinf(t * d[i].f1 * 6.283f + d[i].p1)
                      + d[i].ax2 * sinf(t * d[i].f2 * 6.283f + d[i].p2)
                      + 2.0f * wind;
    float y = d[i].by + d[i].ay1 * cosf(t * d[i].f1 * 5.1f + d[i].p2)
                      + d[i].ay2 * sinf(t * d[i].f2 * 7.3f + d[i].p1);
    // Brightness breathes; gusts also steal a little light (leaves closing).
    float b = d[i].bri * (0.55f + 0.35f * sinf(t * d[i].bf * 6.283f + d[i].bp)
                                 - 0.12f * wind * wind);
    if (b < 0.05f) b = 0.05f;
    blit_add(buf, (int)x, (int)y, (int)(b * 256));
  }
  display.swap();
}
