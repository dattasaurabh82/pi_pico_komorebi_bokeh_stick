// pi_pico_komorebi — main.ino: thin orchestrator only.
// All tuning lives in config.h; behavior in engine/render/controls/params.
//
// Hardware: Pico 2 W + Pico DVI Sock (pico_sock_cfg, GP12-19).
//   GP7 (pin 10): "surprise me" button, to GND (pin 8 or 13).
//   GP6/8/5 (pins 9/11/7): KY-040 encoder DT/CLK/SW, GND at pin 38, VCC 3V3.
// WiFi: on boot, join the stored network or open the "komorebi" AP portal
//   (wifi_portal.h). Everything WiFi that writes flash happens BEFORE
//   display.begin(): a flash write with DVI live freezes the board.
// Interaction: encoder click cycles warmth -> breeze -> density (the
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

DVIGFX8 display(DVI_RES_320x240p60, true, pico_sock_cfg);

KomorebiEngine engine;
RotaryEncoder  encoder;
ClickButton    encoderBtn;
ClickButton    surpriseBtn;
Params         params;
WifiPortal     wifi;
SkyClient      skyc;       // sky data layer; LOG ONLY for now, engine untouched

Param    selected = Param::Warmth;
uint32_t lastInteraction = 0;

void setup() {
  SkyClient::prepareRadioForDvi();  // first: WiFi SPI divisor for the 252 MHz DVI clock
  wifi.boot(); // connect, or portal, or time out. MUST precede display.begin().
  if (wifi.connected()) {
    skyc.setCredentials(wifi.ssid(), wifi.pass());
    skyc.bootSync(10000);            // location, clock, weather. Still pre-DVI.
  }
  if (!display.begin()) { // RAM alloc failed -> blink LED forever
    pinMode(LED_BUILTIN, OUTPUT);
    for (;;) digitalWrite(LED_BUILTIN, (millis() / 500) & 1);
  }
  engine.begin(&display);
  display.swap(false, true); // palette into both buffers
  surpriseBtn.begin(PIN_BTN_SURPRISE);
  encoderBtn.begin(PIN_ENC_SW);
  encoder.begin(PIN_ENC_A, PIN_ENC_B);
}

void loop() {
  static uint32_t lastMs = 0;
  uint32_t now = millis();
  float dt = (now - lastMs) / 1000.0f;
  if (dt > 0.1f) dt = 0.1f; // first frame / hiccups
  lastMs = now;
  float t = now / 1000.0f;

  // --- inputs ---
  if (surpriseBtn.poll(now) == ClickButton::CLICK && !engine.surpriseBusy())
    engine.startSurprise();

  ClickButton::Event ev = encoderBtn.poll(now);
  if (ev == ClickButton::CLICK) {
    selected = (Param)(((uint8_t)selected + 1) % PARAM_COUNT);
    engine.announce(selected, t);   // the param shows itself on the wall
    lastInteraction = now;
  } else if (ev == ClickButton::LONG_PRESS) {
    wifi.requestPortalAndReboot();  // forget network, come back in the portal
  }

  int det = encoder.consumeDetents();
  if (det != 0) {
    params.adjust(selected, det);
    lastInteraction = now;
  }

  // Idle fallback: after a while, turning means warmth again (the most
  // lamp-like expectation for someone approaching the object cold).
  if (selected != Param::Warmth && (now - lastInteraction) > SELECT_TIMEOUT_MS)
    selected = Param::Warmth;

  // --- frame ---
  engine.renderFrame(t, dt, params);
  display.swap();

  // --- sky (log only, stage 3): hourly refresh, reconnects, status line ---
  skyc.tick();
  static uint32_t lastSkyLog = 0;
  if (now - lastSkyLog > 600000) { lastSkyLog = now; skyc.logStatus(Serial); }
}
