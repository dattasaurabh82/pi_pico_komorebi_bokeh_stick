// SPIKE: does EEPROM.commit() (flash write) coexist with PicoDVI on core1?
// Animation must keep running; each commit adds one square to the top row.
// Camera check after ~20s: moving ball + 3-4 squares = SURVIVED.

#include <PicoDVI.h>
#include <EEPROM.h>

DVIGFX8 display(DVI_RES_320x240p60, true, pico_sock_cfg);

static int commits = 0;
static uint32_t last_commit = 0;

void setup() {
  if (!display.begin()) {
    pinMode(LED_BUILTIN, OUTPUT);
    for (;;) digitalWrite(LED_BUILTIN, (millis() / 500) & 1);
  }
  for (int i = 0; i < 256; i++) display.setColor(i, i, i, i); // gray ramp
  display.swap(false, true);
  EEPROM.begin(256);
}

void loop() {
  uint32_t now = millis();
  float t = now / 1000.0f;
  display.fillScreen(0);
  // moving ball proves the animation is alive
  int cx = 160 + (int)(100 * sinf(t * 1.3f));
  display.fillCircle(cx, 140, 30, 200);
  // one 12px square per successful commit
  for (int i = 0; i < commits; i++)
    display.fillRect(10 + i * 18, 10, 12, 12, 255);

  if (now - last_commit > 5000 && commits < 12) {
    last_commit = now;
    EEPROM.put(0, commits + 1000); // dummy payload, changes every time
    EEPROM.commit();               // THE test: flash write w/ DVI live
    commits++;
  }
  display.swap();
}
