# pi_pico_komorebi — DVI setup notes

Hardware: Raspberry Pi Pico (RP2040) + Adafruit DVI Sock, HDMI out.
Toolchain: Arduino IDE / arduino-cli with the **Raspberry Pi Pico/RP2040 (Earle Philhower) core** — the Mbed RP2040 core does NOT work with PicoDVI.
Library: PicoDVI - Adafruit Fork (1.3.2).

## The one code change that made video work

The `16bit_hello` example ships configured for the **Adafruit Feather RP2040 DVI**, which routes TMDS on different GPIOs. On a plain Pico with the DVI Sock, that line must use the Sock's pin map:

```cpp
// before (example default) — video goes to pins the Sock isn't on → black screen
DVIGFX16 display(DVI_RES_320x240p60, adafruit_feather_dvi_cfg);

// after — matches the DVI Sock (D0± = GP12/13, CK± = GP14/15, D2± = GP16/17, D1± = GP18/19)
DVIGFX16 display(DVI_RES_320x240p60, pico_sock_cfg);
```

## Board / build settings

| Setting | Value |
|---|---|
| Board | Raspberry Pi Pico (Philhower core, `rp2040:rp2040:rpipico`) |
| CPU Speed | 250 MHz (Overclock) — see clock note below |
| Upload Method | Default (UF2) |

Clock note: DVI 640×480 needs a 252 MHz TMDS bit clock. The library sets this itself inside `display.begin()` via `set_sys_clock_khz()`, so the menu value isn't what makes video work — but keeping it at 250 keeps `F_CPU`/delay/baud math close to reality.

## Failure signals

- Solid video → all good.
- Onboard LED blinking ~1 Hz → `display.begin()` failed (RAM / config).
- Dead LED + black screen → wiring / solder joints on Sock pins GP12–GP19.

## macOS quirks

- **arduino-cli builds fail with "exit status 1"** and a buried `ctags: cannot open temporary file` — the bundled ctags 5.8 chokes on macOS's per-user TMPDIR. Fix: prefix builds with `TMPDIR=/tmp`:

  ```bash
  TMPDIR=/tmp arduino-cli compile --fqbn rp2040:rp2040:rpipico:freq=250 --export-binaries hello_world
  ```

- **Serial upload is flaky**: the 1200-baud touch that reboots the Pico into BOOTSEL doesn't always enumerate cleanly on macOS. IDE upload usually works; if it hangs at "Waiting for upload port", hold BOOTSEL while plugging in and drag `hello_world/build/rp2040.rp2040.rpipico/hello_world.ino.uf2` onto the `RPI-RP2` volume.
