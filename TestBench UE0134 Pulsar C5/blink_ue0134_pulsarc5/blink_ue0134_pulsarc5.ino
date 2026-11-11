/*
Firmware test base v1.0 Pulsar C5 

Funcionalidades: 

- Control de blink y neopixel 
*/

// ==== BIBLIOTECAS ====
#include <Wire.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_SSD1306.h>

// ==== DECLARACIÓN DE GPIOS ====
#define NEOP_PIN 0
#define LED_BUILTIN 27

// ==== MACROS Y VARIABLES GLOBALES ====
#define NUMPIXELS 1    // Número de Neopixeles
int intensity = 30;    // Intensidad de brillo en Neopixel
int delay_neop = 250;  // Tiempo de actualización en Neopixel

Adafruit_NeoPixel pixels(NUMPIXELS, NEOP_PIN, NEO_GRB + NEO_KHZ800);

// ==== FUNCIONALIDADES ====
void demo() {

  // ---- Arranque de Neopixel ----
  digitalWrite(LED_BUILTIN, HIGH);
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(intensity, 0, 0));
    pixels.show();
    delay(delay_neop);
  }

  digitalWrite(LED_BUILTIN, LOW);
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(0, intensity, 0));
    pixels.show();
    delay(delay_neop);
  }

  digitalWrite(LED_BUILTIN, HIGH);
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 0, intensity));
    pixels.show();
    delay(delay_neop);
  }

  digitalWrite(LED_BUILTIN, LOW);
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 130, intensity));
    pixels.show();
    delay(delay_neop);
  }

  pixels.clear();
}

void messageIntro() {
  delay(1000);  // Pequeña pausa para que el usuario alcance a leerlo
  Serial.println("========================================================");
  Serial.println("*                                                      *");
  Serial.println("*    🚀 ¡HOLA! BIENVENIDO A TU PULSAR C5 🚀      *");
  Serial.println("*                                                      *");
  Serial.println("========================================================");
  Serial.println("*  Tu nueva placa de desarrollo esta lista para usar.  *");
  Serial.println("*  Esperamos que la disfrutes al máximo! :D            *");
  Serial.println("========================================================");
  delay(500);  // Pequeña pausa para que el usuario alcance a leerlo
  Serial.print("\n>> Iniciando proceso de hackeo de computadora");
  for (int i = 0; i < 10; i++) {
    Serial.print(".");
    delay(100);
  }
  Serial.print("\n>> Computadora hackeada");
  for (int i = 0; i < 3; i++) {
    Serial.print(".");
    delay(300);
  }
  Serial.print("\n>> Borrando archivos de sistema");
  for (int i = 0; i < 10; i++) {
    Serial.print(".");
    delay(100);
  }
  delay(1000);
  Serial.println("\n Bromita! XDD");
  Serial.println("¡Disfruta de tu nueva PULSAR C5 de 🚀 UNIT Electronics 🚀 :D!");
}


void setup() {

  // ==== Inicializaciones ====
  Serial.begin(115200);

  pixels.clear();
  pixels.setBrightness(150);

  // ==== Inicialización de GPIOs ====
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // ==== Mensaje de Bienvenida ====
 // messageIntro();
}

void loop() {
  demo();
}
