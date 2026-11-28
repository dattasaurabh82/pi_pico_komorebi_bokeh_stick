// ir_dvi_spike: does sending IR from core 0 disturb the DVI video on core 1?
//
// Hardware: Pico 2 W + Pico DVI Sock (pico_sock_cfg) on a MONITOR, IR LED on
// GP21 (pin 27) -> 150R -> LED -> GND. Unplug the projector's power for this
// test: the sketch sends every key, Power included.
//
// Libraries: PicoDVI - Adafruit Fork, IRremote (tested with 4.7.1).
//
// What it does:
//   - video exactly like main/: DVIGFX8 320x240 double-buffered, 252 MHz
//   - a bar sweeps across the screen and a frame counter ticks, so a stall
//     or a black flash is visible on the monitor
//   - first 15 s: no IR (baseline), then one key every 3 s until 5 min:
//     Power, Source, Up, OK, round and round (about 95 sends), then a summary
//   - after each send, one serial line (115200):
//       send number, key, how long the send took,
//       frames since the last send and the frame rate,
//       longest frame time (expected ~70-90 ms on a send: core 0 is busy
//       sending, the video core keeps showing the last frame),
//       late scanlines from the video core (the real health number:
//       should stay 0 or not grow)
//   - watchdog 8 s: if the video core dies, swap() blocks, the board reboots
//     and the next boot prints "WATCHDOG REBOOT"
//
// Pass: late scanlines do not grow on sends, frame rate stays ~60 between
// sends, the monitor never flickers or goes black, no watchdog reboot.

#include <PicoDVI.h>
#define NO_LED_FEEDBACK_CODE // the Pico 2 W LED sits on the WiFi chip
#include <IRremote.hpp>
#include "hardware/gpio.h"
#include "hardware/watchdog.h"

// Subclass only to read the video core's late-scanline counter (as in main/).
struct DVIGFX8Probe : public DVIGFX8 {
  using DVIGFX8::DVIGFX8;
  unsigned lateScanlines() const { return dvi0.late_scanline_ctr; }
};
DVIGFX8Probe display(DVI_RES_320x240p60, true, pico_sock_cfg);

static const uint8_t PIN_IR_SEND = 21;
static const uint16_t NEC_ADDRESS = 0x00;
struct Key { uint8_t cmd; const char *name; };
static const Key KEYS[] = {
  {0xA8, "Power"}, {0x97, "Source"}, {0x95, "Up"}, {0x9E, "OK"},
};
static const uint8_t N_KEYS = sizeof(KEYS) / sizeof(KEYS[0]);

static const uint32_t BASELINE_MS = 15000;
static const uint32_t SEND_EVERY_MS = 3000;
static const uint32_t TEST_END_MS = 300000; // 5 min, then a summary and no more sends

static uint32_t frames = 0, framesAtLastReport = 0;
static uint32_t lastSwapUs = 0, maxFrameUs = 0, lastReportMs = 0;
static uint32_t lastSendMs = 0, sends = 0;
static unsigned lateAtLastReport = 0;
static bool baselineReported = false, testDone = false;

static void report(const char *what, uint32_t sendUs) {
  uint32_t now = millis();
  uint32_t n = frames - framesAtLastReport;
  float fps = n * 1000.0f / (now - lastReportMs);
  unsigned late = display.lateScanlines();
  Serial.printf("[ir-dvi] %8lu ms  %-8s send %5.1f ms  frames %4lu  %5.1f fps"
                "  max frame %5.1f ms  late scanlines +%u (total %u)\n",
                (unsigned long)now, what, sendUs / 1000.0f, (unsigned long)n, fps,
                maxFrameUs / 1000.0f, late - lateAtLastReport, late);
  framesAtLastReport = frames;
  lateAtLastReport = late;
  lastReportMs = now;
  maxFrameUs = 0;
}

void setup() {
  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 3000) delay(10);
  Serial.println();
  if (watchdog_caused_reboot())
    Serial.println(F("[ir-dvi] WATCHDOG REBOOT: the loop stalled > 8 s, video core likely dead"));

  if (!display.begin()) {
    pinMode(LED_BUILTIN, OUTPUT);
    for (;;) digitalWrite(LED_BUILTIN, (millis() / 500) & 1);
  }
  for (int i = 0; i < 256; i++) display.setColor(i, i, i, i); // grey ramp
  display.swap(false, true);                                // both palettes

  // After display.begin(): IRremote times the carrier with micros(), so the
  // 252 MHz clock change does not matter, but start it on the final clock.
  IrSender.begin(PIN_IR_SEND);
  gpio_set_drive_strength(PIN_IR_SEND, GPIO_DRIVE_STRENGTH_12MA);

  Serial.printf("[ir-dvi] video on, sysclk %lu MHz, IR LED on GP21\n",
                (unsigned long)(clock_get_hz(clk_sys) / 1000000));
  Serial.printf("[ir-dvi] %lu s baseline without IR, then one key every %lu s\n",
                (unsigned long)(BASELINE_MS / 1000), (unsigned long)(SEND_EVERY_MS / 1000));
  rp2040.wdt_begin(8000);
  lastReportMs = millis();
  lastSwapUs = micros();
}

void loop() {
  rp2040.wdt_reset();
  uint32_t now = millis();

  if (!baselineReported && now >= BASELINE_MS) {
    report("baseline", 0);
    baselineReported = true;
    lastSendMs = now;
  }
  if (baselineReported && !testDone && now >= TEST_END_MS) {
    testDone = true;
    Serial.printf("[ir-dvi] DONE after %lu sends, late scanlines total %u, video still running\n",
                  (unsigned long)sends, display.lateScanlines());
  }
  if (baselineReported && !testDone && now - lastSendMs >= SEND_EVERY_MS) {
    const Key &k = KEYS[sends % N_KEYS];
    uint32_t s0 = micros();
    IrSender.sendNEC(NEC_ADDRESS, k.cmd, 0);
    uint32_t sendUs = micros() - s0;
    sends++;
    lastSendMs = millis();
    report(k.name, sendUs);
  }

  // test picture: sweeping bar, frame counter
  display.fillScreen(16);
  int x = (frames * 2) % 320;
  display.fillRect(x, 60, 24, 120, 255);
  display.setTextColor(255);
  display.setTextSize(2);
  display.setCursor(8, 8);
  display.printf("frame %lu", (unsigned long)frames);
  display.setCursor(8, 216);
  display.printf("IR sends %lu", (unsigned long)sends);
  display.swap();

  uint32_t t = micros();
  uint32_t dt = t - lastSwapUs;
  lastSwapUs = t;
  if (dt > maxFrameUs) maxFrameUs = dt;
  frames++;
}
