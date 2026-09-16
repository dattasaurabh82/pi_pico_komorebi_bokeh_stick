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
#include "sky_core.h"

class KomorebiEngine {
public:
  void begin(DVIGFX8* display);

  // Fresh constellation: re-seeds entropy and re-deals every dapple.
  void deal();

  // "Surprise me": breathe out, deal(), breathe in. Ignored mid-fade.
  void startSurprise();
  bool surpriseBusy() const { return fadeState_ != FadeState::Idle; }
  // The sigh: breathe out and stay dark; main reboots when sighDone().
  void startSigh() { if (fadeState_ == FadeState::Idle) fadeState_ = FadeState::SighOut; }
  bool sighDone() const { return fadeState_ == FadeState::Dark; }
  // After a sigh reboot the light was dark anyway: start at the targets.
  void snapSkyToTargets() { sky_ = skyTarget_; }

  // Param selected via encoder click: play its announcement.
  void announce(Param p, float now_s);

  // Sky offsets to approach (slewed over SKY_SLEW_S, never applied at once).
  void setSkyTargets(const sky::Offsets& o) { skyTarget_ = o; }
  const sky::Offsets& skyApplied() const { return sky_; }   // what the frame used
  int effWarmth() const { return effWarmth_; }
  int effBreeze() const { return effBreeze_; }
  int effDensity() const { return effDensity_; }
  int effContrast() const { return effContrast_; }
  float effExposure() const { return 1.0f + sky_.exposure; }

  // Advance and draw one frame into the display's back buffer.
  // Call display->swap() afterwards (main owns the cadence).
  void renderFrame(float now_s, float dt_s, const Params& params);

private:
  enum class FadeState : uint8_t { Idle, Out, In, SighOut, Dark };

  struct Dapple {
    float bx, by;               // anchor
    float ax1, ay1, ax2, ay2;   // drift amplitudes (px, small!)
    float f1, f2, p1, p2;       // drift freqs (Hz, very low) + phases
    float bri, bf, bp;          // brightness base, breath freq, phase
    float sf, sp;               // shape-breathing freq + phase
    float act;                  // activation 0..1 (density melt in/out)
    int ow, oh;                 // rendered size; ow != oh = ellipticity
  };

  void buildPalette(float warmth01, float shimmer, float gain, float contrast01);
  static float slew(float cur, float target, float maxStep);
  uint32_t entropySeed();
  float frand(float lo, float hi);

  DVIGFX8* disp_ = nullptr;
  Dapple d_[N_MAX_DAPPLES];
  FadeState fadeState_ = FadeState::Idle;
  float fade_ = 1.0f;
  int lastWarmth_ = -1;         // rebuild palette only when needed (effective warmth)
  int lastGainQ_  = -1;         // quantised exposure gain, same purpose
  int lastContrast_ = -1;       // effective contrast, same purpose
  sky::Offsets skyTarget_, sky_;// sky: where we're going, where we are
  int effWarmth_ = WARMTH_DEFAULT, effBreeze_ = BREEZE_DEFAULT, effDensity_ = DENSITY_DEFAULT, effContrast_ = CONTRAST_DEFAULT;
  uint8_t palFrames_ = 2;       // frames left to (re)write the palette:
                                // DVIGFX8 has one palette PER buffer, so
                                // every change must be written twice —
                                // once into each back buffer — or the
                                // display strobes between palettes.
  Param announceParam_ = Param::Warmth;
  float announceT0_ = -1e9f;    // announcement start time (s)
};
