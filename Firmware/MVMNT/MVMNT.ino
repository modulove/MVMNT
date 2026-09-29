// MVMNT - Bezier curve smooth random CV (HAGIWO's design, CC0)
//
// This code was originally written by HAGIWO and released under CC0.
// Modulove additions (2026-09): runs unchanged on the Arduino Nano and on the
// pin-compatible LGT8F328P nano boards (32 MHz, detected at compile time).
//
//   A0  LEVEL   output level            A1  STRETCH  rate
//   A3  CURVE   transition shape        A5  FLUCT    randomness of the rate
//   D3  TRIG    track & hold            D10 CV out   8-bit PWM, Timer1 at prescaler 1
//                                                    (31 kHz on the Nano, 63 kHz on the LGT)
//
// What differs from HAGIWO's original, and why:
//  * The Bezier terms are computed with products instead of pow(). Same numbers,
//    ~7x faster. His pow() math needed 915 us per step on a Nano, more than the
//    small time increments at the start and end of a segment, so at faster
//    STRETCH settings those steps ran late and the ramps got rounded toes - at
//    CURVE = 0 the design is a straight line and now it is one.
//  * The tempo is capped at his fastest pace: a segment never takes less than
//    255 x FASTEST_STEP_US = 233 ms, which is what the compute limit gave before.
//    Maximum rate as always, shapes exact, identical on both boards.
//  * Step timing uses overflow-safe unsigned arithmetic. The original kept micros()
//    in a signed long: 35.8 min after power-up it went negative, the step condition
//    became always true and the curve ran at full speed whatever STRETCH said, for
//    the next 35.8 min - and again every ~72 min.
//  * Unused normal-distribution tables removed (RAM), ADC pinned to 10 bit on the LGT.

#include <avr/io.h> // For fast PWM

#if defined(__LGT8FX8P__)
  #define MCU_IS_LGT 1
#else
  #define MCU_IS_LGT 0
#endif

#define FASTEST_STEP_US 915.0f   // measured on the original firmware, Nano, STRETCH at maximum

int i = 0;
int start_val = 0; // Bezier Curve Starting Point
int end_val = 255; // Bezier Curve end Point
float old_wait = 0;
float wait = 0; // Bezier curve x-axis (time)
float bz_val = 0; // Bezier curve y-axis (voltage)
int level, curve, freq;
unsigned long timer = 0;  // last step (micros)
unsigned long timer1 = 0; // Analog read interval (millis)
float x[256]; // Bezier Curve Calculation Tables

int freq_dev = 40;

int prevTrigState = LOW;
float holdValue = 0;

void setup() {
  // Prepare Bezier Curve Calculation Tables
  for (int j = 0; j < 255; j++) {
    x[j] = j * 0.003921; // j / 255
  }

  pinMode(10, OUTPUT); // CV output
#if MCU_IS_LGT
  analogReadResolution(10); // the LGT core's default, pinned so the pot ranges never move
#endif
  timer = micros();
  timer1 = millis();

  // 9,10 pin PWM setting
  TCCR1B &= B11111000;
  TCCR1B |= B00000001;
  delay(50);
}

void loop() {

int currentTrigState = digitalRead(3); // Read the trig input

  // Track & Hold: retain last valid voltage
  if (currentTrigState != prevTrigState) {
    if (currentTrigState == HIGH) {
      holdValue = bz_val * level / 255; // Hold the current voltage
    }
    prevTrigState = currentTrigState;
  }

  if (millis() - timer1 >= 50) { // Fixed timing condition
    freq = map(analogRead(A1), 0, 1023, 250, 1) * freq_dev; // Stretch (A1) controls frequency
    curve = map(analogRead(A3), 0, 1023, 0, 255); // Smooth (A3) controls transition shape
    level = analogRead(A0) / 4; // Elevate (A0) controls output level
    timer1 = millis();
  }

  // Time to the next step: HAGIWO's Bezier time increment, tempo capped at the
  // original's fastest pace (freq below FASTEST_STEP_US/2 would be quicker than
  // his code could ever step). FLUCT can push freq to zero or below - his code
  // then ran flat out, which is the same cap.
  float interval = wait - old_wait;
  if (freq < 1)                          interval = FASTEST_STEP_US;
  else if (2.0f * freq < FASTEST_STEP_US) interval *= FASTEST_STEP_US / (2.0f * freq);
  if (interval < 0) interval = 0;

  if (micros() - timer >= (unsigned long)interval) {
    old_wait = wait;
    i++;

    if (i >= 255) { // Recalculation of target voltage values
      i = 0;
      start_val = end_val;
      end_val = random(0, 255);
      change_freq_error(); // Apply fluctuation
    }

    // Bezier Curve Calculations (HAGIWO's formulas, powers written as products)
    float xi = x[i], u = 1 - xi;
    float u2 = u * u, x2 = xi * xi;
    float u3 = u2 * u, x3 = x2 * xi;
    wait = 3 * u2 * xi * curve +
           3 * u * x2 * (255 - curve) +
           x3 * 255;
    wait = max(5, 1 + wait * freq * 2); // Ensure wait never goes negative
    bz_val = u3 * start_val +
             3 * u2 * xi * start_val +
             3 * u * x2 * end_val +
             x3 * end_val;

    timer = micros();
    PWM_OUT(); // PWM output
  }
}

// Fluctuate (A5) modifies randomness of frequency changes
void change_freq_error() {
  int deviation = map(analogRead(A5), 0, 1023, 0, 300); // Fluctuate (A5) controls randomness range
  freq = freq + random(-deviation, deviation); // Apply randomness to Stretch-based frequency
}

void PWM_OUT() {
    if (prevTrigState == HIGH) {
        analogWrite(10, holdValue); // Output held value
    } else {
        analogWrite(10, bz_val * level / 255); // Output normal signal
    }
}
