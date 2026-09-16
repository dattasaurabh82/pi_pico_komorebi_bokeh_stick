// pi_pico_komorebi — main.ino: thin orchestrator only.
// All tuning lives in config.h; behavior in engine/render/controls/params.
//
// Hardware: Pico 2 W + Pico DVI Sock (pico_sock_cfg, GP12-19).
//   GP7 (pin 10): "surprise me" button, to GND (pin 8 or 13).
//   GP6/8/5 (pins 9/11/7): KY-040 encoder DT/CLK/SW, GND at pin 38, VCC 3V3.
// WiFi: on boot, join the stored network or open the "komorebi" AP portal
//   (wifi_portal.h). Everything WiFi that writes flash happens BEFORE
//   display.begin(): a flash write with DVI live freezes the board.
// Interaction: encoder click cycles warmth -> breeze -> density -> contrast (the
// selected parameter announces itself visually); turning adjusts it.
// Selection falls back to warmth after SELECT_TIMEOUT_MS of inactivity.
// Encoder long-press: forget the WiFi network, reboot into the portal.

#include <PicoDVI.h>
#include "config.h"
#include "params.h"
#include "controls.h"
#include "engine.h"
#include "wifi_portal.h"
#include "sky.h"
#include "log.h"

// Subclass only to read the video core's late-scanline counter (protected).
struct DVIGFX8Probe : public DVIGFX8 {
  using DVIGFX8::DVIGFX8;
  unsigned lateScanlines() const { return dvi0.late_scanline_ctr; }
};
DVIGFX8Probe display(DVI_RES_320x240p60, true, pico_sock_cfg);

KomorebiEngine engine;
RotaryEncoder  encoder;
ClickButton    encoderBtn;
ClickButton    surpriseBtn;
Params         params;
WifiPortal     wifi;
SkyClient      skyc;       // sky data layer
bool           skyComplement = SKY_DEFAULT_COMPLEMENT;   // false = mirror
uint32_t       lastSurpriseClick = 0;

Param    selected = Param::Warmth;
uint32_t lastInteraction = 0;

// Dials and mode survive a sigh reboot in watchdog scratch registers
// ([0] portal flag, [3] stage recorder; [1] dials, [2] magic + mode).
static constexpr uint32_t SIGH_MAGIC = 0x53494700;   // "SIG" + mode bit
static void saveStateForReboot() {
  watchdog_hw->scratch[1] = ((uint32_t)params.warmth << 24) | ((uint32_t)params.breeze << 16) |
                            ((uint32_t)params.density << 8) | (uint32_t)params.contrast;
  watchdog_hw->scratch[2] = SIGH_MAGIC | (skyComplement ? 1 : 0);
}
static bool restoreStateAfterReboot() {
  if ((watchdog_hw->scratch[2] & 0xFFFFFF00) != SIGH_MAGIC) return false;
  uint32_t d = watchdog_hw->scratch[1];
  params.warmth = (d >> 24) & 0xFF; params.breeze = (d >> 16) & 0xFF;
  params.density = (d >> 8) & 0xFF; params.contrast = d & 0xFF;
  skyComplement = watchdog_hw->scratch[2] & 1;
  watchdog_hw->scratch[2] = 0;
  return true;
}

static const char* modeName() { return skyComplement ? "complement" : "mirror"; }
static sky::Offsets skyOffsets() {
  sky::OffsetRanges r = { SKY_RANGE_WARMTH, SKY_RANGE_BREEZE, SKY_RANGE_DENSITY, SKY_RANGE_EXPOSURE, SKY_RANGE_CONTRAST };
  return sky::mapOffsets(skyc.vector(), skyComplement ? -1 : +1, SKY_INFLUENCE, r);
}
static void pushSky(const char* why) {        // recompute targets, hand to engine, say so
  STAGE(11);
  sky::Offsets o = skyOffsets();
  engine.setSkyTargets(o);
  LOGV("[sky] targets (%s): warmth %+.0f breeze %+.0f density %+.1f exposure %+.0f%% contrast %+.0f\n",
       why, o.warmth, o.breeze, o.density, o.exposure * 100.0f, o.contrast);
}

void setup() {
  SkyClient::prepareRadioForDvi();  // first: WiFi SPI divisor for the 252 MHz DVI clock
  bool wdReboot = watchdog_caused_reboot();
  uint32_t lastStage = watchdog_hw->scratch[3];
  STAGE(0);
  wifi.boot(); // connect, or portal, or time out. MUST precede display.begin().
  bool sighReboot = restoreStateAfterReboot();
  if (sighReboot)   LOGE("[boot] sigh reboot: dials restored (warmth %d breeze %d density %d contrast %d, %s)\n",
                         params.warmth, params.breeze, params.density, params.contrast, modeName());
  else if (wdReboot) LOGE("[boot] WATCHDOG REBOOT: the loop stalled >8 s, last stage %lu (see log.h)\n", (unsigned long)lastStage);
  else               LOGE("[boot] power-on / normal reset\n");
  if (wifi.connected()) {
    skyc.setCredentials(wifi.ssid(), wifi.pass());
    skyc.bootSync(10000);            // location, clock, weather. Still pre-DVI.
  }
  STAGE(10); skyc.dumpSnapshot(Serial, modeName(), skyOffsets());
  STAGE(11); pushSky("boot");        // engine slides from baseline to these over SKY_SLEW_S
  if (sighReboot) engine.snapSkyToTargets();   // it was dark anyway: no 3-min drift after a sigh
  STAGE(0);
#if !SKY_LIVE_NET
  if (wifi.connected()) { WiFi.disconnect(); WiFi.mode(WIFI_OFF); LOGE("[sky] radio off (SKY_LIVE_NET 0): boot data only\n"); }
#endif
  if (!display.begin()) { // RAM alloc failed -> blink LED forever
    pinMode(LED_BUILTIN, OUTPUT);
    for (;;) digitalWrite(LED_BUILTIN, (millis() / 500) & 1);
  }
  engine.begin(&display);
  display.swap(false, true); // palette into both buffers
  rp2040.wdt_begin(8000);    // from here on: a loop stall > 8 s reboots the board
  surpriseBtn.begin(PIN_BTN_SURPRISE);
  encoderBtn.begin(PIN_ENC_SW);
  encoder.begin(PIN_ENC_A, PIN_ENC_B);
}

void loop() {
  rp2040.wdt_reset();
  static uint32_t lastMs = 0;
  uint32_t now = millis();
  float dt = (now - lastMs) / 1000.0f;
  if (dt > 0.1f) dt = 0.1f; // first frame / hiccups
  lastMs = now;
  float t = now / 1000.0f;

  // --- inputs ---
  STAGE(13);
  if (surpriseBtn.poll(now) == ClickButton::CLICK) {
    if (now - lastSurpriseClick < DOUBLE_CLICK_MS) {   // 2nd click: flip the world
      skyComplement = !skyComplement;
      LOGE("[btn] double-click: mode -> %s\n", modeName());
      pushSky("mode flip");
      STAGE(10); skyc.dumpSnapshot(Serial, modeName(), skyOffsets()); STAGE(0);
    } else if (!engine.surpriseBusy()) {
      LOGV("[btn] surprise: breathe out, re-deal, breathe in\n");
      engine.startSurprise();
    } else {
      LOGV("[btn] surprise click ignored (fade running)\n");
    }
    lastSurpriseClick = now;
    lastInteraction = now;
  }

  static const char* PNAME[4] = { "warmth", "breeze", "density", "contrast" };
  ClickButton::Event ev = encoderBtn.poll(now);
  if (ev == ClickButton::CLICK) {
    selected = (Param)(((uint8_t)selected + 1) % PARAM_COUNT);
    engine.announce(selected, t);   // the param shows itself on the wall
    LOGV("[enc] click: selected %s (announcing)\n", PNAME[(uint8_t)selected]);
    lastInteraction = now;
  } else if (ev == ClickButton::LONG_PRESS) {
    LOGE("[enc] long-press: forget WiFi, reboot into portal\n");
    wifi.requestPortalAndReboot();  // forget network, come back in the portal
  }

  int det = encoder.consumeDetents();
  if (det != 0) {
    params.adjust(selected, det);
    int v = selected == Param::Warmth ? params.warmth : selected == Param::Breeze ? params.breeze : selected == Param::Density ? params.density : params.contrast;
    int e = selected == Param::Warmth ? engine.effWarmth() : selected == Param::Breeze ? engine.effBreeze() : selected == Param::Density ? engine.effDensity() : engine.effContrast();
    LOGV("[enc] turn %+d: %s set %d (on the wall %d incl. sky)\n", det, PNAME[(uint8_t)selected], v, e);
    lastInteraction = now;
  }

  // Idle fallback: after a while, turning means warmth again (the most
  // lamp-like expectation for someone approaching the object cold).
  if (selected != Param::Warmth && (now - lastInteraction) > SELECT_TIMEOUT_MS) {
    selected = Param::Warmth;
    LOGV("[enc] idle %lu s: selection back to warmth\n", SELECT_TIMEOUT_MS / 1000);
  }

  // --- frame ---
  STAGE(12);
  engine.renderFrame(t, dt, params);
  STAGE(14);
  display.swap();
  STAGE(0);

  // --- sky: hourly refresh, reconnects; targets re-pushed every minute
  //     (the sun moves) and right after a successful fetch ---
  uint32_t fetchesBefore = skyc.fetchCount();
#if SKY_LIVE_NET
  skyc.tick();
#endif
  static uint32_t lastSkyPush = 0;
  if (skyc.fetchCount() != fetchesBefore) {
    pushSky("fetch");
    STAGE(10); skyc.dumpSnapshot(Serial, modeName(), skyOffsets()); STAGE(0);
    lastSkyPush = now;
  } else if (now - lastSkyPush > 60000) {
    lastSkyPush = now;
    pushSky("minute");
    const sky::Offsets& a = engine.skyApplied();
    LOGV("[sky] applied: warmth %+.1f breeze %+.1f density %+.1f exposure %+.0f%% contrast %+.1f -> wall warmth %d breeze %d density %d contrast %d | dvi late %u\n",
         a.warmth, a.breeze, a.density, a.exposure * 100.0f, a.contrast, engine.effWarmth(), engine.effBreeze(), engine.effDensity(), engine.effContrast(),
         display.lateScanlines());
  }
  static uint32_t lastSkyLog = 0;
  if (now - lastSkyLog > 600000) { lastSkyLog = now; skyc.logStatus(Serial); }

  // --- the sigh: breathe out, reboot with dials kept, fetch fresh sky pre-DVI ---
  if (SKY_SIGH_MINUTES && now > SKY_SIGH_MINUTES * 60000UL && !engine.surpriseBusy() && !engine.sighDone()) {
    LOGE("[sigh] %lu min up: breathing out, will reboot to refresh the sky\n", now / 60000UL);
    engine.startSigh();
  }
  if (engine.sighDone()) {
    saveStateForReboot();
    Serial.flush();
    delay(100);
    rp2040.reboot();
  }
}
