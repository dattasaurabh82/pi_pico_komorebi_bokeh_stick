// ir_send: replay the YG300 remote codes from an IR LED on the Pico.
//
// Hardware: Pico 2 W.
//   GP21 (pin 27) -> 1k -> NPN base (BC337 / 2N2222), emitter -> GND (pin 28),
//   collector -> IR LED cathode, LED anode -> 33R -> VSYS (pin 39). ~100 mA pulses.
//   Quick test without transistor: GP21 -> 150R -> LED anode, cathode -> GND.
// The receiver on GP22 from ir_capture can stay wired, it is not used here.
//
// Library: IRremote by shirriff, z3t0, ArminJo (tested with 4.7.1).
//
// On boot: wait 5 s, send Power, wait 10 s, send Power again.
// Serial 115200, one key per command, any time:
//   p Power   s Source   u Up   d Down   l Left   r Right
//   o OK      m Menu     x Exit a run the boot sequence again
//
// Codes captured with tests/ir_capture on 2026-11-28: NEC, address 0x00.

// No LED feedback code at all: on the Pico 2 W the LED sits on the WiFi chip.
#define NO_LED_FEEDBACK_CODE
#include <IRremote.hpp>
#include "hardware/gpio.h"

static const uint8_t PIN_IR_SEND = 21;
static const uint16_t NEC_ADDRESS = 0x00;

struct Key { char c; uint8_t cmd; const char *name; };
static const Key KEYS[] = {
  {'p', 0xA8, "Power"}, {'s', 0x97, "Source"}, {'u', 0x95, "Up"},
  {'d', 0x9A, "Down"},  {'l', 0x9B, "Left"},   {'r', 0x99, "Right"},
  {'o', 0x9E, "OK"},    {'m', 0x91, "Menu"},   {'x', 0xA4, "Exit"},
};
static const uint8_t POWER = 0xA8;

static const uint32_t BOOT_WAIT_MS = 5000;
static const uint32_t ON_TIME_MS = 10000;

static void send(uint8_t cmd, const char *name) {
  IrSender.sendNEC(NEC_ADDRESS, cmd, 0); // one frame, no repeats
  Serial.printf("[ir] %8lu ms  sent %-6s (NEC 0x%02X 0x%02X)\n",
                (unsigned long)millis(), name, NEC_ADDRESS, cmd);
}

static void handleSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == 'a') { bootSequence(); continue; }
    for (const Key &k : KEYS)
      if (k.c == c) send(k.cmd, k.name);
  }
}

// Waits while still answering serial keys.
static void waitMs(uint32_t ms) {
  uint32_t t0 = millis();
  while (millis() - t0 < ms) { handleSerial(); delay(10); }
}

void bootSequence() {
  Serial.printf("[ir] sequence: wait %lu s, Power, wait %lu s, Power\n",
                BOOT_WAIT_MS / 1000, ON_TIME_MS / 1000);
  waitMs(BOOT_WAIT_MS);
  send(POWER, "Power");
  Serial.println(F("[ir] projector should be turning on"));
  waitMs(ON_TIME_MS);
  send(POWER, "Power");
  Serial.println(F("[ir] projector should be turning off (or asking to confirm)"));
  Serial.println(F("[ir] sequence done, keys: p s u d l r o m x a"));
}

void setup() {
  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 3000) delay(10);
  IrSender.begin(PIN_IR_SEND);
  // Strongest pin drive, for the quick test without transistor.
  gpio_set_drive_strength(PIN_IR_SEND, GPIO_DRIVE_STRENGTH_12MA);
  Serial.println();
  Serial.println(F("[ir] ir_send ready, IR LED on GP21"));
  bootSequence();
}

void loop() { handleSerial(); delay(10); }
