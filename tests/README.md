# pi_pico_komorebi: DVI and WiFi setup notes

Hardware: Raspberry Pi Pico 2 W (RP2350; a plain Pico RP2040 also works for
video) + Pico DVI Sock (Luke Wren design, pi3g/buyzero or Adafruit), HDMI out.
Toolchain: Arduino IDE / arduino-cli with the **Raspberry Pi Pico/RP2040/RP2350
(Earle Philhower) core**, the Mbed RP2040 core does NOT work with PicoDVI.
Library: PicoDVI - Adafruit Fork (1.3.2), which on RP2350 still bit-bangs
TMDS on PIO with core1 (no HSTX).

## The one code change that made video work

The `16bit_hello` example ships configured for the **Adafruit Feather RP2040
DVI**, which routes TMDS on different GPIOs. On a plain Pico with the DVI
Sock, that line must use the Sock's pin map:

```cpp
// before (example default): video goes to pins the Sock isn't on, black screen
DVIGFX16 display(DVI_RES_320x240p60, adafruit_feather_dvi_cfg);

// after: matches the DVI Sock (D0 GP12/13, CK GP14/15, D2 GP16/17, D1 GP18/19)
DVIGFX16 display(DVI_RES_320x240p60, pico_sock_cfg);
```

## Board / build settings

- Board: Raspberry Pi Pico 2 W, `rp2040:rp2040:rpipico2w` (plain Pico:
  `rp2040:rp2040:rpipico`)
- CPU speed menu: irrelevant. DVI 640x480 needs a 252 MHz TMDS bit clock and
  the library sets it inside `display.begin()` via `set_sys_clock_khz()`.
- Upload method: default (UF2). BOOTSEL volume is named `RPI-RP2` on RP2040
  and `RP2350` on RP2350.

## Spikes in this folder

- `hello_world` the first working video, DVIGFX16
- `dvigfx8_test` paletted double-buffered canvas, chosen for the piece
- `density_spike` how many dapples before additive blowout (8 to 28)
- `eeprom_dvi_spike` EEPROM.commit() with DVI live on RP2040: video core dies
- `wifi_dvi_spike` (Pico 2 W) scan + softAP + WebServer with DVI live at
  252 MHz sysclk: PASS, the fixed CYW43 PIO clock divider is fine
- `core1reset_flash_spike` (Pico 2 W) EEPROM.commit() with DVI live after
  stopping core1 (v1 `multicore_reset_core1`, v2 PSM hold): the whole board
  freezes inside the commit, USB dead. Recovery: BOOTSEL replug. This is why
  every flash write in `main/` happens before `display.begin()`.
- `sky_host` host-side tests for `main/sky_core` (`./run.sh`, clang, no
  hardware): 54 checks against real Open-Meteo / ip-api responses, broken
  inputs, sun position vs sunrise/sunset and solstices.
- `sky_spike` (Pico 2 W) the real `main/sky.*` via symlinks, DVI live,
  frame-stall meter, serial commands for failure injection, `SPIKE_MODE`
  switch (0 no DVI, 1 no DVI at 252 MHz, 2 DVI live). Found that DVI
  activity degrades the STA link and that a 15 s UDP keepalive holds it.
  See the main README, "Two hard-won facts".
- `flash.sh <sketch_dir>` compile + 1200-baud reset + wait for the RP2350
  volume + copy. Use this; arduino-cli's own upload scan gives up before
  the volume mounts on macOS.

## Failure signals

- Solid video: all good.
- Onboard LED blinking ~1 Hz: `display.begin()` failed (RAM / config).
- Dead LED + black screen: wiring / solder joints on Sock pins GP12 to GP19.
- Dark screen for up to 3 min after boot with a "komorebi" WiFi network
  visible: the setup portal is waiting (see main README).
- USB port present but every open blocks: board frozen by a flash write
  with DVI live. BOOTSEL replug.

## macOS quirks

- **arduino-cli builds fail with "exit status 1"** and a buried `ctags: cannot
  open temporary file`: the bundled ctags 5.8 chokes on macOS's per-user
  TMPDIR. Fix: prefix builds with `TMPDIR=/tmp`:

  ```bash
  TMPDIR=/tmp arduino-cli compile --fqbn rp2040:rp2040:rpipico2w --export-binaries hello_world
  ```

- **Serial upload is flaky**: the 1200-baud touch that reboots the Pico into
  BOOTSEL doesn't always enumerate cleanly on macOS. If it hangs at "Waiting
  for upload port", hold BOOTSEL while plugging in and copy the built UF2 onto
  the `RP2350` (or `RPI-RP2`) volume as `NEW.UF2`.
- **Reading the boot log**: `cat /dev/cu.usbmodem*` can block forever on a
  wedged board. A non-blocking open/read loop (python `os.open` with
  `O_NONBLOCK`) is safer; the main sketch waits 2 s for a host so the first
  lines are not lost.

## Fallback plan: the hourly "sigh"

Status: designed, not implemented. Use only if the WiFi keepalive approach
(see the main README, "Two hard-won facts") proves unreliable on the wall.

### The problem it sidesteps

Two things this board cannot do reliably while the DVI output is running:
write flash, and keep a WiFi station link healthy over hours. Both work
fine before `display.begin()`. So instead of fighting for a live link,
the piece can do all its networking in the few seconds before the video
starts, and simply start over once an hour.

### What it looks like on the wall

Once an hour (interval in `config.h`), the light breathes out over about
a second, the wall is dark for three to five seconds, and the light
breathes back in with a fresh constellation. The same gesture as the
"surprise me" button, on a slow clock. On a piece that changes over
minutes, one absence per hour is part of the character, not a glitch.

### What happens underneath

1. The engine fades to black (the existing surprise fade-out).
2. Warmth, breeze, density and the complement/mirror mode are written to
   watchdog scratch registers 1 to 3. These survive `rp2040.reboot()`
   and touch no flash. Register 0 stays reserved for the portal flag.
3. `rp2040.reboot()`.
4. Boot as today: WiFi joins the stored network, the sky layer fetches
   location, clock and weather (about 1.5 s, all before the video),
   the scratch registers restore the parameters, the video starts, the
   light fades in.

Nothing about the sky needs to persist: the model is rebuilt from the
boot fetch. If WiFi is down at that boot, the piece runs neutral until
the next sigh, exactly like an offline boot today.

### What it costs

- 3 to 5 seconds of darkness per interval.
- A new constellation each time (could be kept by also saving the RNG
  seed to a scratch register, if continuity matters more than freshness).
- Nothing else: no keepalive, no reconnect logic, no live fetch code path
  needs to be exercised at all.

### Why it is not the default

The keepalive approach keeps the light continuous, and on a network with
a decent signal at the piece it holds. The sigh is the honest answer if
it doesn't.
