#include <Adafruit_NeoPixel.h>

#define LED_NEOP 24  // PIN DEDICADO AL NEOPIXEL WS2812
#define LED_BUIL 25  // Led Builtin GPIO25

Adafruit_NeoPixel np = Adafruit_NeoPixel(11, LED_NEOP, NEO_GRB + NEO_KHZ800);  // Objeto NeoPixel

void setup() {
  pinMode(LED_BUIL, OUTPUT);  // Led Builtin GPIO25
}

void loop() {
  np.setBrightness(20);

  digitalWrite(LED_BUIL, HIGH);
  delay(2000); 
  digitalWrite(LED_BUIL, LOW); 
  delay(2000); 

}
