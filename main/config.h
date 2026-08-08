// config.h — every pin, limit, and tuning constant in one place.
// If you're adjusting the feel of the piece, this is the only file to touch.
#pragma once
#include <stdint.h>

// ---------- Display geometry ----------
constexpr int SCREEN_W = 320;
constexpr int SCREEN_H = 240;

// ---------- Pins (physical pin in parentheses) ----------
constexpr uint8_t PIN_BTN_SURPRISE = 6; // (9)  momentary to GND, pull-up
constexpr uint8_t PIN_ENC_A        = 7; // (10) KY-040 A/CLK — 3V3 module!
constexpr uint8_t PIN_ENC_B        = 8; // (11) KY-040 B/DT
constexpr uint8_t PIN_ENC_SW       = 9; // (12) encoder push, to GND
// A0 (GP26) stays free-floating: entropy source. DVI Sock owns GP12-19.

// ---------- Dapple field ----------
constexpr int N_MAX_DAPPLES = 28;   // hard ceiling (spiked: 36 saturates)
constexpr int DENSITY_MIN   = 8;    // spiked: below this feels empty
constexpr int DENSITY_MAX   = 28;
constexpr int DENSITY_DEFAULT = 16;
constexpr int SPRITE_SIZE   = 96;   // soft radial sprite, px

// ---------- User parameters (0..100 unless noted) ----------
constexpr int WARMTH_DEFAULT = 50;  // 0 = cool daylight, 100 = low amber
constexpr int BREEZE_DEFAULT = 40;  // 0 = near-still, 100 = lively
constexpr int PARAM_STEP     = 4;   // warmth/breeze change per detent
// density moves 1 per detent (its own scale)

// ---------- Timing ----------
constexpr float FADE_OUT_S = 0.8f;  // surprise: breathe out
constexpr float FADE_IN_S  = 1.2f;  // surprise: breathe in
constexpr uint32_t DEBOUNCE_MS   = 50;
constexpr uint32_t LONGPRESS_MS  = 600;   // reserved (future: off)
constexpr uint32_t SELECT_TIMEOUT_MS = 30000; // selection falls back to warmth
constexpr float ANNOUNCE_S = 0.9f;  // param self-announcement duration
constexpr float ACT_RAMP_PER_S = 1.2f; // dapple activation fade (density)

// ---------- Vignette (frame-dissolve) ----------
constexpr int VIGNETTE_MARGIN_X = 72; // px of falloff at left/right
constexpr int VIGNETTE_MARGIN_Y = 58; // px of falloff at top/bottom
