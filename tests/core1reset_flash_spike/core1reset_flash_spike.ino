// SPIKE B v2: write flash while PicoDVI is live by stopping core1 first,
// then rebooting. v1 (multicore_reset_core1 + EEPROM.commit) HUNG the
// board at cycle 1 with USB CDC dead (host open() blocked) - no serial
// evidence of where. Suspect: multicore_reset_core1() ends with
// multicore_fifo_pop_blocking() waiting for core1's bootrom handshake.
// v2: hold core1 in reset via PSM (no handshake, we reboot anyway) and
// print a stage marker before every step so the last line locates a hang.
//
// Self-driving: each boot reads a counter from EEPROM. If counter < CYCLES:
// DVI runs 5 s (ball moves), then stop core1, counter++, commit, reboot.
// At counter == CYCLES it stops: CYCLES squares + a bar, Serial: SURVIVED.
// Re-run: bump SPIKE_MAGIC.
#include <PicoDVI.h>
#include <EEPROM.h>
#include "hardware/structs/psm.h"

#define SPIKE_MAGIC 0xB0000002u
#define CYCLES 5

DVIGFX8 display(DVI_RES_320x240p60, true, pico_sock_cfg);

struct Rec { uint32_t magic; uint32_t count; uint32_t check; };
static Rec rec;
static bool done = false;
static uint32_t bootMs = 0;

static uint32_t checksum(const Rec& r) { return r.magic ^ (r.count * 2654435761u); }

static void stage(const char* s) { Serial.printf("[B] %s\n", s); Serial.flush(); delay(300); }

// Hold core1 in reset. We never bring it back: a reboot follows.
static void haltCore1() {
  hw_set_bits(&psm_hw->frce_off, PSM_FRCE_OFF_PROC1_BITS);
  while (!(psm_hw->frce_off & PSM_FRCE_OFF_PROC1_BITS)) tight_loop_contents();
}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(256);
  EEPROM.get(0, rec);
  if (rec.magic != SPIKE_MAGIC || rec.check != checksum(rec)) {
    rec = { SPIKE_MAGIC, 0, 0 };
    rec.check = checksum(rec);
  }
  done = rec.count >= CYCLES;

  if (!display.begin()) {
    pinMode(LED_BUILTIN, OUTPUT);
    for (;;) digitalWrite(LED_BUILTIN, (millis() / 500) & 1);
  }
  for (int i = 0; i < 256; i++) display.setColor(i, i, i, i);
  display.swap(false, true);
  bootMs = millis();
  delay(1500);
  Serial.printf("[B] boot, count=%lu %s\n", rec.count, done ? "SURVIVED (all cycles ok)" : "");
}

void loop() {
  uint32_t now = millis();
  float t = now / 1000.0f;
  display.fillScreen(0);
  int cx = 160 + (int)(100 * sinf(t * 1.3f));
  display.fillCircle(cx, 130, 30, 200);
  for (uint32_t i = 0; i < rec.count; i++)
    display.fillRect(10 + i * 18, 10, 12, 12, 255);   // one per survived cycle
  if (done) display.fillRect(10, 220, 300, 8, 255);   // finished
  display.swap();

  if (!done && now - bootMs > 5000) {
    Serial.printf("[B] cycle %lu\n", rec.count + 1);
    stage("s1 halt core1");
    haltCore1();                       // DVI dies here, on purpose
    stage("s2 core1 halted, put");
    rec.count++;
    rec.check = checksum(rec);
    EEPROM.put(0, rec);
    stage("s3 commit");
    bool ok = EEPROM.commit();
    stage(ok ? "s4 commit ok, reboot" : "s4 commit FAILED, reboot");
    rp2040.reboot();
  }
}
