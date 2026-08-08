// render.h — pixel-level primitives shared by the engine: the soft
// radial sprite, the edge vignette, and the additive scaled blit.
// Everything here is precomputed-table + integer math: the Cortex-M0+
// has no FPU and core 1 is fully occupied generating the DVI signal.
#pragma once
#include <stdint.h>

namespace fx {

// Build the lookup tables. Call once in setup(), before any rendering.
void init();

// Additive blit of the soft sprite, resampled to ow x oh px, centered at
// (cx, cy), scaled by bscale (0..256 = 0..1.0 brightness), clipped to the
// screen, saturating at 255. Overlapping dapples merge into brighter
// pools — like real light — instead of overwriting each other.
void blitAddScaled(uint8_t* buf, int cx, int cy, int bscale, int ow, int oh);

// Multiply the whole frame by the edge falloff so no light ever reaches
// the raster border: on the wall this dissolves the projection rectangle.
// Separable (vig = vigx[x] * vigy[y]) — two 1-D tables instead of a 75 KB
// 2-D mask, because the two framebuffers already eat most of the RAM.
void vignettePass(uint8_t* buf);

} // namespace fx
