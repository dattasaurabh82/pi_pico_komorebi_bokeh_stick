#!/bin/sh
# flash.sh <sketch_dir> : compile for the Pico 2 W, reset it into BOOTSEL via
# the 1200-baud touch, wait for the RP2350 volume (arduino-cli's own scan gives
# up too early on macOS), copy the UF2. Falls back to an already-mounted volume.
set -e
SKETCH="${1:-.}"
cd "$SKETCH"
TMPDIR=/tmp arduino-cli compile --fqbn rp2040:rp2040:rpipico2w --export-binaries . 2>&1 | grep -E "error|Sketch uses" || true
UF2=$(ls build/rp2040.rp2040.rpipico2w/*.uf2 | head -1)
[ -f "$UF2" ] || { echo "no uf2 built"; exit 1; }
if [ ! -d /Volumes/RP2350 ]; then
  PORT=$(ls /dev/cu.usbmodem* 2>/dev/null | head -1)
  [ -n "$PORT" ] || { echo "no serial port and no RP2350 volume: hold BOOTSEL and replug"; exit 1; }
  python3 - "$PORT" <<'EOF'
import os, sys, time, termios, fcntl
fd = os.open(sys.argv[1], os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
a = termios.tcgetattr(fd); a[4] = a[5] = termios.B1200; termios.tcsetattr(fd, termios.TCSANOW, a)
try: fcntl.ioctl(fd, 0x8004746b, (0x002).to_bytes(4, 'little'))   # clear DTR
except Exception: pass
os.close(fd)
for i in range(40):
    time.sleep(0.5)
    if os.path.isdir("/Volumes/RP2350"): sys.exit(0)
print("RP2350 volume did not appear"); sys.exit(1)
EOF
fi
sleep 0.5
cp "$UF2" /Volumes/RP2350/NEW.UF2 && echo "flashed $(basename "$UF2") at $(date +%T)"
