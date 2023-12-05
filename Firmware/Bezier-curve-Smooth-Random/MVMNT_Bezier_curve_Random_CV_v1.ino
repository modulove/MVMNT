#include <avr/io.h>

int i = 0;
int start_val = 0;
int end_val = 255;

// with integer types
int old_wait = 0;
int wait = 0;
int bz_val = 0;
int dev, level, curve, freq;
unsigned long timer = 0;
unsigned long timer1 = 0;
float x[256];

const float WAIT_SCALE = 0.7;

int freq_rnd = 501;
int freq_dev = 40;
int chance[32] = {5, 12, 21, 33, 48, 67, 90, 118, 151, 189, 232, 279, 331, 386, 443, 501, 559, 616, 671, 723, 770, 813, 851, 884, 912, 935, 954, 969, 981, 990, 997, 1000};
int freq_err[32] = {8, 9, 10, 11, 12, 13, 14, 15, 16, 18, 20, 22, 24, 26, 28, 30, 33, 36, 40, 46, 52, 58, 64, 70, 76, 82, 90, 98, 110, 122, 136, 148};

uint16_t seed;

// previous state
int prevTrigState = LOW;
float holdValue = 0;
unsigned long lastTrigTime = 0; // New variable for debounce

void setup() {
  //Serial.begin(57600); // Uncomment for debugging
  for (unsigned int j = 0; j < 256; j++) {
    x[j] = j * 0.003921;
  }

  pinMode(10, OUTPUT);
  pinMode(3, INPUT_PULLUP); // Set Pin D3 as input with internal pull-up resistor
  timer = micros();
  timer1 = millis();
  TCCR1B &= B11111000;
  TCCR1B |= B00000001;
  delay(50);
  randomSeed(analogRead(0));
}

void loop() {
  unsigned long startTime = micros();

  int currentTrigState = digitalRead(3); // Read the trig input

  // Enough time has passed since the last state change?
  if (millis() - lastTrigTime > 5) {
    // trig state has changed?
    if (currentTrigState != prevTrigState) {
      // If Trig is held high store the current output
      if (currentTrigState == HIGH) {
        holdValue = bz_val * level / 255;
      }
      // reset holdValue
      else {
        holdValue = 0;
      }
      prevTrigState = currentTrigState;
      lastTrigTime = millis(); // time of the last state change
    }
  }

  if (timer1 + 50 < millis()) {
    freq = (511 - min(511, (analogRead(5) / 2))) * freq_dev; // fluctuate rate
    curve = min(255, (analogRead(3) / 2));                  // linear to Bezier
    level = analogRead(0) / 4;
    timer1 = millis();
  }

  if (timer + (wait - old_wait) <= micros()) {
    old_wait = wait;
    i++;

    i = i % 256;
    if (i == 0) {
      start_val = end_val;
      end_val = random(256);
      change_freq_error();
    }

    // Bezier Curve Calculations
    wait = 3 * pow((1 - x[i]), 2) * x[i] * curve + 3 * (1 - x[i]) * pow(x[i], 2) * (255 - curve) + pow(x[i], 3) * 255;
    wait = 1 + wait * freq * 2;
    wait *= WAIT_SCALE;
    bz_val = pow((1 - x[i]), 3) * start_val + 3 * pow((1 - x[i]), 2) * x[i] * start_val + 3 * (1 - x[i]) * pow(x[i], 2) * end_val + pow(x[i], 3) * end_val;

    timer = micros();
    PWM_OUT();
    //debug();
  }
}

// Stretch
void change_freq_error() {
  dev = map(analogRead(1), 0, 1023, 0, 500);
  freq_rnd = random(500 - dev, 500 + dev);
  for (int k = 0; k < 32; k++) {
    if (freq_rnd >= chance[k] && freq_rnd < chance[k + 1]) {
      freq_dev = freq_err[k];
    }
  }
}

void PWM_OUT() {
  if (prevTrigState == HIGH) {
    analogWrite(10, holdValue);
  } else {
    analogWrite(10, bz_val * level / 255);
  }
}

void debug() {
  int pwm = bz_val * level / 255;
  // Pot1 = Level (Elevate), Pot2 = FREQency change (stretch), Pot3 = linear / Bezier (Smooth), Pot4 rate (Fluctuate)
  Serial.print("Elevation:");
  Serial.print(level);
  Serial.print(",");
  Serial.print("Frequency:");
  Serial.print(freq);
  Serial.print(",");
  Serial.print("Smooth:");
  Serial.print(curve);
  Serial.print(",");
  Serial.print("Freq_dev:");
  Serial.print(freq_dev);
  Serial.print(",");
  Serial.print("Sigma:");
  Serial.print(dev);
  Serial.print(",");
  Serial.print("RAND_FREQ:");
  Serial.print(freq_rnd);
  Serial.print(",");
  Serial.print("DevPot:");
  Serial.print(analogRead(1));
  Serial.print(",");
  Serial.print("wait:");
  Serial.print(wait);
  Serial.print(",");
  Serial.print("OUTPUT:");
  Serial.println(pwm);
}