// SPIKE: BH1750 ambient light sensor on the Pico 2 W, on its own.
// No DVI, no WiFi. Two questions only: is the sensor on the bus, and what
// does it read when we move it around?
//
// Wiring (assets/wiring.png):
//   VCC -> 3V3 OUT (pin 36)   GND -> GND (pin 3)
//   SDA -> GP2 (pin 4)        SCL -> GP3 (pin 5)      ADDR open
// GP2/GP3 are I2C1, so this is Wire1. Wire (I2C0) will not work on them.
//
// Nothing to install: Wire ships with the Pico core, and the sensor needs
// one command byte and a two-byte read.
//
// Serial 115200. At boot: a scan of the bus. Then one line per reading:
// lux, raw count, min and max seen so far, a bar on a log scale.
// Commands:  s  scan again     r  reset min / max
#include <Wire.h>

#define PIN_BH_SDA    2       // GP2, physical pin 4, I2C1 SDA
#define PIN_BH_SCL    3       // GP3, physical pin 5, I2C1 SCL
#define READ_EVERY_MS 500     // one reading takes the sensor 120 ms (180 max)
#define RESCAN_MS     2000    // while no sensor answers

#define BH_ADDR_LOW   0x23    // ADDR open or low
#define BH_ADDR_HIGH  0x5C    // ADDR tied to VCC
#define BH_POWER_ON   0x01
#define BH_CONT_HRES  0x10    // continuous, 1 lx steps
#define BH_MAX_LUX    54612.5f // 65535 counts / 1.2

static uint8_t  addr = 0;     // 0 = no sensor found yet
static float    luxMin = BH_MAX_LUX, luxMax = 0;
static uint32_t lastReadMs = 0, lastScanMs = 0;

static bool sendCmd(uint8_t cmd) {
  Wire1.beginTransmission(addr);
  Wire1.write(cmd);
  return Wire1.endTransmission() == 0;
}

// Print every address that answers. If one of them is a BH1750 address,
// start it in continuous mode.
static void scan() {
  addr = 0;
  int found = 0;
  Serial.print("[i2c] scan on GP2 / GP3:");
  for (uint8_t a = 0x08; a < 0x78; a++) {
    Wire1.beginTransmission(a);
    if (Wire1.endTransmission() == 0) {
      Serial.printf(" 0x%02X", a);
      found++;
      if (a == BH_ADDR_LOW || a == BH_ADDR_HIGH) addr = a;
    }
  }
  Serial.println(found ? "" : " nothing answers");
  if (!addr) {
    Serial.println("[bh1750] not found. Check VCC on pin 36, GND on pin 3, SDA on pin 4, SCL on pin 5.");
    return;
  }
  if (!sendCmd(BH_POWER_ON) || !sendCmd(BH_CONT_HRES)) {
    Serial.printf("[bh1750] 0x%02X answered the scan but refused the start command\n", addr);
    addr = 0;
    return;
  }
  Serial.printf("[bh1750] found at 0x%02X, continuous mode, 1 lx steps\n", addr);
  delay(180);                 // first reading is ready after this
}

void setup() {
  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 3000) delay(10);   // wait for a host, 3 s at most
  Serial.println("\n[boot] bh1750_spike");

  bool pinsOk = Wire1.setSDA(PIN_BH_SDA) && Wire1.setSCL(PIN_BH_SCL);
  if (!pinsOk) Serial.println("[i2c] the core refused GP2 / GP3 for Wire1");
  Wire1.begin();              // 100 kHz, 25 ms timeout: a missing sensor cannot hang the loop
  lastScanMs = millis();
  scan();
}

void loop() {
  uint32_t now = millis();

  if (Serial.available()) {
    char c = Serial.read();
    if (c == 's') { lastScanMs = now; scan(); }
    if (c == 'r') { luxMin = BH_MAX_LUX; luxMax = 0; Serial.println("[bh1750] min / max reset"); }
  }

  if (!addr) {                // no sensor: keep looking, so a replug is picked up
    if (now - lastScanMs >= RESCAN_MS) { lastScanMs = now; scan(); }
    return;
  }
  if (now - lastReadMs < READ_EVERY_MS) return;
  lastReadMs = now;

  if (Wire1.requestFrom(addr, (size_t)2) != 2) {
    Serial.println("[bh1750] read failed, sensor gone? Scanning again.");
    addr = 0;
    lastScanMs = now;
    return;
  }
  uint8_t  hi  = Wire1.read();
  uint8_t  lo  = Wire1.read();
  uint16_t raw = ((uint16_t)hi << 8) | lo;
  float    lux = raw / 1.2f;
  if (lux < luxMin) luxMin = lux;
  if (lux > luxMax) luxMax = lux;

  char bar[31];               // 30 steps from 0 lx to the sensor's top, log scale
  int n = (int)(log10f(lux + 1.0f) / log10f(BH_MAX_LUX + 1.0f) * 30.0f + 0.5f);
  for (int i = 0; i < 30; i++) bar[i] = i < n ? '#' : ' ';
  bar[30] = 0;

  Serial.printf("[bh1750] %8.1f lx  raw %5u  min %8.1f  max %8.1f  |%s|%s\n",
                lux, raw, luxMin, luxMax, bar, raw == 0xFFFF ? " SATURATED" : "");
}
