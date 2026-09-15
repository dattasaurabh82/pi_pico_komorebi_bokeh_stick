// config.h — every pin, limit, and tuning constant in one place.
// If you're adjusting the feel of the piece, this is the only file to touch.
#pragma once
#include <stdint.h>

// ---------- Display geometry ----------
constexpr int SCREEN_W = 320;
constexpr int SCREEN_H = 240;

// ---------- Pins (physical pin in parentheses) ----------
constexpr uint8_t PIN_BTN_SURPRISE = 7; // (10) momentary to GND, pull-up
constexpr uint8_t PIN_ENC_A        = 6; // (9)  KY-040 DT  — 3V3 module!
constexpr uint8_t PIN_ENC_B        = 8; // (11) KY-040 CLK
// NOTE: A/B vs CLK/DT assignment is arbitrary for the quadrature decoder —
// swapping them only inverts the rotation direction. If clockwise turns
// the values DOWN after wiring, negate the detents in main.ino
// (params.adjust(selected, -det)) instead of resoldering.
constexpr uint8_t PIN_ENC_SW       = 5; // (7)  encoder push, to GND (was GP9 on the RP2040 build)
// A0 (GP26) stays free-floating: entropy source. DVI Sock owns GP12-19.

// ---------- WiFi (boot-time portal, see wifi_portal.h) ----------
constexpr const char* WIFI_AP_SSID = "komorebi";
constexpr const char* WIFI_AP_PASS = "komorebi";  // WPA2, min 8 chars
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;  // stored network
constexpr uint32_t WIFI_PORTAL_TIMEOUT_MS  = 180000; // then start offline

// ---------- Sky-driven light (see README "A lamp that answers the sky") ----------
// The sky pushes each parameter away from (complement) or toward (mirror)
// the encoder setting. Full swing per parameter when the sky is extreme:
constexpr float SKY_RANGE_WARMTH   = 25.0f;  // points on 0..100
constexpr float SKY_RANGE_BREEZE   = 25.0f;  // points on 0..100
constexpr float SKY_RANGE_DENSITY  = 5.0f;   // dapples
constexpr float SKY_RANGE_CONTRAST = 25.0f;  // points on 0..100 (from cloud/rain: crispness)
constexpr float SKY_RANGE_EXPOSURE = 0.30f;  // palette gain, +-30% mid-tones
constexpr float SKY_INFLUENCE      = 1.0f;   // 0 = off, 1 = full swing. Tuning: start visible.
constexpr float SKY_SLEW_S         = 180.0f; // seconds to travel a full swing (never pops)
constexpr uint32_t DOUBLE_CLICK_MS = 350;    // surprise button: 2nd click flips complement/mirror
constexpr bool     SKY_DEFAULT_COMPLEMENT = true;

// Live network while the video runs? 1 = keepalive + hourly fetch (WiFi
// traffic under DVI, suspected of killing the video core, see tests/README).
// 0 = fetch at boot only, then radio off; sun/season stay live from the
// clock, weather ages (fresh 6 h, neutral by 12 h). Bisect setting.
#define SKY_LIVE_NET 0

// ---------- Serial logging ----------
// 1: every interaction and step (clicks, detents, values, 60 s sky line).
// 0: essentials only (boot, fetches, mode flips, errors). Never per-frame.
#define LOG_VERBOSE 1

// ---------- Dapple field ----------
constexpr int N_MAX_DAPPLES = 28;   // hard ceiling (spiked: 36 saturates)
constexpr int DENSITY_MIN   = 8;    // spiked: below this feels empty
constexpr int DENSITY_MAX   = 28;
constexpr int DENSITY_DEFAULT = 16;
constexpr int SPRITE_SIZE   = 96;   // soft radial sprite, px

// ---------- User parameters (0..100 unless noted) ----------
constexpr int WARMTH_DEFAULT = 50;  // 0 = cool daylight, 100 = low amber
constexpr int BREEZE_DEFAULT = 40;  // 0 = near-still, 100 = lively
constexpr int CONTRAST_DEFAULT = 50;// 0 = overcast flat (big, soft, dim pools), 100 = crisp sun
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
