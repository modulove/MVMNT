/*
  SyncLFO  (MDL_HAGIWO_TuringLFO v2, quantized)
  ============================================
  HAGIWO sync LFO + clocked S&H / Turing machine + quantized MELODY mode.
  Runs on the Arduino Nano (ATmega328P, 16 MHz) and on the pin-compatible
  LGT8F328P nano boards (32 MHz); the board is detected at compile time.

  Modes (WAVE knob, CCW -> CW): steady, saw up, saw down, sine, tri, square,
  random (raw S&H), MELODY (quantized S&H, fully CW).

  LFO waves
  ---------
  * FREQ = rate; with a clock on CLK IN the LFO hard-syncs to it and FREQ
    becomes the PHASE knob (0..999), exactly as in HAGIWO's firmware.
  * SELF-MOD = HAGIWO's self modulation, reproduced bit for bit - including
    the way his code runs off the end of the wave tables. That overrun is
    what makes the shapes finely adjustable with PHASE and repetitive with
    the clock, so it is emulated on a virtual copy of a Nano's flash
    (vflash_word + hagiwo_flash_image.h) and is identical on every board.
    Zones: off, saw up, saw down, sine, tri, square, random. It refreshes
    every SELF_MOD_UPDATE_MS like his loop did. SELF_MOD_AM 1 turns the
    same zones into amplitude modulation instead (fade, swell, chop).
  * AMP = level.

  random / MELODY (looping S&H, Turing style)
  -------------------------------------------
  * 16 note slots played as a loop. Clock on CLK IN: one step per rising
    edge, FREQ = loop length 2 3 4 5 6 8 10 12. No clock: FREQ = rate, loop
    length TM_DEFAULT_LEN. Slots beyond the current length keep their notes,
    so lengthening brings them back - the loop never runs dry.
  * SELF-MOD: fully CCW locked, the loop repeats for ever. Opening it lets
    single new notes sprinkle in, sparsely at first (square law), until at
    12 o'clock every note is new. Past 12 o'clock the loop comes round
    inverted, again with fewer and fewer new notes, until fully CW it is
    locked at double length (loop + its mirror). A change is always one
    note, never a cascade, and the loop can never run empty.
  * random: AMP = level, raw 8-bit S&H out.
  * MELODY: the byte is quantized to a scale and sent out as 1 V/oct.
    AMP = range (CCW root only, CW MELODY_OCTAVES octaves). Scale via
    MELODY_SCALE (pentatonic minor active, others in SCALE_STEPS).

  Output / calibration
  --------------------
  * Timer1 fast PWM with PWM_STEPS levels (2000): 8 kHz on the Nano,
    16 kHz on the LGT8F328P at 32 MHz - same resolution, half the ripple.
    5 mV per step at 10 V. (1000 steps would double the PWM frequency.)
  * 1 V/oct: WAVE = steady, AMP fully CW, meter on OUT 1 -> CAL_FULL_SCALE_MV.
    Optionally AMP fully CCW -> CAL_ZERO_MV, which takes any op-amp offset
    out of the scale. Re-measure after swapping the board (Nano <-> LGT).
    Every note carries the same +1 PWM step (fast PWM duty is
    (OCR+1)/(TOP+1)), so intervals are exact and the offset is tuning.

  Hardware
  --------
    D3   CLK IN     rising edge, 0/5 V
    D10  OUT        OC1B, fast PWM
    A0   AMP        level | range (MELODY)
    A1   FREQ       rate | phase (LFO + clock) | length (random/MELODY + clock)
    A3   WAVE
    A5   SELF-MOD   self modulation (LFO) | Turing probability (random/MELODY)

  Build: arduino:avr:nano:cpu=atmega328old   (Nano, old bootloader)
         lgt8fx:avr:328                      (LGT8F328P nano-style, defaults)
         Always compile and upload in one go (compile -u -p COMx): the upload
         command alone flashes whatever the last compile for ANY board left.
*/

#include <Arduino.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "hagiwo_flash_image.h"

// ----------------------------------------------------------------- board
#if defined(__LGT8FX8P__)
  #define MCU_IS_LGT 1                  // LGT8F328P (32 MHz internal RC, 12-bit ADC used at 10 bit)
#else
  #define MCU_IS_LGT 0                  // ATmega328P Nano
#endif

// ------------------------------------------------------------------ pins
#define PWM_PIN   10
#define CLK_PIN    3      // must stay on PD3, the ISR reads PIND directly
#define POT_AMP   A0
#define POT_FREQ  A1
#define POT_WAVE  A3
#define POT_MOD   A5

// ---------------------------------------------------------------- melody
#define SCALE_PENT_MINOR   0
#define SCALE_PENT_MAJOR   1
#define SCALE_MAJOR        2
#define SCALE_MINOR        3
#define SCALE_DORIAN       4
#define SCALE_BLUES        5
#define SCALE_WHOLE_TONE   6
#define SCALE_CHROMATIC    7
#define SCALE_OCT_FIFTHS   8

#define MELODY_SCALE       SCALE_PENT_MINOR   // <- swap here
#define MELODY_OCTAVES     2                  // range with AMP fully CW
#if MCU_IS_LGT                                // measured OUT 1 per board: WAVE steady + AMP fully CW,
  #define CAL_FULL_SCALE_MV  9750UL           // and any LFO wave + AMP fully CCW
  #define CAL_ZERO_MV        10UL
#else
  #define CAL_FULL_SCALE_MV  9740UL
  #define CAL_ZERO_MV        10UL
#endif
#define MELODY_ZONE        981                // WAVE >= this -> MELODY, 939..980 stays raw random

static const uint8_t SCALE_LEN[9] PROGMEM = { 5, 5, 7, 7, 7, 6, 6, 12, 2 };
static const uint8_t SCALE_STEPS[9][12] PROGMEM = {
  { 0, 3, 5, 7, 10 },                          // pentatonic minor
  { 0, 2, 4, 7, 9 },                           // pentatonic major
  { 0, 2, 4, 5, 7, 9, 11 },                    // major
  { 0, 2, 3, 5, 7, 8, 10 },                    // natural minor
  { 0, 2, 3, 5, 7, 9, 10 },                    // dorian
  { 0, 3, 5, 6, 7, 10 },                       // blues
  { 0, 2, 4, 6, 8, 10 },                       // whole tone
  { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 },    // chromatic
  { 0, 7 },                                    // octaves + fifths
};

// ---------------------------------------------------------------- tuning
#define PWM_STEPS_CFG      2000UL           // PWM levels; PWM frequency = F_CPU / steps:
                                            // 8 kHz on the 16 MHz Nano, 16 kHz on the 32 MHz LGT8F328P
#define TICK_HZ            20000UL          // timer interrupt rate, 50 us
#define TABLE_LEN          1000             // steps per LFO cycle
#define SH_INTERNAL_DIV    125              // internal S&H clock = one sample per 125 LFO steps
#define EXT_CLOCK_TIMEOUT  (8UL * TICK_HZ)  // no edge for 8 s -> back to the internal clock
#define CLK_IN_ENABLED     1                // 0 = ignore CLK IN completely (diagnosis)
#define CLK_STABLE         10               // CLK IN must be low, then high, for this many ticks (0.5 ms) to count
#define CLK_REFRACTORY     40               // ignore a second edge within 2 ms (max clock 500 Hz)
#define SELF_MOD_UPDATE_MS 15               // self-mod refresh: 15 = HAGIWO's loop with his serial debug
                                            // print on (the code as received), 0 = every pass (~0.5 ms)
#define SELF_MOD_AM        0                // 1 = the mod wave scales the AMPLITUDE instead of bending the phase
#define TM_STAGES          16               // note slots (>= longest loop)
#define TM_DEFAULT_LEN     8                // loop length with the internal clock
#define TM_LOCK_CCW        30               // SELF-MOD <= this  -> locked
#define TM_LOCK_CW         993              // SELF-MOD >= this  -> locked, double length

static const uint16_t PWM_STEPS = PWM_STEPS_CFG;
static const uint16_t PWM_TOP   = PWM_STEPS - 1;
static const uint8_t  TICK_OCR  = F_CPU / 8UL / TICK_HZ - 1;     // 99 @ 16 MHz, 199 @ 32 MHz
static_assert(F_CPU / 8UL / TICK_HZ - 1 <= 255, "Timer2 tick does not fit this F_CPU");
static_assert(PWM_STEPS_CFG >= 1000 && PWM_STEPS_CFG <= 65536, "PWM_STEPS_CFG out of range");

enum Wave : uint8_t { W_SAW_UP = 0, W_SAW_DOWN, W_SINE, W_TRI, W_SQUARE, W_RANDOM, W_STEADY, W_MELODY };

// the 7 pot zones of the WAVE and SELF-MOD knobs (HAGIWO's thresholds), CCW -> CW
static const uint8_t ZONE_TO_WAVE[7] = { W_STEADY, W_SAW_UP, W_SAW_DOWN, W_SINE, W_TRI, W_SQUARE, W_RANDOM };
static const uint8_t TM_LENGTHS[8]   = { 2, 3, 4, 5, 6, 8, 10, 12 };   // FREQ knob zones when clocked

// HAGIWO's flash layout (avr-gcc 7.3, Arduino AVR core): the five 1000-word tables
// sit back to back right after the vector table and the core's pin tables.
#define ORIG_TABLES_ADDR   0x00C2
static const uint16_t ORIG_TABLE_ADDR[5]  = { 0x2002, 0x1832, 0x1062, 0x0892, 0x00C2 };   // by Wave: saw up, saw down, sine, tri, square
static const uint8_t  ORIG_TABLE_ORDER[5] = { W_SQUARE, W_TRI, W_SINE, W_SAW_DOWN, W_SAW_UP }; // by address

// sine, 0..1000, generated: round(500 + 500 * sin(2*pi*i/1000))
const uint16_t SINE[TABLE_LEN] PROGMEM = {
   500, 503, 506, 509, 513, 516, 519, 522, 525, 528, 531, 535, 538, 541, 544, 547, 550, 553, 556, 560,
   563, 566, 569, 572, 575, 578, 581, 584, 588, 591, 594, 597, 600, 603, 606, 609, 612, 615, 618, 621,
   624, 627, 630, 633, 636, 639, 643, 646, 649, 652, 655, 657, 660, 663, 666, 669, 672, 675, 678, 681,
   684, 687, 690, 693, 696, 699, 701, 704, 707, 710, 713, 716, 719, 721, 724, 727, 730, 733, 735, 738,
   741, 744, 746, 749, 752, 755, 757, 760, 763, 765, 768, 771, 773, 776, 778, 781, 784, 786, 789, 791,
   794, 796, 799, 801, 804, 806, 809, 811, 814, 816, 819, 821, 824, 826, 828, 831, 833, 835, 838, 840,
   842, 845, 847, 849, 851, 854, 856, 858, 860, 862, 864, 867, 869, 871, 873, 875, 877, 879, 881, 883,
   885, 887, 889, 891, 893, 895, 897, 899, 901, 903, 905, 906, 908, 910, 912, 914, 915, 917, 919, 920,
   922, 924, 925, 927, 929, 930, 932, 934, 935, 937, 938, 940, 941, 943, 944, 946, 947, 948, 950, 951,
   952, 954, 955, 956, 958, 959, 960, 961, 963, 964, 965, 966, 967, 968, 969, 970, 971, 973, 974, 975,
   976, 976, 977, 978, 979, 980, 981, 982, 983, 984, 984, 985, 986, 987, 987, 988, 989, 989, 990, 991,
   991, 992, 992, 993, 993, 994, 994, 995, 995, 996, 996, 996, 997, 997, 997, 998, 998, 998, 999, 999,
   999, 999, 999,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000, 999, 999,
   999, 999, 999, 998, 998, 998, 997, 997, 997, 996, 996, 996, 995, 995, 994, 994, 993, 993, 992, 992,
   991, 991, 990, 989, 989, 988, 987, 987, 986, 985, 984, 984, 983, 982, 981, 980, 979, 978, 977, 976,
   976, 975, 974, 973, 971, 970, 969, 968, 967, 966, 965, 964, 963, 961, 960, 959, 958, 956, 955, 954,
   952, 951, 950, 948, 947, 946, 944, 943, 941, 940, 938, 937, 935, 934, 932, 930, 929, 927, 925, 924,
   922, 920, 919, 917, 915, 914, 912, 910, 908, 906, 905, 903, 901, 899, 897, 895, 893, 891, 889, 887,
   885, 883, 881, 879, 877, 875, 873, 871, 869, 867, 864, 862, 860, 858, 856, 854, 851, 849, 847, 845,
   842, 840, 838, 835, 833, 831, 828, 826, 824, 821, 819, 816, 814, 811, 809, 806, 804, 801, 799, 796,
   794, 791, 789, 786, 784, 781, 778, 776, 773, 771, 768, 765, 763, 760, 757, 755, 752, 749, 746, 744,
   741, 738, 735, 733, 730, 727, 724, 721, 719, 716, 713, 710, 707, 704, 701, 699, 696, 693, 690, 687,
   684, 681, 678, 675, 672, 669, 666, 663, 660, 657, 655, 652, 649, 646, 643, 639, 636, 633, 630, 627,
   624, 621, 618, 615, 612, 609, 606, 603, 600, 597, 594, 591, 588, 584, 581, 578, 575, 572, 569, 566,
   563, 560, 556, 553, 550, 547, 544, 541, 538, 535, 531, 528, 525, 522, 519, 516, 513, 509, 506, 503,
   500, 497, 494, 491, 487, 484, 481, 478, 475, 472, 469, 465, 462, 459, 456, 453, 450, 447, 444, 440,
   437, 434, 431, 428, 425, 422, 419, 416, 412, 409, 406, 403, 400, 397, 394, 391, 388, 385, 382, 379,
   376, 373, 370, 367, 364, 361, 357, 354, 351, 348, 345, 343, 340, 337, 334, 331, 328, 325, 322, 319,
   316, 313, 310, 307, 304, 301, 299, 296, 293, 290, 287, 284, 281, 279, 276, 273, 270, 267, 265, 262,
   259, 256, 254, 251, 248, 245, 243, 240, 237, 235, 232, 229, 227, 224, 222, 219, 216, 214, 211, 209,
   206, 204, 201, 199, 196, 194, 191, 189, 186, 184, 181, 179, 176, 174, 172, 169, 167, 165, 162, 160,
   158, 155, 153, 151, 149, 146, 144, 142, 140, 138, 136, 133, 131, 129, 127, 125, 123, 121, 119, 117,
   115, 113, 111, 109, 107, 105, 103, 101,  99,  97,  95,  94,  92,  90,  88,  86,  85,  83,  81,  80,
    78,  76,  75,  73,  71,  70,  68,  66,  65,  63,  62,  60,  59,  57,  56,  54,  53,  52,  50,  49,
    48,  46,  45,  44,  42,  41,  40,  39,  37,  36,  35,  34,  33,  32,  31,  30,  29,  27,  26,  25,
    24,  24,  23,  22,  21,  20,  19,  18,  17,  16,  16,  15,  14,  13,  13,  12,  11,  11,  10,   9,
     9,   8,   8,   7,   7,   6,   6,   5,   5,   4,   4,   4,   3,   3,   3,   2,   2,   2,   1,   1,
     1,   1,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,
     1,   1,   1,   2,   2,   2,   3,   3,   3,   4,   4,   4,   5,   5,   6,   6,   7,   7,   8,   8,
     9,   9,  10,  11,  11,  12,  13,  13,  14,  15,  16,  16,  17,  18,  19,  20,  21,  22,  23,  24,
    24,  25,  26,  27,  29,  30,  31,  32,  33,  34,  35,  36,  37,  39,  40,  41,  42,  44,  45,  46,
    48,  49,  50,  52,  53,  54,  56,  57,  59,  60,  62,  63,  65,  66,  68,  70,  71,  73,  75,  76,
    78,  80,  81,  83,  85,  86,  88,  90,  92,  94,  95,  97,  99, 101, 103, 105, 107, 109, 111, 113,
   115, 117, 119, 121, 123, 125, 127, 129, 131, 133, 136, 138, 140, 142, 144, 146, 149, 151, 153, 155,
   158, 160, 162, 165, 167, 169, 172, 174, 176, 179, 181, 184, 186, 189, 191, 194, 196, 199, 201, 204,
   206, 209, 211, 214, 216, 219, 222, 224, 227, 229, 232, 235, 237, 240, 243, 245, 248, 251, 254, 256,
   259, 262, 265, 267, 270, 273, 276, 279, 281, 284, 287, 290, 293, 296, 299, 301, 304, 307, 310, 313,
   316, 319, 322, 325, 328, 331, 334, 337, 340, 343, 345, 348, 351, 354, 357, 361, 364, 367, 370, 373,
   376, 379, 382, 385, 388, 391, 394, 397, 400, 403, 406, 409, 412, 416, 419, 422, 425, 428, 431, 434,
   437, 440, 444, 447, 450, 453, 456, 459, 462, 465, 469, 472, 475, 478, 481, 484, 487, 491, 494, 497
};

// ------------------------------------------------- shared with the ISR
volatile uint8_t  wave          = W_SAW_UP;
volatile uint32_t cycle_ticks   = TABLE_LEN;      // timer ticks per LFO cycle (>= TABLE_LEN)
volatile int16_t  lg_count      = 0;              // HAGIWO's `count`  (goes negative on purpose)
volatile int16_t  lg_phase      = 0;              // HAGIWO's `phase`  = PHASE knob + self-mod read
volatile uint16_t am_depth      = 1000;           // SELF_MOD_AM: amplitude factor 0..1000
volatile uint16_t tm_prob       = 0;              // 0..1024, chance that the returning note is replaced by a new one
volatile bool     tm_invert     = false;          // CW side: the returning note comes back mirrored (255 - v)
volatile uint8_t  tm_len        = TM_DEFAULT_LEN;
volatile uint8_t  tm_byte       = 0;              // the current note value 0..255
volatile uint16_t level         = 0;              // output before AMP: 0..1000, more on table overruns
volatile bool     clock_present = false;

// ------------------------------------------------------ ISR-only state
static uint32_t acc         = 0;                  // fractional step accumulator
static uint8_t  sh_div      = 0;
static uint32_t since_clk   = EXT_CLOCK_TIMEOUT;
static uint32_t period_last = 0;
static uint8_t  clk_edges   = 0;                  // edges since the clock appeared, saturates at 3
static uint8_t  clk_hi_run  = 0;                  // consecutive high / low samples on CLK IN
static uint8_t  clk_lo_run  = 0;
static bool     clk_armed   = false;              // low long enough, next stable high is an edge
static uint8_t  tm_seq[TM_STAGES];               // the note slots
static uint8_t  tm_pos      = 0;                  // slot that plays next
static bool     tm_pass2    = false;              // CW side: this pass plays the loop mirrored
static uint32_t rng         = 0x2545F491UL;

static inline bool is_turing(uint8_t w) { return w == W_RANDOM || w == W_MELODY; }

static inline uint32_t xorshift32() {
  uint32_t x = rng;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  return rng = x;
}

// the five wave tables of the original, computed (only the sine is stored)
static inline uint16_t wave_value(uint8_t w, uint16_t idx) {
  while (idx >= TABLE_LEN) idx -= TABLE_LEN;
  switch (w) {
    case W_SAW_UP:   return idx;                                                   // 0..999
    case W_SAW_DOWN: return (TABLE_LEN - 1) - idx;                                 // 999..0
    case W_TRI:      return idx < TABLE_LEN / 2 ? idx * 2 : (TABLE_LEN - idx) * 2; // 0..1000..2
    case W_SQUARE:   return idx < TABLE_LEN / 2 ? 0 : 1000;
    case W_SINE:     return pgm_read_word(&SINE[idx]);
    case W_STEADY:   return 1000;
  }
  return 0;
}

// ---- HAGIWO's self modulation, emulated exactly --------------------------
// His code did  index = count + phase,  phase = PHASE knob + modtable[count],
// and wrapped with  count -= 1000  whenever count + phase >= 1000. That leaves
// count negative, so modtable[count] and wavetable[index] then read *outside*
// their tables: the neighbouring table, the pin tables and vectors below the
// first one, erased flash (0xFFFF) or the bootloader. Those reads are what
// shaped the self-mod, so they are reproduced on a virtual copy of his layout:
//   0x00C2 squ | 0x0892 tri | 0x1062 sine | 0x1832 saw2 | 0x2002 saw1
// Below the tables his vectors and pin tables are embedded (ORIG_HEAD), the old
// Nano bootloader too (ORIG_BOOT); his code and erased flash read as 0xFFFF. So
// nothing depends on what is physically in this chip - Nano and LGT8F328P match.
static uint16_t vflash_word(uint16_t addr) {
  addr &= 0x7FFF;                                                   // 32 KB flash mirrors above 0x7FFF
  if (addr < ORIG_TABLES_ADDR) return pgm_read_word(ORIG_HEAD + addr);          // vectors + pin tables
  if (addr < ORIG_TABLES_ADDR + 5U * 2000U) {                                    // the five wave tables
    uint16_t off = addr - ORIG_TABLES_ADDR;
    uint8_t  t = 0;
    while (off >= 2000) { off -= 2000; t++; }
    return wave_value(ORIG_TABLE_ORDER[t], off >> 1);
  }
  if (addr >= 0x7800) return pgm_read_word(ORIG_BOOT + (addr - 0x7800));       // old Nano bootloader
  return 0xFFFF;                                                                 // his code / erased flash
}

// what the original read as  table[idx]  - idx may be anything, like his int
static inline uint16_t legacy_read(uint8_t w, int16_t idx) {
  return vflash_word((uint16_t)(ORIG_TABLE_ADDR[w] + 2U * (uint16_t)idx));
}

// One clock of the sequencer. 16 note slots, the first tm_len of them play as a
// loop. With probability tm_prob the slot that is up gets a fresh random note -
// one note changes, never a cascade, and it stays in the loop. On the CW side
// every second pass plays the loop mirrored (255 - v): loop + inverse = double
// length. Slots past tm_len are never touched, so the loop cannot run dry.
static inline void turing_step() {
  if (tm_pos >= tm_len) {                                           // wrap (also after shortening)
    tm_pos = 0;
    tm_pass2 = tm_invert ? !tm_pass2 : false;
  }
  if ((xorshift32() & 0x3FF) < tm_prob) tm_seq[tm_pos] = xorshift32() & 0xFF;   // a new note sprinkles in
  uint8_t v = tm_seq[tm_pos];
  if (tm_pass2) v = 255 - v;
  tm_pos++;
  tm_byte = v;
  level   = ((uint32_t)v * 251UL) >> 6;                            // 0..255 -> 0..1000
}

ISR(TIMER2_COMPA_vect) {
  uint8_t w = wave;
  bool turing = is_turing(w);

  // ---- CLK IN: rising edge, sampled every 50 us, glitch filtered ----
  // The pin has to sit low for CLK_STABLE ticks and then high for CLK_STABLE ticks
  // before it counts as an edge. Every real trigger (>= 1 ms) passes; PWM spikes,
  // hum bursts and contact bounce on an unpatched jack do not. HAGIWO's loop only
  // ever saw pulses that happened to be high at a ~2 ms poll, so nothing is lost.
  bool rising = false;
#if CLK_IN_ENABLED
  bool clk = PIND & _BV(PD3);
  if (clk) {
    clk_lo_run = 0;
    if (clk_hi_run < 255) clk_hi_run++;
    if (clk_armed && clk_hi_run == CLK_STABLE) { rising = true; clk_armed = false; }
  } else {
    clk_hi_run = 0;
    if (clk_lo_run < 255) clk_lo_run++;
    if (clk_lo_run >= CLK_STABLE) clk_armed = true;
  }
#endif

  if (since_clk < EXT_CLOCK_TIMEOUT) {
    since_clk++;
  } else if (clock_present) {                     // silent for 8 s -> internal clock again
    clock_present = false;
    clk_edges = 0;
  }

  if (rising && since_clk >= CLK_REFRACTORY) {
    if (clk_edges < 3) clk_edges++;
    if (clk_edges == 2) {                         // first full period
      period_last = since_clk;
      cycle_ticks = period_last < TABLE_LEN ? TABLE_LEN : period_last;
    } else if (clk_edges == 3) {                  // running: mean of the last two periods
      uint32_t p = (period_last + since_clk) >> 1;
      period_last = since_clk;
      cycle_ticks = p < TABLE_LEN ? TABLE_LEN : p;
    }
    clock_present = true;
    since_clk = 0;

    // hard sync (LFO: HAGIWO's count = 0) / sample (S&H, melody)
    acc = 0;
    sh_div = 0;
    lg_count = 0;
    if (turing) turing_step();
  }

  // ---- step clock: TABLE_LEN steps per cycle_ticks, fractional ----
  acc += TABLE_LEN;
  if (acc >= cycle_ticks) {
    acc -= cycle_ticks;
    if (turing) {
      if (!clock_present && ++sh_div >= SH_INTERNAL_DIV) {
        sh_div = 0;
        turing_step();
      }
    } else {
      // HAGIWO:  count++;  if (count + phase >= 1000) count -= 1000;  duty = table[count + phase]
      uint16_t c  = (uint16_t)lg_count + 1;
      uint16_t ph = (uint16_t)lg_phase;
      if ((int16_t)(c + ph) >= 1000) c -= 1000;
      lg_count = (int16_t)c;
      uint16_t v = (w == W_STEADY) ? 1000 : legacy_read(w, (int16_t)(c + ph));
#if SELF_MOD_AM
      if (v > 1000) v = 1000;
      v = ((uint32_t)v * am_depth) / 1000;
#endif
      level = v;
    }
  }
}

static uint8_t zone7(uint16_t a) {
  if (a <  31) return 0;
  if (a < 155) return 1;
  if (a < 352) return 2;
  if (a < 571) return 3;
  if (a < 771) return 4;
  if (a < 939) return 5;
  return 6;
}

// FREQ knob -> ticks per LFO cycle. HAGIWO's curve: 1 + 0.0007 * a^2 ticks per step,
// i.e. 50 ms (20 Hz) .. 36.7 s per cycle; internal S&H clock 160 Hz .. 0.22 Hz.
static uint32_t internal_cycle_ticks(uint16_t a) {
  uint32_t ticks_per_step = 1 + ((uint32_t)a * a * 7UL) / 10000UL;   // 1..733
  return ticks_per_step * TABLE_LEN;
}

// Turing byte -> PWM compare value of a 1 V/oct note.
// range = number of reachable scale degrees (1 = root only). The byte is scaled
// to 0..range-1 like the attenuator in front of a hardware quantizer, then the
// degree is looked up in the scale and converted with the measured full scale.
static uint16_t melody_ocr(uint8_t b, uint8_t range) {
  uint8_t len    = pgm_read_byte(&SCALE_LEN[MELODY_SCALE]);
  uint8_t degree = ((uint16_t)b * range) >> 8;
  uint8_t semis  = 12 * (degree / len) + pgm_read_byte(&SCALE_STEPS[MELODY_SCALE][degree % len]);
  // OCR 0..TOP spans CAL_ZERO_MV..CAL_FULL_SCALE_MV, a semitone is 1000/12 mV of it
  uint32_t den   = 12UL * (CAL_FULL_SCALE_MV - CAL_ZERO_MV);
  uint32_t ocr   = ((uint32_t)semis * PWM_TOP * 1000UL + den / 2) / den;     // rounded to the PWM grid
  // fast PWM duty is (OCR+1)/(TOP+1): every note sits one step (5 mV at 10 V) above
  // the grid - a constant offset the VCO tune knob absorbs, intervals stay exact
  return ocr > PWM_TOP ? PWM_TOP : (uint16_t)ocr;
}

void setup() {
  pinMode(PWM_PIN, OUTPUT);
  pinMode(CLK_PIN, INPUT);
#if MCU_IS_LGT
  analogReadResolution(10);                       // the LGT core's default, pinned so the pot zones never move
#endif

  // Timer1: fast PWM (mode 14), TOP = ICR1, OC1B non-inverting, no prescaler
  TCCR1A = _BV(COM1B1) | _BV(WGM11);
  TCCR1B = _BV(WGM13)  | _BV(WGM12) | _BV(CS10);
  ICR1   = PWM_TOP;
  OCR1B  = 0;

  // seed the Turing register from pot positions, floating inputs and time
  uint32_t seed = micros();
  for (uint8_t i = 0; i < 8; i++) {
    seed = (seed << 3) ^ (seed >> 29)
         ^ analogRead(A6)
         ^ ((uint32_t)analogRead(A2)      << 10)
         ^ ((uint32_t)analogRead(POT_MOD) << 20);
  }
  if (seed == 0) seed = 0x2545F491UL;
  rng = seed;
  randomSeed(seed);
  for (uint8_t i = 0; i < TM_STAGES; i++) tm_seq[i] = xorshift32() & 0xFF;   // start with a random loop
  tm_pos  = 0;
  tm_byte = tm_seq[0];

  // Timer2: CTC, /8, 20 kHz -> ISR every 50 us
  TCCR2A = _BV(WGM21);
  TCCR2B = _BV(CS21);
  OCR2A  = TICK_OCR;                              // 99 @ 16 MHz, 199 @ 32 MHz
  TCNT2  = 0;
  TIMSK2 = _BV(OCIE2A);

}

void loop() {
  uint16_t a_amp  = analogRead(POT_AMP);
  uint16_t a_freq = analogRead(POT_FREQ);
  uint16_t a_wave = analogRead(POT_WAVE);
  uint16_t a_mod  = analogRead(POT_MOD);

  uint8_t w      = (a_wave >= MELODY_ZONE) ? (uint8_t)W_MELODY : ZONE_TO_WAVE[zone7(a_wave)];
  bool    turing = is_turing(w);
  bool    ext    = clock_present;

  uint32_t new_cycle = internal_cycle_ticks(a_freq);   // only applied without a clock
  uint16_t new_prob  = 0;
  bool     new_invert = false;
  uint8_t  new_len   = TM_DEFAULT_LEN;
  uint16_t new_phase = 0;
  uint16_t new_am    = 1000;
  bool     set_phase = false;

  if (turing) {
    // SELF-MOD: CCW locked | opening it lets new notes in, sparsely at first (square law)
    // | 12 o'clock every note new | CW side the same, but the loop comes round mirrored
    // | fully CW locked at double length. new_prob 0..1024 per step.
    uint32_t t;
    if      (a_mod <= TM_LOCK_CCW) { new_prob = 0; new_invert = false; }
    else if (a_mod >= TM_LOCK_CW)  { new_prob = 0; new_invert = true;  }
    else if (a_mod < 512) { t = (uint32_t)(a_mod - TM_LOCK_CCW) * 1024UL / (512 - TM_LOCK_CCW); new_prob = (t * t) >> 10; new_invert = false; }
    else                  { t = (uint32_t)(TM_LOCK_CW - a_mod)  * 1024UL / (TM_LOCK_CW - 512);  new_prob = (t * t) >> 10; new_invert = true;  }
    if (ext) new_len = TM_LENGTHS[a_freq >> 7];          // FREQ = loop length when clocked
  } else {
    // HAGIWO's loop:  phase = PHASE knob (clocked) or 0;  phase += modtable[count]
    bool refresh = true;
#if SELF_MOD_UPDATE_MS > 0
    static uint32_t last_phase_ms = 0;
    refresh = (millis() - last_phase_ms >= SELF_MOD_UPDATE_MS);
    if (refresh) last_phase_ms = millis();
#endif
    if (refresh) {
      set_phase = true;
      if (ext) new_phase = map(a_freq, 0, 1023, 0, TABLE_LEN - 1);
      uint8_t m = zone7(a_mod);                          // 0 off, 1 saw up .. 5 square, 6 random
      if (m) {
        int16_t c;
        noInterrupts(); c = lg_count; interrupts();
#if SELF_MOD_AM
        // self AM: the mod wave, read at the LFO's own position, scales the amplitude
        uint16_t idx = (uint16_t)(c + (int16_t)new_phase);
        new_am = (m == 6) ? random(1, TABLE_LEN) : wave_value(ZONE_TO_WAVE[m], idx);
#else
        if (m == 6) new_phase += random(1, TABLE_LEN);   // his saw1[random(1,1000)] is the number itself
        else        new_phase += legacy_read(ZONE_TO_WAVE[m], c);
#endif
      }
    }
  }

  uint16_t lvl; uint8_t byte;
  noInterrupts();
  wave = w;
  if (!ext) cycle_ticks = new_cycle;              // when clocked the ISR owns cycle_ticks
  if (set_phase) { lg_phase = (int16_t)new_phase; am_depth = new_am; }
  tm_prob   = new_prob;
  tm_invert = new_invert;
  tm_len    = new_len;
  lvl       = level;
  byte      = tm_byte;
  interrupts();

  uint8_t range = 0;
  if (w == W_MELODY) {
    // AMP = range: 1 (root only) .. MELODY_OCTAVES octaves of the scale, octave note included
    uint8_t len = pgm_read_byte(&SCALE_LEN[MELODY_SCALE]);
    range = 1 + ((uint32_t)a_amp * (MELODY_OCTAVES * len + 1)) / 1024;
    OCR1B = melody_ocr(byte, range);
  } else {
    // AMP scales the level. lvl can exceed 1000 on the self-mod's table overruns -
    // the original then ran past its PWM top and pinned the output high, same here.
    uint32_t v = (uint32_t)lvl * a_amp / 1023UL;
    v = v * PWM_TOP / 1000UL;
    OCR1B = v > PWM_TOP ? PWM_TOP : (uint16_t)v;
  }

}
