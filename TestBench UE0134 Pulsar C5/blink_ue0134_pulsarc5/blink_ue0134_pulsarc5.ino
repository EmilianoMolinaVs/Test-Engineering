/*
Firmware test base v1.0 Pulsar C5 

Funcionalidades: 

- Control de blink y neopixel 
- Lectura de direcciones I2C y manejo de sensores nativos 
  . MAX17048 Addr 0x36      | Charger Battery Sensor
  . MMC5603 Addr 0x30       | 
  . SHT40 Addr 0x44 XDDDD   | Humidity and Temperature Sensor
  . LPS22DF Addr 0x5C
  . VCNL4040 Addr 0x60

  34 60 18
*/

// ==== BIBLIOTECAS ====
#include <Wire.h>
#include <WiFi.h>
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
      Serial.println("Halo murió en Reach...");
    } else {

      int opc = 0;
      String Function = receiveJSON["Function"];

      if (Function == "ping") opc = 1;           // {"Function":"ping"}
      else if (Function == "mac") opc = 2;       // {"Function":"mac"}
      else if (Function == "i2c_scan") opc = 3;  // {"Function":"i2c_scan"}

      switch (opc) {
        case 1:
          {
            sendJSON.clear();
            sendJSON["ping"] = "pong";
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }

        case 2:
          {
            sendJSON.clear();
            WiFi.mode(WIFI_MODE_STA);
            String mac = WiFi.macAddress();
            sendJSON["mac"] = mac;
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }


        case 3:
          {
            for (uint8_t addr = 1; addr < 127; addr++) {
              Wire.beginTransmission(addr);
              if (Wire.endTransmission() == 0) {
                Serial.print("Dispositivo en 0x");
                Serial.println(addr, HEX);
              }
              delay(10);
            }
            break;
          }

        default:
          sendJSON.clear();
          sendJSON["error"] = "InvalidInput";
          sendJSON["msg"] = "invalid option";
          serializeJson(sendJSON, Serial);
          Serial.println();
          break;
      }
    }
  } else {
    demo();
  }
}