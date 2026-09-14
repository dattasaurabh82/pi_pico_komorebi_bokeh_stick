# Fallback: the hourly "sigh"

Status: designed, not implemented. Use only if the WiFi keepalive approach
(see the main README, "Two hard-won facts") proves unreliable on the wall.

## The problem it sidesteps

Two things this board cannot do reliably while the DVI output is running:
write flash, and keep a WiFi station link healthy over hours. Both work
fine before `display.begin()`. So instead of fighting for a live link,
the piece can do all its networking in the few seconds before the video
starts, and simply start over once an hour.

## What it looks like on the wall

Once an hour (interval in `config.h`), the light breathes out over about
a second, the wall is dark for three to five seconds, and the light
breathes back in with a fresh constellation. The same gesture as the
"surprise me" button, on a slow clock. On a piece that changes over
minutes, one absence per hour is part of the character, not a glitch.

## What happens underneath

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

## What it costs

- 3 to 5 seconds of darkness per interval.
- A new constellation each time (could be kept by also saving the RNG
  seed to a scratch register, if continuity matters more than freshness).
- Nothing else: no keepalive, no reconnect logic, no live fetch code path
  needs to be exercised at all.

## Why it is not the default

The keepalive approach keeps the light continuous, and on a network with
a decent signal at the piece it holds. The sigh is the honest answer if
it doesn't.
