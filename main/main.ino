// pi_pico_komorebi — main.ino: thin orchestrator only.
// All tuning lives in config.h; behavior in engine/render/controls/params.
//
// Hardware: Pico + Adafruit DVI Sock (pico_sock_cfg, GP12-19).
//   GP6 (pin 9):  "surprise me" button, to GND (pin 8).
//   GP7/8/9 (pins 10/11/12): KY-040 encoder A/B/SW, GND at pin 13, VCC 3V3.
// Interaction: encoder click cycles warmth -> breeze -> density (the
// selected parameter announces itself visually); turning adjusts it.
// Selection falls back to warmth after SELECT_TIMEOUT_MS of inactivity.

#include <PicoDVI.h>
#include "config.h"
#include "params.h"
#include "controls.h"
#include "engine.h"

DVIGFX8 display(DVI_RES_320x240p60, true, pico_sock_cfg);

KomorebiEngine engine;
RotaryEncoder  encoder;
ClickButton    encoderBtn;
ClickButton    surpriseBtn;
Params         params;

Param    selected = Param::Warmth;
uint32_t lastInteraction = 0;

void setup() {
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
  }
  // ev == LONG_PRESS: reserved (future: fade-to-off), deliberately unused.

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
}
