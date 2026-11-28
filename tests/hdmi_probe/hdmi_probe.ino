// hdmi_probe: why do some YG300 projectors show "no signal" for the Pico?
//
// Hardware: Pico 2 W + Pico DVI Sock (pico_sock_cfg, GP12-19), no WiFi.
// Shows a test card (white border, colour bars, mode name, a moving block)
// in one of several output modes, and lets the serial port switch the mode
// and the TMDS pin drive strength.
//
// Serial, 115200, one character per command:
//   0..5  switch output mode (the board reboots into it, the port drops ~2 s)
//   d     next drive strength (2, 4, 8, 12 mA), applied live, no reboot
//   ?     print the current state
//
// Modes (all DVI, no HDMI data islands, as PicoDVI always does):
//   0  640x480 60 Hz   252 MHz sysclk   what komorebi sends today
//   1  800x480 60 Hz   295 MHz sysclk
//   2  800x480 30 Hz   148 MHz sysclk
//   3  1280x720 30 Hz  319 MHz sysclk   reduced blanking, may crash the board
//
// The mode survives the reboot in watchdog scratch register 0 (no flash
// write). If a mode crashes the board, the watchdog reboots it and the
// probe falls back to mode 0 and says so on serial.

#include <PicoDVI.h>
#include "hardware/watchdog.h"
#include "hardware/gpio.h"

struct ProbeMode {
  const char *name;
  const dvi_timing *t;
  uint8_t vrep;      // vertical line repeat, keeps the canvas small
  vreg_voltage v;
};

// Not const on purpose: keeps the timings in RAM like libdvi's own (__dvi_const).
// CEA-861 VIC 17: 720x576p 50 Hz. H 720+12+64+68 = 864, V 576+5+5+39 = 625.
static dvi_timing timing_720x576p_50hz = {
  .h_sync_polarity = false, .h_front_porch = 12, .h_sync_width = 64,
  .h_back_porch = 68, .h_active_pixels = 720,
  .v_sync_polarity = false, .v_front_porch = 5, .v_sync_width = 5,
  .v_back_porch = 39, .v_active_lines = 576,
  .bit_clk_khz = 270000};

// CEA-861 VIC 3: 720x480p 59.94 Hz. H 720+16+62+60 = 858, V 480+9+6+30 = 525.
static dvi_timing timing_720x480p_60hz = {
  .h_sync_polarity = false, .h_front_porch = 16, .h_sync_width = 62,
  .h_back_porch = 60, .h_active_pixels = 720,
  .v_sync_polarity = false, .v_front_porch = 9, .v_sync_width = 6,
  .v_back_porch = 30, .v_active_lines = 480,
  .bit_clk_khz = 270000};

static const ProbeMode MODES[] = {
  {"640x480 60Hz", &dvi_timing_640x480p_60hz, 2, VREG_VOLTAGE_1_20},
  {"800x480 60Hz", &dvi_timing_800x480p_60hz, 2, VREG_VOLTAGE_1_20},
  {"800x480 30Hz", &dvi_timing_800x480p_30hz, 2, VREG_VOLTAGE_1_20},
  {"1280x720 30Hz", &dvi_timing_1280x720p_reduced_30hz, 4, VREG_VOLTAGE_1_30},
  {"720x576 50Hz", &timing_720x576p_50hz, 2, VREG_VOLTAGE_1_20},
  {"720x480 60Hz", &timing_720x480p_60hz, 2, VREG_VOLTAGE_1_20},
};
static const uint8_t N_MODES = sizeof(MODES) / sizeof(MODES[0]);

static const uint32_t MAGIC_MODE = 0xC0DE5A00; // scratch[0]: magic | mode
static const uint32_t MAGIC_INTENT = 0x1A7E5700; // scratch[1]: our own reboot
static const uint8_t TMDS_FIRST_PIN = 12, TMDS_LAST_PIN = 19;

static const gpio_drive_strength DRIVES[] = {
  GPIO_DRIVE_STRENGTH_2MA, GPIO_DRIVE_STRENGTH_4MA,
  GPIO_DRIVE_STRENGTH_8MA, GPIO_DRIVE_STRENGTH_12MA};
static const uint8_t DRIVE_MA[] = {2, 4, 8, 12};

// A 16-bit framebuffer at half width and 1/vrep height of any libdvi timing.
// Same scheme as DVIGFX16 in the library, which only takes a fixed mode list.
class ProbeDVI : public PicoDVI, public GFXcanvas16 {
public:
  ProbeDVI(const ProbeMode &m)
      : PicoDVI(*m.t, pico_sock_cfg, m.v),
        GFXcanvas16(m.t->h_active_pixels / 2, m.t->v_active_lines / m.vrep) {
    dvi_vertical_repeat = m.vrep;
    dvi_monochrome_tmds = false;
  }
  bool start();
  void nextLine();
private:
  uint16_t line_ = 2;
};

static ProbeDVI *probe = nullptr;
static void lineCallback() { probe->nextLine(); }

void ProbeDVI::nextLine() {
  uint16_t *buf;
  while (queue_try_remove_u32(&dvi0.q_colour_free, &buf))
    ;
  buf = &getBuffer()[WIDTH * line_];
  queue_add_blocking_u32(&dvi0.q_colour_valid, &buf);
  line_ = (line_ + 1) % HEIGHT;
}

bool ProbeDVI::start() {
  uint16_t *buf = getBuffer();
  if (!buf) return false;
  mainloop = dvi_scanbuf_main_16bpp;
  dvi0.scanline_callback = lineCallback;
  PicoDVI::begin();
  queue_add_blocking_u32(&dvi0.q_colour_valid, &buf);
  buf += WIDTH;
  queue_add_blocking_u32(&dvi0.q_colour_valid, &buf);
  wait_begin = false;
  return true;
}

static uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

static uint8_t mode = 0, drive = 0;
static int8_t crashedMode = -1;
static int16_t blockX = 0;

static void applyDrive() {
  for (uint8_t p = TMDS_FIRST_PIN; p <= TMDS_LAST_PIN; p++)
    gpio_set_drive_strength(p, DRIVES[drive]);
}

static void drawCard() {
  ProbeDVI &d = *probe;
  int16_t w = d.width(), h = d.height();
  d.fillScreen(0);
  d.drawRect(0, 0, w, h, 0xFFFF);
  d.drawRect(2, 2, w - 4, h - 4, 0xFFFF);
  static const uint16_t bars[] = {0xFFFF, 0xFFE0, 0x07FF, 0x07E0,
                                  0xF81F, 0xF800, 0x001F};
  int16_t bw = (w - 8) / 7;
  for (uint8_t i = 0; i < 7; i++)
    d.fillRect(4 + i * bw, 4, bw, h / 3, bars[i]);
  uint8_t s = w >= 600 ? 4 : 3;
  d.setTextColor(0xFFFF);
  d.setTextSize(s);
  d.setCursor(8, h / 3 + 10);
  d.printf("MODE %u", mode);
  d.setTextSize(s - 1);
  d.setCursor(8, h / 3 + 10 + 9 * s);
  d.print(MODES[mode].name);
  d.setCursor(8, h / 3 + 10 + 9 * s + 9 * (s - 1));
  d.setTextColor(rgb(255, 200, 120));
  d.printf("drive %u mA", DRIVE_MA[drive]);
}

static void printState() {
  const dvi_timing &t = *MODES[mode].t;
  Serial.printf("[probe] mode %u: %s, sysclk %lu MHz, pixel clock %.2f MHz, "
                "hsync %s, vsync %s, drive %u mA\n",
                mode, MODES[mode].name, t.bit_clk_khz / 1000,
                t.bit_clk_khz / 10000.0f,
                t.h_sync_polarity ? "+" : "-", t.v_sync_polarity ? "+" : "-",
                DRIVE_MA[drive]);
  Serial.println("[probe] keys: 0..5 mode (reboots), d next drive strength, ? state");
}

void setup() {
  uint32_t s0 = watchdog_hw->scratch[0], s1 = watchdog_hw->scratch[1];
  bool ours = (s1 & 0xFFFFFF00) == MAGIC_INTENT;
  watchdog_hw->scratch[1] = 0;
  if ((s0 & 0xFFFFFF00) == MAGIC_MODE && (s0 & 0xFF) < N_MODES)
    mode = s0 & 0xFF;
  if (ours) drive = s1 & 0x03;
  if (watchdog_caused_reboot() && !ours && mode != 0) {
    crashedMode = mode; // a mode froze the board: fall back
    mode = 0;
  }
  watchdog_hw->scratch[0] = MAGIC_MODE | mode;

  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 2000) delay(10);
  Serial.println("\n[probe] hdmi_probe boot");
  if (crashedMode >= 0)
    Serial.printf("[probe] mode %d froze the board (watchdog), back to mode 0\n",
                  crashedMode);

  probe = new ProbeDVI(MODES[mode]);
  rp2040.wdt_begin(4000);
  if (!probe->start()) {
    Serial.println("[probe] framebuffer alloc failed");
    pinMode(LED_BUILTIN, OUTPUT);
    for (;;) { digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); delay(500); }
  }
  applyDrive();
  drawCard();
  printState();
}

static void rebootInto(uint8_t m) {
  watchdog_hw->scratch[0] = MAGIC_MODE | m;
  watchdog_hw->scratch[1] = MAGIC_INTENT | drive;
  Serial.printf("[probe] rebooting into mode %u\n", m);
  Serial.flush();
  delay(100);
  rp2040.reboot();
}

void loop() {
  rp2040.wdt_reset();
  while (Serial.available()) {
    char c = Serial.read();
    if (c >= '0' && c < '0' + N_MODES) rebootInto(c - '0');
    else if (c == 'd') {
      drive = (drive + 1) % 4;
      applyDrive();
      drawCard();
      Serial.printf("[probe] drive %u mA\n", DRIVE_MA[drive]);
    } else if (c == '?') printState();
  }
  // moving block along the bottom: proves the picture is live, not frozen
  ProbeDVI &d = *probe;
  int16_t h = d.height(), bs = h / 12;
  d.fillRect(blockX, h - bs - 6, bs, bs, 0);
  blockX = (blockX + 2) % (d.width() - bs - 8) + 4;
  d.fillRect(blockX, h - bs - 6, bs, bs, 0xFFFF);
  delay(16);
}
