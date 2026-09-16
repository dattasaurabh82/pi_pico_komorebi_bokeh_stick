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

## Manual test script for the wall build (main/)

Open the serial port at 115200 (the board waits 2 s for a host at boot),
`LOG_VERBOSE 1` in config.h. Each step: what to do, what the wall does,
what the log says.

1. **Boot with the Mac attached.** Replug the board. Within ~3 s: the
   `[wifi] connected` line, three fetch lines (location, ntp, weather),
   the `[sky] ---- snapshot ----` block (time, place, sun, weather, model,
   mode, targets), then `[sky] targets (boot): ...`. Over the next 3 min
   the light slides from the encoder baseline to the sky targets; the
   `[sky] applied: ... -> wall ...` line every minute shows the creep.
2. **Surprise, single click.** Wall: breathe out, new constellation,
   breathe in. Log: `[btn] surprise: breathe out, re-deal, breathe in`.
   A click during the fade logs `ignored (fade running)`.
3. **Double-click** (second press within 350 ms). The first press still
   breathes. Log: `[btn] double-click: mode -> mirror` and a fresh snapshot
   whose targets have flipped sign (warmth +10 becomes -10, contrast +14
   becomes -14, etc.). The
   wall does not jump: the offsets slide over 3 min, so only the direction
   of drift changes. Double-click again flips back to complement.
4. **Encoder click.** Log: `[enc] click: selected breeze (announcing)`,
   wall: one gust. Density: one pool blinks. Warmth: a palette shimmer.
   Contrast: all pools tighten and brighten for a moment. Four clicks
   bring you back to warmth.
5. **Encoder turn.** Per detent: `[enc] turn +1: warmth set 54 (on the
   wall 64 incl. sky)`. "set" is the encoder value, "on the wall" adds the
   sky offset: what the palette is actually built from.
6. **Idle 30 s** with breeze or density selected: `[enc] idle 30 s:
   selection back to warmth`.
7. **Long-press the encoder** (0.6 s): `[enc] long-press: forget WiFi,
   reboot into portal`, then the portal as in the README.
8. **The sigh.** After `SKY_SIGH_MINUTES` (set it to 3 for a test):
   `[sigh] N min up: breathing out, will reboot to refresh the sky`, the
   port drops for a second, then `[boot] sigh reboot: dials restored
   (...)`, the three fetch lines, a snapshot, `radio off`. Dials, mode
   and the picture must all be back.
9. **Watchdog.** If the loop ever stalls 8 s the board reboots itself and
   the boot line says `WATCHDOG REBOOT ... last stage N` (stages listed in
   `log.h`). Seeing this line is a bug report; save the log.

Knobs while testing (config.h): `SKY_INFLUENCE` (1.0 = full swing for
tuning, 0.3 = subtle), `SKY_RANGE_*` per parameter, `SKY_SLEW_S` (180;
set 20 to watch the slide), `LOG_VERBOSE` (0 = boot, fetches, mode
flips, long-press, errors only). The look of "flat" vs "crisp" lives in
`engine.cpp` (`sizeM`, `briM`, the palette gamma multiplier).

Reading the model line: `light warmth motion foliage crisp`, each 0..1
with 0.5 = an average day. Complement pushes every offset away from the
value, mirror toward it; a 0.5 gives a zero offset in both modes.

## The "sigh": implemented (16 Sept 2026)

Status: this is the shipped design, not a fallback. The keepalive approach
was tried and lost the bisect (see the main README, "Two hard-won facts").
Interval `SKY_SIGH_MINUTES` in config.h (180). Tested 4 of 4 cycles at a
3-minute interval with the user watching: dials and mode restored, fresh
fetch each time, monitor re-synced after every dark gap.

### The problem it sidesteps

Two things this board cannot do reliably while the DVI output is running:
write flash, and keep a WiFi station link healthy over hours. Both work
fine before `display.begin()`. So instead of fighting for a live link,
the piece can do all its networking in the few seconds before the video
starts, and simply start over once an hour.

### What it looks like on the wall

Every three hours (interval in `config.h`), the light breathes out over
about a second, the wall is dark for about ten seconds (boot, WiFi join,
three fetches), and the light breathes back in with a fresh constellation. The same gesture as the
"surprise me" button, on a slow clock. On a piece that changes over
minutes, one absence per hour is part of the character, not a glitch.

### What happens underneath

1. The engine fades to black (the existing surprise fade-out).
2. Warmth, breeze, density, contrast and the complement/mirror mode are
   written to watchdog scratch registers 1 and 2. These survive
   `rp2040.reboot()` and touch no flash. Register 0 is the portal flag,
   register 3 the stage recorder.
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

### Why it is the default

Live traffic under video killed the video core four times in two days;
the radio-off run went 16.5 hours without a dropped frame. Ten seconds of
dark three times a day is the price of a board that never needs a plug
pulled.
