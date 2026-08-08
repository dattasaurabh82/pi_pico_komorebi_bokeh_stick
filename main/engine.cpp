// engine.cpp — implementation. Sections: setup/deal, palette, surprise
// fade, announcements, and the per-frame render.
#include <Arduino.h>
#include "engine.h"
#include "render.h"

// ---------- randomness ----------

float KomorebiEngine::frand(float lo, float hi) {
  return lo + (hi - lo) * (random(10000) / 10000.0f);
}

// Stronger boot entropy than a single ADC read: stir 16 floating-pin
// samples with time. Gives a genuinely fresh constellation per press
// and per power cycle.
uint32_t KomorebiEngine::entropySeed() {
  uint32_t s = micros();
  for (int i = 0; i < 16; i++) {
    s = (s << 3) ^ (s >> 5) ^ (uint32_t)analogRead(A0) ^ (s * 2654435761u);
    delayMicroseconds(37 + (s & 63));
  }
  return s;
}

// ---------- lifecycle ----------

void KomorebiEngine::begin(DVIGFX8* display) {
  disp_ = display;
  fx::init();
  randomSeed(entropySeed());
  deal();
  // Every dapple starts fully melted-in; density gating happens per frame.
  for (int i = 0; i < N_MAX_DAPPLES; i++) d_[i].act = 1.0f;
}

// Deal all N_MAX dapples in three size classes (proportions from the
// 16-dapple original: 3 large pools / 5 mid / 8 small sparkles).
// Density only decides how many are SHOWN — dealt state covers the max.
void KomorebiEngine::deal() {
  const int nLarge = (N_MAX_DAPPLES * 3 + 8) / 16;
  const int nMidEnd = nLarge + (N_MAX_DAPPLES * 5 + 8) / 16;
  for (int i = 0; i < N_MAX_DAPPLES; i++) {
    float base;
    if (i < nLarge)       base = frand(110, 150);
    else if (i < nMidEnd) base = frand(65, 95);
    else                  base = frand(30, 50);
    float ecc = frand(-0.22f, 0.22f);         // squash one axis a bit
    d_[i].ow = (int)(base * (1.0f + ecc));
    d_[i].oh = (int)(base * (1.0f - ecc));
    d_[i].bx = frand(20, SCREEN_W - 20);
    d_[i].by = frand(20, SCREEN_H - 20);
    d_[i].ax1 = frand(3, 8);  d_[i].ay1 = frand(2, 6);
    d_[i].ax2 = frand(1, 4);  d_[i].ay2 = frand(1, 3);
    d_[i].f1 = frand(0.003f, 0.010f);         // 100-330 s: barely drifts
    d_[i].f2 = frand(0.008f, 0.025f);         // small tremble on top
    d_[i].p1 = frand(0, 6.283f); d_[i].p2 = frand(0, 6.283f);
    if (i < nMidEnd) { d_[i].bri = frand(0.40f, 0.80f); d_[i].bf = frand(0.010f, 0.030f); }
    else             { d_[i].bri = frand(0.25f, 0.55f); d_[i].bf = frand(0.025f, 0.060f); }
    d_[i].bp = frand(0, 6.283f);
    d_[i].sf = frand(0.006f, 0.020f);         // shape morph: 50-170 s
    d_[i].sp = frand(0, 6.283f);
    // act is NOT reset here: melt state carries across deals so a
    // surprise mid-density-change doesn't pop dapples.
  }
}

// ---------- palette ----------
// warmth01 blends between a cool near-white and a low amber by lerping
// the per-channel gain and gamma. shimmer (-1..+1) is the announcement:
// it briefly pushes warmth around its set point so the user SEES which
// parameter they just selected.
void KomorebiEngine::buildPalette(float warmth01, float shimmer) {
  float w = warmth01 + shimmer * 0.20f;
  if (w < 0) w = 0; if (w > 1) w = 1;
  // endpoints: cool (w=0) .. warm (w=1)
  float rG = 255.0f,                 rE = 0.82f + w * (0.55f - 0.82f);
  float gG = 250.0f + w * (205.0f - 250.0f), gE = 0.84f + w * (0.88f - 0.84f);
  float bG = 240.0f + w * (110.0f - 240.0f), bE = 0.92f + w * (1.70f - 0.92f);
  for (int i = 0; i < 256; i++) {
    float t = i / 255.0f;
    disp_->setColor(i, (uint8_t)(rG * powf(t, rE)),
                       (uint8_t)(gG * powf(t, gE)),
                       (uint8_t)(bG * powf(t, bE)));
  }
}

// ---------- surprise + announcements ----------

void KomorebiEngine::startSurprise() {
  if (fadeState_ == FadeState::Idle) fadeState_ = FadeState::Out;
}

void KomorebiEngine::announce(Param p, float now_s) {
  announceParam_ = p;
  announceT0_ = now_s;
}

// ---------- per-frame ----------

void KomorebiEngine::renderFrame(float t, float dt, const Params& params) {
  // Surprise fade machine: breathe out -> fresh deal -> breathe in.
  if (fadeState_ == FadeState::Out) {
    fade_ -= dt / FADE_OUT_S;
    if (fade_ <= 0.0f) {
      fade_ = 0.0f;
      randomSeed(entropySeed());
      deal();
      fadeState_ = FadeState::In;
    }
  } else if (fadeState_ == FadeState::In) {
    fade_ += dt / FADE_IN_S;
    if (fade_ >= 1.0f) { fade_ = 1.0f; fadeState_ = FadeState::Idle; }
  }
  float fEased = fade_ * fade_ * (3.0f - 2.0f * fade_); // smooth breathe

  // Announcement envelope: 1 at click, decaying to 0 over ANNOUNCE_S.
  float aU = (t - announceT0_) / ANNOUNCE_S;
  float aEnv = (aU >= 0.0f && aU < 1.0f) ? (1.0f - aU) : 0.0f;

  // Palette: rebuild when warmth changed, or animate the warmth shimmer.
  bool shimmering = (aEnv > 0.0f) && (announceParam_ == Param::Warmth);
  if (params.warmth != lastWarmth_ || shimmering) {
    float shimmer = shimmering ? sinf(aU * 6.283f * 1.5f) * aEnv : 0.0f;
    buildPalette(params.warmth / 100.0f, shimmer);
    lastWarmth_ = params.warmth;
  }

  // Breeze scaling: one scalar drives amplitudes, wind, and breath rate.
  float b01 = params.breeze / 100.0f;
  float amp = 0.3f + 2.2f * b01;      // positional sway multiplier
  float rate = 0.6f + 1.2f * b01;     // breathing tempo multiplier
  // Breeze announcement = one extra gust pushed through the wind term.
  float gust = (announceParam_ == Param::Breeze) ? aEnv * 1.6f : 0.0f;
  float wind = (sinf(t * 0.020f) + 0.5f * sinf(t * 0.053f + 1.7f)) * amp
               + gust * sinf(aU * 3.14159f);

  uint8_t* buf = disp_->getBuffer();
  memset(buf, 0, SCREEN_W * SCREEN_H);

  for (int i = 0; i < N_MAX_DAPPLES; i++) {
    Dapple& dp = d_[i];
    // Density melt: ramp activation toward shown/hidden, render if lit.
    float target = (i < params.density) ? 1.0f : 0.0f;
    if (dp.act < target)      { dp.act += dt * ACT_RAMP_PER_S; if (dp.act > 1) dp.act = 1; }
    else if (dp.act > target) { dp.act -= dt * ACT_RAMP_PER_S; if (dp.act < 0) dp.act = 0; }
    if (dp.act <= 0.01f) continue;

    // Sway scaled by breeze (amp); breath tempo scaled by breeze (rate).
    float x = dp.bx + (dp.ax1 * sinf(t * dp.f1 * 6.283f + dp.p1)
                     + dp.ax2 * sinf(t * dp.f2 * 6.283f + dp.p2)) * amp
                    + 2.0f * wind;
    float y = dp.by + (dp.ay1 * cosf(t * dp.f1 * 5.1f + dp.p2)
                     + dp.ay2 * sinf(t * dp.f2 * 7.3f + dp.p1)) * amp;
    float br = dp.bri * (0.55f + 0.35f * sinf(t * dp.bf * 6.283f * rate + dp.bp)
                                - 0.12f * wind * wind * 0.25f);
    if (br < 0.05f) br = 0.05f;

    // Density announcement: the first dapple blinks out and back once.
    float blink = 1.0f;
    if (announceParam_ == Param::Density && aEnv > 0.0f && i == 0) {
      blink = 1.0f - sinf(aU * 3.14159f); // dip to 0 mid-announcement
    }

    // Shape breathing: slow area-preserving squash/stretch.
    float m = 1.0f + 0.06f * sinf(t * dp.sf * 6.283f + dp.sp);
    fx::blitAddScaled(buf, (int)x, (int)y,
                      (int)(br * fEased * dp.act * blink * 256),
                      (int)(dp.ow * m), (int)(dp.oh / m));
  }
  fx::vignettePass(buf);
}
