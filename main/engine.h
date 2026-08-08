// engine.h — the komorebi itself: the dapple field, the warm palette,
// the surprise fade, and the parameter self-announcements.
//
// Design notes (why it looks the way it does):
// - Motion is sums of slow sines: position is a pure function of time,
//   so nothing accumulates or drifts off. Dapples stay ANCHORED and
//   breathe; they never travel (user-locked aesthetic, see context/).
// - Density changes never re-deal: all N_MAX_DAPPLES exist, and each has
//   an activation level ramping toward on/off — dapples melt in and out.
// - A "mood announcement" is how a selected parameter identifies itself
//   on a display-less object: warmth shimmers the palette, breeze sends
//   one gust, density blinks one dapple.
#pragma once
#include <stdint.h>
#include <PicoDVI.h>
#include "config.h"
#include "params.h"

class KomorebiEngine {
public:
  void begin(DVIGFX8* display);

  // Fresh constellation: re-seeds entropy and re-deals every dapple.
  void deal();

  // "Surprise me": breathe out, deal(), breathe in. Ignored mid-fade.
  void startSurprise();
  bool surpriseBusy() const { return fadeState_ != FadeState::Idle; }

  // Param selected via encoder click: play its announcement.
  void announce(Param p, float now_s);

  // Advance and draw one frame into the display's back buffer.
  // Call display->swap() afterwards (main owns the cadence).
  void renderFrame(float now_s, float dt_s, const Params& params);

private:
  enum class FadeState : uint8_t { Idle, Out, In };

  struct Dapple {
    float bx, by;               // anchor
    float ax1, ay1, ax2, ay2;   // drift amplitudes (px, small!)
    float f1, f2, p1, p2;       // drift freqs (Hz, very low) + phases
    float bri, bf, bp;          // brightness base, breath freq, phase
    float sf, sp;               // shape-breathing freq + phase
    float act;                  // activation 0..1 (density melt in/out)
    int ow, oh;                 // rendered size; ow != oh = ellipticity
  };

  void buildPalette(float warmth01, float shimmer);
  uint32_t entropySeed();
  float frand(float lo, float hi);

  DVIGFX8* disp_ = nullptr;
  Dapple d_[N_MAX_DAPPLES];
  FadeState fadeState_ = FadeState::Idle;
  float fade_ = 1.0f;
  int lastWarmth_ = -1;         // rebuild palette only when needed
  Param announceParam_ = Param::Warmth;
  float announceT0_ = -1e9f;    // announcement start time (s)
};
