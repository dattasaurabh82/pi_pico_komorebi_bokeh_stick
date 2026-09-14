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
