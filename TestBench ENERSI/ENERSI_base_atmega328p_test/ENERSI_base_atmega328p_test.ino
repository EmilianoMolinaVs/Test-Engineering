
#include <Arduino.h>

#define BLINK PD7

void setup() {
  pinMode(BLINK, OUTPUT);
}

void loop() {
  digitalWrite(BLINK, HIGH);
  delay(500);
  digitalWrite(BLINK, LOW);
  delay(500);
}
