# pi_pico_komorebi

An ambient light installation: a Raspberry Pi Pico (RP2040) with an Adafruit
DVI Sock feeds a small pico projector, casting slowly breathing komorebi-like
light dapples (sunlight through foliage) onto a wall. Everything is generated
procedurally on the microcontroller — no video files, no computer.

Best viewed with the projector slightly defocused: the optical blur turns the
rendered falloffs into true soft penumbra.

## Hardware

- Raspberry Pi Pico (RP2040) + Adafruit DVI Sock for RP2 Pico
  (D0± = GP12/13, CK± = GP14/15, D2± = GP16/17, D1± = GP18/19)
- Any HDMI display or projector (signal is DVI 640x480@60, content 320x240)
- Planned: one momentary button, GP2 to GND, for mood switching

## Build & flash

Requires arduino-cli with the Raspberry Pi Pico/RP2040 (Earle Philhower)
core — NOT the Mbed core — and the "PicoDVI - Adafruit Fork" library.

```bash
cd main
TMPDIR=/tmp arduino-cli compile --fqbn rp2040:rp2040:rpipico:freq=250 \
  -u -p /dev/cu.usbmodem1401 .
```

`TMPDIR=/tmp` works around a macOS ctags bug in arduino-cli; the port number
may change between replugs (`arduino-cli board list`). If serial upload
hangs: hold BOOTSEL while plugging in, then drag the built UF2 onto RPI-RP2.

## How it works

The renderer (in `main/`) is built around one constraint: a Cortex-M0+ with
no FPU, with the second core fully occupied generating the DVI signal. So
everything expensive is precomputed once at boot into lookup tables, and the
per-frame work is integer adds, multiplies, and shifts.

![Frame pipeline](assets/frame_pipeline.png)

- **8-bit paletted framebuffer, double-buffered** (`DVIGFX8`, 2 x 77 KB).
  A pixel byte means "amount of light"; a 256-entry pale warm-white palette
  turns that into color. Changing the mood's color = rewriting the palette.
- **Dapples are stamps.** One 96x96 soft radial sprite (smoothstep falloff)
  is precomputed; each of the 16 dapples stamps it scaled to its own
  width/height (ellipticity) and brightness, **additively** with saturation —
  overlapping dapples merge into brighter pools, like real light.
- **Motion is sums of slow sines.** Position = anchor + two small sine
  drifts (periods 100–330 s, amplitudes a few px) + a shared "wind" lean.
  Brightness and shape breathe on their own slow sines. Dapples stay
  anchored and breathe — they never travel. Position is a pure function of
  time: nothing accumulates, nothing can drift off.
- **Fresh constellation every power-up.** 16 stirred floating-ADC reads seed
  the RNG; anchors, sizes, periods and phases are re-dealt each boot while
  the overall character stays fixed.
- **Vignette pass** multiplies every pixel by an edge falloff (two 1-D
  tables, `vig = vigx[x] * vigy[y]`), forcing hard black at all four raster
  edges — the wall shows floating light, not a projected rectangle.
- **`swap()`** waits for vsync and flips buffers; no tearing, no half-drawn
  frames.

## The "surprise me" button

One momentary button between **GP6 (physical pin 9) and GND (physical pin
8)** — two adjacent pins, no resistor needed (internal pull-up). Enclosure
label: *surprise me*.

![Surprise button flow](assets/mood_button.png)

A press never hard-cuts. The light breathes out (~0.8 s eased fade to
black), the RNG is re-seeded from 16 stirred floating-ADC reads, all 16
dapples are re-dealt (new anchors, sizes, speeds, phases), and the light
breathes back in (~1.2 s). Same character, fresh constellation — the same
thing a power cycle does, minus the wait, plus the grace.

Debounce is 50 ms on the falling edge, and presses are ignored while a
fade is already running.

Mood *presets* (palette temperature + parameter ranges as data) remain an
easy future addition on the same engine, e.g. on long-press — see
`context` notes.

## The encoder: warmth / breeze / density

A KY-040 rotary encoder (A=GP7, B=GP8, SW=GP9 — physical pins 10/11/12,
GND at pin 13, VCC to **3V3, never 5V**) carries the three adjustable
parameters. Since the piece has no display, the interaction grammar is:

- **Click** cycles the selected parameter: warmth → breeze → density.
  The selected parameter *announces itself* on the wall in its own
  language — warmth: a brief palette shimmer; breeze: a single gust;
  density: one dapple blinks out and back.
- **Turn** adjusts the selected parameter, hard-capped to safe ranges
  (warmth 0–100, breeze 0–100, density 8–28 — limits chosen by visual
  spike tests; see `tests/density_spike`).
- After 30 s without interaction, selection falls back to warmth (the
  most lamp-like expectation). Long-press is reserved for a future
  fade-to-off.

Density changes never re-deal the field: all 28 dapples always exist and
melt in/out of visibility, so turning the knob feels continuous.

The encoder is read via **pin-change interrupts** (full quadrature
table), not polling — the render loop runs at 60 Hz (vsync-locked), far
too slow to poll a fast twirl. Settings are session-only by design:
writing the Pico's flash while PicoDVI runs kills the video core
(proven in `tests/eeprom_dvi_spike`); if persistence is ever needed, use
an external I2C FRAM/EEPROM — see context notes.

## Code layout (main/)

```
main.ino      thin orchestrator: inputs → engine → swap
config.h      every pin, limit, and tuning constant
engine.*      KomorebiEngine: dapples, palette, fades, announcements
render.*      fx:: sprite/vignette tables, additive scaled blit
controls.*    RotaryEncoder (ISR quadrature) + ClickButton grammar
params.h      the three user values + capped adjustment
```

## Repo layout

```
main/            current installation sketch (build this)
komorebi/        variant A — first anchored-dapple baseline (frozen)
komorebi_noise/  variant C — morphing noise-field style (frozen reference)
tests/           proven hardware/library experiments + setup notes
assets/          diagrams (SVG sources + rendered PNGs)
```

Variant history and comparisons: see commit log. A/C are kept diffable
against `main/` on purpose.
