// TEST 1: DVIGFX8 double-buffer sanity check on Pico + DVI Sock.
// Success = smooth orbiting warm blob, no flicker. Failure = blinking LED.

#include <PicoDVI.h>

// 320x240, 8-bit paletted, double-buffered (arg 2 = true), DVI Sock pins.
DVIGFX8 display(DVI_RES_320x240p60, true, pico_sock_cfg);

void setup() {
  if (!display.begin()) { // Insufficient RAM → blink LED forever
    pinMode(LED_BUILTIN, OUTPUT);
    for (;;) digitalWrite(LED_BUILTIN, (millis() / 500) & 1);
  }
  // Warm-light gradient palette: 0=black → amber → warm white.
  for (int i = 0; i < 256; i++) {
    float t = i / 255.0f;
    uint8_t r = (uint8_t)(255 * powf(t, 0.55f));
    uint8_t g = (uint8_t)(200 * powf(t, 0.85f));
    uint8_t b = (uint8_t)(120 * powf(t, 1.6f));
    display.setColor(i, r, g, b);
  }
  display.swap(false, true); // copy palette to both buffers
}

void loop() {
  float t = millis() / 1000.0f;
  display.fillScreen(0);
  // One soft blob: concentric circles stepping down palette = fake radial falloff
  int cx = 160 + (int)(90 * sinf(t * 0.5f));
  int cy = 120 + (int)(60 * cosf(t * 0.37f));
  for (int r = 60; r > 0; r -= 4) {
    uint8_t c = (uint8_t)(255 - r * 4); // brighter toward center
    display.fillCircle(cx, cy, r, c);
  }
  // Corner marker squares to spot tearing/flicker at edges
  display.fillRect(0, 0, 8, 8, 255);
  display.fillRect(312, 232, 8, 8, 255);
  display.swap(); // flip buffers
}
