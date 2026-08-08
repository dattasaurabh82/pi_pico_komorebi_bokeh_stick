// controls.cpp — see controls.h for the polling-vs-interrupt rationale.
#include "controls.h"
#include "config.h"

// Full quadrature state table, index = (prevAB << 2) | currAB.
// +1/-1 = valid quarter-step, 0 = no move, 99 = impossible jump (both
// lines changed = a missed sample; harmless noise-reject here, since the
// ISR runs per edge). Same table as the shadow_puppet encoder-test.
const int8_t RotaryEncoder::QDEC[16] = {
   0, -1, +1, 99,
  +1,  0, 99, -1,
  -1, 99,  0, +1,
  99, +1, -1,  0
};

RotaryEncoder* RotaryEncoder::self = nullptr;

void RotaryEncoder::begin(uint8_t pinA, uint8_t pinB) {
  pinA_ = pinA; pinB_ = pinB;
  pinMode(pinA_, INPUT_PULLUP);  // KY-040 has its own pull-ups; harmless
  pinMode(pinB_, INPUT_PULLUP);
  prevAB_ = (digitalRead(pinA_) << 1) | digitalRead(pinB_);
  self = this;
  attachInterrupt(digitalPinToInterrupt(pinA_), isrTrampoline, CHANGE);
  attachInterrupt(digitalPinToInterrupt(pinB_), isrTrampoline, CHANGE);
}

void RotaryEncoder::isrTrampoline() { if (self) self->handleEdge(); }

void RotaryEncoder::handleEdge() {
  uint8_t ab = (digitalRead(pinA_) << 1) | digitalRead(pinB_);
  if (ab == prevAB_) return;
  int8_t d = QDEC[(prevAB_ << 2) | ab];
  if (d != 99) quarters_ += d;   // ignore impossible jumps
  prevAB_ = ab;
}

int RotaryEncoder::consumeDetents() {
  noInterrupts();
  int32_t q = quarters_;
  interrupts();
  int32_t det = q / 4;           // 4 quarter-steps per detent (KY-040)
  int out = (int)(det - taken_);
  taken_ = det;
  return out;
}

void ClickButton::begin(uint8_t pin) {
  pin_ = pin;
  pinMode(pin_, INPUT_PULLUP);
}

ClickButton::Event ClickButton::poll(uint32_t now) {
  bool down = (digitalRead(pin_) == LOW);
  Event ev = NONE;
  if (down && !wasDown_) {                   // press edge
    pressedAt_ = now;
  } else if (!down && wasDown_) {            // release edge
    uint32_t held = now - pressedAt_;
    if (held >= LONGPRESS_MS)      ev = LONG_PRESS;
    else if (held >= DEBOUNCE_MS)  ev = CLICK;
    // < DEBOUNCE_MS: contact bounce, swallow it.
  }
  wasDown_ = down;
  return ev;
}
