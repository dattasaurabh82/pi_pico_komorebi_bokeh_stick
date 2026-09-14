// wifi_portal.h — boot-time WiFi: stored credentials or a captive portal.
//
// HARD RULE (spiked 2026-09-14, tests/core1reset_flash_spike): flash writes
// with PicoDVI running freeze the whole board, even with core1 stopped.
// So everything here runs in setup() BEFORE display.begin(), and the only
// way to reconfigure later is a reboot into the portal, signalled through a
// watchdog scratch register (survives rp2040.reboot(), touches no flash).
#pragma once
#include <Arduino.h>

class WifiPortal {
public:
  // Full boot sequence. Blocks until one of:
  //   connected to stored network            -> returns true
  //   portal timed out, nobody configured    -> returns false (run offline)
  //   portal saved new credentials           -> writes EEPROM, reboots
  bool boot();

  // From the running art (encoder long-press): forget the stored network
  // and come back up in the portal. Never returns.
  void requestPortalAndReboot();

  bool connected() const { return connected_; }
  const char* ssid() const { return creds_.ssid; }   // stored network (empty if none)
  const char* pass() const { return creds_.pass; }

  // Stored record (EEPROM, last 4 KB flash sector). Public because the
  // portal's HTTP handlers (file-static) fill it in.
  struct Creds { uint32_t magic; char ssid[33]; char pass[65]; uint32_t check; };
  static uint32_t checksum(const Creds& c);

private:
  bool load(Creds& c);
  void save(const Creds& c);           // EEPROM write: pre-DVI only
  bool tryConnect(const Creds& c);
  bool runPortal();                    // true = saved (then reboots), false = timeout
  bool connected_ = false;
  Creds creds_ = {};
};
