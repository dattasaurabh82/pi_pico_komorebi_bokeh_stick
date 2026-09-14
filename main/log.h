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
