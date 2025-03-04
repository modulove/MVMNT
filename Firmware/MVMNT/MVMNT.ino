// This code was originally written by HAGIWO and released under CC0

#include <avr/io.h> // For fast PWM

int i = 0;
int start_val = 0; // Bezier Curve Starting Point
int end_val = 255; // Bezier Curve end Point
float old_wait = 0;
float wait = 0; // Bezier curve x-axis (time)
float bz_val = 0; // Bezier curve y-axis (voltage)
int dev, level, curve, freq;
long timer = 0;
long timer1 = 0; // Analog read interval
float x[256]; // Bezier Curve Calculation Tables

int freq_rnd = 501;
int freq_dev = 40;
int chance[32] = {5, 12, 21, 33, 48, 67, 90, 118, 151, 189, 232, 279, 331, 386, 443, 501, 559, 616, 671, 723, 770, 813, 851, 884, 912, 935, 954, 969, 981, 990, 997, 1000}; // Normal distribution table
int freq_err[32] = {8, 9, 10, 11, 12, 13, 14, 15, 16, 18, 20, 22, 24, 26, 28, 30, 33, 36, 40, 46, 52, 58, 64, 70, 76, 82, 90, 98, 110, 122, 136, 148}; // Frequency Variation

int prevTrigState = LOW;
float holdValue = 0;
float noiseSeed = 0;
int baseFreq;
int fluctuateOffset = 0;

float perlinNoise(float x) {
    int xi = (int)x & 255;
    float xf = x - (int)x;
    float u = xf * xf * (3.0 - 2.0 * xf);

    float grad1 = (random(-100, 100) / 100.0);
    float grad2 = (random(-100, 100) / 100.0);
    
    float n0 = grad1 * xf;
    float n1 = grad2 * (xf - 1);
    
    return (1.0 - u) * n0 + u * n1;
}

void setup() {
    // Prepare Bezier Curve Calculation Tables
    for (int j = 0; j < 255; j++) {
        x[j] = j * 0.003921; // j / 255
    }

    pinMode(10, OUTPUT); // CV output
    pinMode(3, INPUT_PULLUP); // Set Trig (D3) as input with pull-up resistor
    timer = micros();
    timer1 = millis();

    // 9,10 pin PWM setting
    TCCR1B &= B11111000;
    TCCR1B |= B00000001;
    delay(50);
}

void loop() {
    int currentTrigState = digitalRead(3); // Read the trig input

    // Hold Input correction: retain last valid voltage instead of resetting to 0
    if (currentTrigState != prevTrigState) {
        if (currentTrigState == HIGH) {
            holdValue = bz_val * level / 255; // Hold the current voltage
        }
        prevTrigState = currentTrigState;
    }

    if (millis() - timer1 >= 50) { // timing condition
        freq = map(analogRead(A1), 0, 1023, 5000, 5); // Stretch (A1) now directly sets base frequency
        baseFreq = map(analogRead(A1), 0, 1023, 5000, 5);
        curve = map(analogRead(A3), 0, 1023, 0, 255); // Smooth (A3) controls transition shape
        level = analogRead(A0) / 4; // Elevate (A0) controls output level
        timer1 = millis();
    }

    if (timer + (wait - old_wait) <= micros()) {
        old_wait = wait;
        i++;

        if (i >= 255) { // Recalculation of target voltage values
            i = 0;
            start_val = end_val;
            end_val = random(0, 255);
            change_freq_error(); // Apply fluctuation
        }

        // Bezier Curve Calculations
        wait = 3 * pow((1 - x[i]), 2) * x[i] * curve + 
               3 * (1 - x[i]) * pow(x[i], 2) * (255 - curve) + 
               pow(x[i], 3) * 255;
        int effectiveFreq = max(1, baseFreq + fluctuateOffset);
        wait = max(1, 1 + wait * effectiveFreq * 3);
         // Ensure wait never goes negative or too slow
        bz_val = pow((1 - x[i]), 3) * start_val + 
                 3 * pow((1 - x[i]), 2) * x[i] * start_val + 
                 3 * (1 - x[i]) * pow(x[i], 2) * end_val + 
                 pow(x[i], 3) * end_val;

        timer = micros();
        PWM_OUT(); // PWM output
    }
}

// Fluctuate (A5) modifies randomness of frequency changes
// Use Perlin Noise for a organic, flowing modulation/motion instead of random jumps
void change_freq_error() {
    int deviation = map(analogRead(A5), 0, 1023, 0, 500);
    noiseSeed += 0.1;
    float noiseValue = perlinNoise(noiseSeed);
    fluctuateOffset = noiseValue * deviation;
}


void PWM_OUT() {
    if (prevTrigState == HIGH) {
        analogWrite(10, holdValue); // Output held value
    } else {
        analogWrite(10, bz_val * level / 255); // Output normal signal
    }
}
