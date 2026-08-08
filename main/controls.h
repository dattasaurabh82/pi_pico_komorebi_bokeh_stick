// controls.h — physical inputs: quadrature encoder (interrupt-driven)
// and debounced push buttons with click/long grammar.
//
// WHY INTERRUPTS for the encoder: the render loop runs at ~60 Hz because
// display.swap() blocks on vsync. Polling A/B at 16 ms intervals would
// miss steps on any fast turn (the "illegal transitions" the
// shadow_puppet encoder-test was built to detect). The ISR does the same
// quadrature table walk at wire speed instead.
#pragma once
#include <Arduino.h>

class RotaryEncoder {
public:
  // Registers CHANGE interrupts on both pins. One instance only
  // (single static self — this installation has exactly one encoder).
  void begin(uint8_t pinA, uint8_t pinB);
  // Detents accumulated since the last call (+cw / -ccw), then cleared.
  int consumeDetents();

private:
  static void isrTrampoline();      // ISRs can't take instance methods
  void handleEdge();                // the actual table walk
  static RotaryEncoder* self;
  static const int8_t QDEC[16];     // quadrature decode (see .cpp)
  uint8_t pinA_ = 0, pinB_ = 0;
  volatile int32_t quarters_ = 0;   // 4 quarter-steps per detent
  volatile uint8_t prevAB_ = 0;
  int32_t taken_ = 0;               // detents already handed out
};

// Polled push button (per-frame poll is plenty for human clicks).
// Reports one event per press-release cycle.
class ClickButton {
public:
  enum Event : uint8_t { NONE, CLICK, LONG_PRESS };
  void begin(uint8_t pin);          // INPUT_PULLUP, active LOW
  Event poll(uint32_t now_ms);

private:
  uint8_t pin_ = 0;
  bool wasDown_ = false;
  uint32_t pressedAt_ = 0;
};
