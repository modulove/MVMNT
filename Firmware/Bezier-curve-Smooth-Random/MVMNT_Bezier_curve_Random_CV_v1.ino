#include <avr/io.h>

int i = 0;
int start_val = 0;
int end_val = 255;
float old_wait = 0;
float wait = 0;
float bz_val = 0;
int dev, level, curve, freq;
long timer = 0;
long timer1 = 0;
float x[256];

int freq_rnd = 501;
int freq_dev = 40;
int chance[32] = {5, 12, 21, 33, 48, 67, 90, 118, 151, 189, 232, 279, 331, 386, 443, 501, 559, 616, 671, 723, 770, 813, 851, 884, 912, 935, 954, 969, 981, 990, 997, 1000};
int freq_err[32] = {8, 9, 10, 11, 12, 13, 14, 15, 16, 18, 20, 22, 24, 26, 28, 30, 33, 36, 40, 46, 52, 58, 64, 70, 76, 82, 90, 98, 110, 122, 136, 148};

uint16_t seed;

// hold previous state
int prevTrigState = LOW;
float holdValue = 0;

void setup() {
  for (unsigned int j = 0; j < 255; j++) {
    x[j] = j * 0.003921;
  }
  pinMode(10, OUTPUT);
  pinMode(3, INPUT_PULLUP);
  timer = micros();
  timer1 = millis();
  TCCR1B &= B11111000;
  TCCR1B |= B00000001;
  delay(50);
}

void loop() {
  int currentTrigState = digitalRead(3);

  if (currentTrigState != prevTrigState) {
    if (currentTrigState == HIGH) {
      holdValue = bz_val * level / 255;
    } else {
      holdValue = 0;
    }
    prevTrigState = currentTrigState;
  }

  if (timer1 + 50 < millis()) {
    freq = min(511, (analogRead(5) / 4)) * freq_dev;
    curve = min(255, (analogRead(3) / 2));
    level = analogRead(0) / 4;
    timer1 = millis();
  }

  if (timer + (wait - old_wait) <= micros()) {
    old_wait = wait;
    i++;

    if (i >= 255) {
      i = 0;
      start_val = end_val;
      end_val = random(0, 255);
      change_freq_error();
    }

    wait = 3 * pow((1 - x[i]), 2) * x[i] * curve + 3 * (1 - x[i]) * pow(x[i], 2) * (255 - curve) + pow(x[i], 3) * 255;
    wait = 1 + wait * freq * 2;
    bz_val = pow((1 - x[i]), 3) * start_val + 3 * pow((1 - x[i]), 2) * x[i] * start_val + 3 * (1 - x[i]) * pow(x[i], 2) * end_val + pow(x[i], 3) * end_val;

    timer = micros();
    PWM_OUT();
  }
}

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
