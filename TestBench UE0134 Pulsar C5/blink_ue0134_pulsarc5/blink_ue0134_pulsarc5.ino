/*
Firmware test base v1.0 Pulsar C5 

Funcionalidades: 

- Control de blink y neopixel 
- Lectura de direcciones I2C y manejo de sensores nativos 
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
#define SDA_PIN 2
#define SCL_PIN 3

// ==== MACROS Y VARIABLES GLOBALES ====
#define NUMPIXELS 1    // Número de Neopixeles
int intensity = 30;    // Intensidad de brillo en Neopixel
int delay_neop = 250;  // Tiempo de actualización en Neopixel

String JSON_entrada;  // Variable que recibe JSON del frontend
String JSON_salida;   // Variable que envía el JSON de datos

// ==== CREACIÓN DE OBJETOS ====
Adafruit_NeoPixel pixels(NUMPIXELS, NEOP_PIN, NEO_GRB + NEO_KHZ800);
StaticJsonDocument<254> receiveJSON;
StaticJsonDocument<254> sendJSON;

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

  Wire.begin(SDA_PIN, SCL_PIN);

  pixels.clear();
  pixels.setBrightness(150);

  // ==== Inicialización de GPIOs ====
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // ==== Mensaje de Bienvenida ====
  // messageIntro();
}

void loop() {


  if (Serial.available()) {

    JSON_entrada = Serial.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);

    if (error) {
      sendJSON.clear();
      sendJSON["error"] = error.c_str();
      sendJSON["message"] = "JSON invalido recibido";
      serializeJson(sendJSON, Serial);
      Serial.println();
    } else {

      int opc = 0;
      String Function = receiveJSON["Function"];

      if (Function == "ping") opc = 1;  // {"Function":"ping"}

      switch (opc) {
        case 1:
          {
            sendJSON.clear();
            sendJSON["ping"] = "pong";
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }


        default: break;
      }
    }







  } else {
    demo();
  }
}