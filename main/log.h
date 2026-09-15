// log.h — two levels of serial output, switched in config.h (LOG_VERBOSE).
// LOGE: essential, always. LOGV: verbose, only when LOG_VERBOSE is 1.
#pragma once
#include <Arduino.h>
#include "config.h"
#define LOGE(...) Serial.printf(__VA_ARGS__)
#if LOG_VERBOSE
#define LOGV(...) Serial.printf(__VA_ARGS__)
#else
#define LOGV(...) do {} while (0)
#endif

// Black-box recorder: a stage id in watchdog scratch[3] (survives a
// watchdog reboot, touches no flash). Set before risky steps; read at boot.
// 0 idle, 1 fetch connect, 2 fetch send, 3 fetch reading, 4 fetch parse,
// 5 reassociate disconnect, 6 reassociate begin, 7 keepalive,
// 10 snapshot dump, 11 push targets, 12 render, 13 inputs.
#include "hardware/watchdog.h"
#define STAGE(n) (watchdog_hw->scratch[3] = (n))
