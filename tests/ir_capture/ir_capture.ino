// ir_capture: read the YG300 remote, print what each key sends.
//
// Hardware: Pico 2 W, IR receiver module (VS1838B type, metal cage):
//   OUT -> GP22 (pin 29), GND -> GND, VCC -> 3V3 OUT (pin 36), not 5 V.
// No DVI, no WiFi: this sketch only listens.
//
// Library: IRremote by shirriff, z3t0, ArminJo (tested with 4.7.1),
// installed from the Arduino IDE Library Manager.
//
// Serial 115200. For every frame it prints one summary line (protocol,
// address, command, repeat flag), the IRremote send call that would replay
// it, and the raw mark/space timings in microseconds (for replaying a
// protocol IRremote does not know).

#define RAW_BUFFER_LENGTH 200 // room for long frames, NEC needs 68
#include <IRremote.hpp>

static const uint8_t PIN_IR_RECEIVE = 22;
static uint32_t frames = 0;

void setup() {
  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 3000) delay(10);
  // LED feedback off: on the Pico 2 W the LED sits on the WiFi chip,
  // which must not be touched from the receive interrupt.
  IrReceiver.begin(PIN_IR_RECEIVE, DISABLE_LED_FEEDBACK);
  Serial.println();
  Serial.println(F("[ir] ir_capture ready, receiver on GP22"));
  Serial.print(F("[ir] protocols: "));
  printActiveIRProtocols(&Serial);
  Serial.println();
  Serial.println(F("[ir] point the remote at the receiver and press a key"));
}

void loop() {
  if (!IrReceiver.decode()) return;
  frames++;
  Serial.print(F("\n[ir] frame "));
  Serial.println(frames);
  IrReceiver.printIRResultShort(&Serial);
  IrReceiver.printIRSendUsage(&Serial);
  IrReceiver.printIRResultRawFormatted(&Serial, true);
  IrReceiver.resume();
}
