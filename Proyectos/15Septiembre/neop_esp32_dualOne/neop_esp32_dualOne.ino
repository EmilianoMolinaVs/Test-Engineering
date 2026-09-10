/*
#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

#define RED 25  // On Trinket or Gemma, suggest changing this to 1
#define GREEN 26
#define BLUE 27

#define DELAYVAL 500  // Time (in milliseconds) to pause between pixels

void setup() {
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);
}

void loop() {
  digitalWrite(RED, HIGH);
  digitalWrite(BLUE, LOW);
  digitalWrite(GREEN, HIGH);
  delay(DELAYVAL);

  digitalWrite(RED, LOW);
  digitalWrite(BLUE, HIGH);
  digitalWrite(GREEN, HIGH);
  delay(DELAYVAL);

  digitalWrite(RED, HIGH);
  digitalWrite(BLUE, HIGH);
  digitalWrite(GREEN, LOW);
  delay(DELAYVAL);
}
*/



#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

#define PIN 21        // On Trinket or Gemma, suggest changing this to 1
#define NUMPIXELS 30  // Popular NeoPixel ring size

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

#define DELAYVAL 50  // Time (in milliseconds) to pause between pixels

void setup() {
  pixels.begin();  // INITIALIZE NeoPixel strip object (REQUIRED)
}

void loop() {
  pixels.clear();

  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 50, 0));
    pixels.show();
    delay(DELAYVAL);
  }

  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 50, 0));
    pixels.show();
    delay(DELAYVAL);
  }

  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 50, 0));
    pixels.show();
    delay(DELAYVAL);
  }
}
