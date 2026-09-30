/*
 * MVMNT - custom firmware (rework of the HAGIWO / Modulove Bezier random CV)
 * Original Bezier engine by HAGIWO, released CC0. This reworks the firmware almost
 * completely to act quite a bit like the Kilohearts random modulator.
 * Rev 2026-06-23 v2
 * Output: 9-bit fast PWM on OC1B (~31 kHz carrier, 512 levels). 8-bit fallback
 * documented at writeCV().
 *
 * KNOBS  (physical positions unchanged; functions reassigned):
 *   A0  (Elevate pos)   -> DEPTH       
        output amplitude. 0 = flat, 1 = full swing.
 *   A1  (Stretch pos)   -> CHAOS       
        max deviation of each new point from the current value. Never 
        goes fully static; full CW = full-range jumps.
 *   A3  (Smooth)        -> SMOOTHNESS  
        BIPOLAR. Center = stairstep (instant step then hold). Toward CCW = linear 
        ramps, toward CW = smoothstep ramps. The ramp fraction grows from 0 at center 
        to 1 at either end.
 *   A5  (Fluctuate pos) -> FREQ
        Rate of new points. Full CCW will freeze the output, and you'll need to use 
        the Trigger input to generate a new random value..
 *
 * TRIG IN (pin 3)
        a rising edge generates a new random point immediately and
        re-phases the internal clock. Runs alongside FREQ (interleaving = polyrhythm).
        When FREQ is frozen, triggers are the only thing that advances the value;
        ramp time then follows the gap between the last two triggers.
 */

#include <avr/io.h>

// ---- tunables --------------------------------------------------------------
#define PIN_CV        10          // OC1B PWM output
#define PIN_TRIG      3           // trigger input

#define FREQ_MIN_HZ   0.05f       // slowest non-frozen rate (~1 point / 20 s)
#define FREQ_MAX_HZ   1000.0f     // fastest. Real ceiling is the output RC filter
                                  // AND the control rate (see SAMPLE_INTERVAL_US).
#define FREEZE_ENTER  8           // ADC reading below this -> enter freeze
#define FREEZE_EXIT   24          // ADC reading above this -> leave freeze (hysteresis)

#define CHAOS_MIN     0.03f       // floor so CHAOS never goes fully static

#define FROZEN_DEFAULT_INTERVAL_US 500000UL   // ramp basis before a trigger gap is known
#define MAX_INTERVAL_US            8000000UL  // clamp very slow (8 s)
#define MIN_INTERVAL_US            400UL      // clamp very fast (2.5 kHz point rate).
                                              // Must be < 1e6/FREQ_MAX_HZ or FREQ_MAX
                                              // gets silently capped here.

#define SAMPLE_INTERVAL_US 200    // control/output update period (~5 kHz). Steps per
                                  // point = (1e6/freq)/this; at 1 kHz that is ~5.
                                  // Lower it for finer fast movement, but watch the
                                  // float budget (~85 us of work per sample).
#define KNOB_INTERVAL_US   20000  // knob re-read period (~50 Hz)
#define TRIG_DEBOUNCE_US   500    // ignore re-triggers within this window

#define OUT_MAX 511               // 9-bit PWM (TOP = 511). See writeCV() for 8-bit.

// ---- state -----------------------------------------------------------------
float depth      = 1.0f;
float chaos      = 1.0f;
float smoove = 0.0f;        // ramp fraction (magnitude of the bipolar knob)
bool  smooveLinear = false;       // true = linear ramp, false = smoove
float freqHz       = 1.0f;
bool  frozen       = false;

float rampStart  = 0.0f;        // value at the start of the current ramp
float target     = 0.0f;        // value we are heading toward
float out        = 0.0f;        // current output, 0..1 (pre-depth)

unsigned long internalIntervalUs = 1000000UL;  // cached 1/FREQ in us
unsigned long curIntervalUs      = FROZEN_DEFAULT_INTERVAL_US; // ramp basis for this point
float invRampUs = 0.0f;           // 1 / (ramp duration); 0 means instant (stairstep)
unsigned long lastPointUs = 0;
unsigned long lastSampleUs = 0;
unsigned long lastKnobUs   = 0;
unsigned long lastTrigUs   = 0;
int prevTrig = LOW;

// ----------------------------------------------------------------------------
// Recompute the ramp reciprocal whenever smoothness or the interval changes,
// so the hot loop multiplies instead of dividing.
void updateRamp() {
  float rampUs = smoove * (float)curIntervalUs;
  invRampUs = (rampUs > 1.0f) ? (1.0f / rampUs) : 0.0f;
}

void readKnobs() {
  depth = analogRead(A0) / 1023.0f;
  chaos = CHAOS_MIN + (1.0f - CHAOS_MIN) * (analogRead(A1) / 1023.0f);

  // Bipolar smoothness: distance from center = ramp fraction, side = curve type.
  float b = analogRead(A3) / 1023.0f - 0.5f;     // -0.5 .. +0.5
  float mag = (b < 0.0f) ? -b : b;               // 0 .. 0.5
  smoove = constrain(mag * 2.0f, 0.0f, 1.0f);
  smooveLinear = (b < 0.0f);                      // CCW half linear, CW half smoothstep

  int f = analogRead(A5);
  if (frozen) {
    if (f > FREEZE_EXIT) frozen = false;
  } else {
    if (f < FREEZE_ENTER) frozen = true;
  }
  if (!frozen) {
    float t = (f - FREEZE_EXIT) / (1023.0f - FREEZE_EXIT);
    t = constrain(t, 0.0f, 1.0f);
    freqHz = FREQ_MIN_HZ * pow(FREQ_MAX_HZ / FREQ_MIN_HZ, t);  // exponential sweep
    internalIntervalUs = (unsigned long)(1000000.0f / freqHz);
    internalIntervalUs = constrain(internalIntervalUs, MIN_INTERVAL_US, MAX_INTERVAL_US);
  }
  updateRamp();
}

// Pick a new target and reset the ramp/hold cycle from wherever we currently are.
void newPoint(unsigned long now) {
  if (!frozen) {
    curIntervalUs = internalIntervalUs;
  } else {
    unsigned long gap = now - lastPointUs;       // time since previous point
    curIntervalUs = (gap > 0) ? gap : FROZEN_DEFAULT_INTERVAL_US;
  }
  curIntervalUs = constrain(curIntervalUs, MIN_INTERVAL_US, MAX_INTERVAL_US);

  rampStart = out;                           // continue smoothly from current value

  // CHAOS: new target is a bounded step from the current value; rails reflect.
  float delta = (random(0, 2001) / 1000.0f - 1.0f) * chaos;  // [-chaos, +chaos]
  float v = rampStart + delta;
  if (v < 0.0f) v = -v;
  if (v > 1.0f) v = 2.0f - v;
  target = v;

  lastPointUs = now;
  updateRamp();
}

void writeCV(float v01) {
  v01 = constrain(v01, 0.0f, 1.0f);
  OCR1B = (uint16_t)(v01 * OUT_MAX + 0.5f);
  // --- 8-bit fallback ---
  //   In setup() use:  TCCR1B &= B11111000;  TCCR1B |= B00000001;
  //   set OUT_MAX to 255, and replace the line above with:
  //     analogWrite(PIN_CV, (int)(v01 * OUT_MAX + 0.5f));
}

// ----------------------------------------------------------------------------
void setup() {
  pinMode(PIN_CV, OUTPUT);
  pinMode(PIN_TRIG, INPUT);

  // 9-bit Fast PWM on OC1B: TOP = 511, prescaler /1  -> ~31.25 kHz carrier.
  TCCR1A = _BV(COM1B1) | _BV(WGM11);
  TCCR1B = _BV(WGM12)  | _BV(CS10);

  randomSeed(analogRead(A2) * 31 + analogRead(A4) * 7 + micros());

  unsigned long now = micros();
  lastPointUs = lastSampleUs = lastKnobUs = now;

  readKnobs();                                   // initialize all knob-derived state
  curIntervalUs = frozen ? FROZEN_DEFAULT_INTERVAL_US : internalIntervalUs;
  target = rampStart = out = random(0, 1001) / 1000.0f;
  updateRamp();
}

// ----------------------------------------------------------------------------
void loop() {
  unsigned long now = micros();

  // Trigger: rising edge -> new point now. Checked every loop so none is missed.
  int t = digitalRead(PIN_TRIG);
  if (t == HIGH && prevTrig == LOW && (now - lastTrigUs) > TRIG_DEBOUNCE_US) {
    lastTrigUs = now;
    newPoint(now);
  }
  prevTrig = t;

  if (now - lastKnobUs >= KNOB_INTERVAL_US) {
    lastKnobUs = now;
    readKnobs();
  }

  if (now - lastSampleUs >= SAMPLE_INTERVAL_US) {
    lastSampleUs = now;

    // Internal clock: fire when the LIVE interval has elapsed (responsive both ways).
    if (!frozen && (now - lastPointUs) >= internalIntervalUs) {
      newPoint(now);
    }

    unsigned long elapsed = now - lastPointUs;
    float p = (invRampUs > 0.0f) ? (float)elapsed * invRampUs : 1.0f;  // 0 => stairstep
    p = constrain(p, 0.0f, 1.0f);
    float s = smooveLinear ? p : (p * p * (3.0f - 2.0f * p));          // linear or smoothstep
    out = rampStart + (target - rampStart) * s;

    writeCV(out * depth);
  }
}
