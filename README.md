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

## Mood button (planned)

A single momentary button (GP2 to GND, internal pull-up) will cycle between
"moods" — presets that re-skin the same engine without touching its logic.

![Mood button flow](assets/mood_button.png)

Each press is debounced (~50 ms, falling edge only), advances
`mood = (mood + 1) % N_MOODS`, and applies a preset struct:

- **Palette rebuild** — color temperature and max brightness (e.g. midday
  white vs late-afternoon amber vs dim moonlight). 256 `setColor()` calls,
  effectively free.
- **Re-deal dapples** — the preset supplies the random *ranges* (dapple
  count, size distribution, drift speeds, breathing rates), then dapples are
  regenerated exactly like at boot. A "breeze" mood = livelier small
  dapples; a "still evening" = fewer, slower, dimmer.

The engine loop itself never changes — a mood is data, not code. GP2 is free
on this board (DVI uses GP12–19, entropy uses A0/GP26).

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
