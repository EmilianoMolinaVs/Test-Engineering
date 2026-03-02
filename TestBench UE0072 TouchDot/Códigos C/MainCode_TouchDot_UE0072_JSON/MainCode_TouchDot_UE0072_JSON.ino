// --- BIBLIOTECAS ---
#include <Wire.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include <Arduino.h>

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>  // Required for 16 MHz Adafruit Trinket
#endif



#define PIN_NEOPIXEL 45  // PIN de Neopixel

Adafruit_NeoPixel pixels(1, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);


void setup() {
  Serial.begin(115200);
  Serial.println("Serial inicializado TouchDot...");

  pixels.begin();  // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels.clear();



  pinMode(LED_BUILTIN, OUTPUT);
}


void loop() {
  ledBUILTIN();
}


void ledBUILTIN() {
  digitalWrite(LED_BUILTIN, HIGH);
  pixels.setPixelColor(0, pixels.Color(20, 0, 0));
  pixels.show(); 
  delay(200);    

  digitalWrite(LED_BUILTIN, LOW);
  pixels.setPixelColor(0, pixels.Color(0, 20, 0));
  pixels.show();  
  delay(200);     

  digitalWrite(LED_BUILTIN, HIGH);
  pixels.setPixelColor(0, pixels.Color(0, 0, 20));
  pixels.show();  
  delay(200);    

  digitalWrite(LED_BUILTIN, LOW);
  pixels.setPixelColor(0, pixels.Color(20, 0, 20));
  pixels.show(); 
  delay(200);    
}