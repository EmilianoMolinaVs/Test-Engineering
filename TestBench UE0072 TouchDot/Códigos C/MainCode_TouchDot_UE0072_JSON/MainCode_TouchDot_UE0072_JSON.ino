

// --- BIBLIOTECAS ---
#include <Wire.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>  // Required for 16 MHz Adafruit Trinket
#endif


// ---- Declaración de pines ----
#define PIN_SCL 6        // SCL pin para OLED
#define PIN_SDA 5        // SDA pin para OLED
#define PIN_NEOPIXEL 45  // PIN de Neopixel

#define PIN_1 1  // Pines Táctiles
#define PIN_2 2
#define PIN_3 3
#define PIN_4 4

#define PIN_8 8
#define PIN_9 9
#define PIN_10 10
#define PIN_11 11
#define PIN_17 17
#define PIN_18 18


// ---- Declaración de Constantes en Objetos ----
#define OLED_RESET -1     // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64

// ---- Declaración de Objetos ----
Adafruit_NeoPixel pixels(1, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ---- Declaración de variables ----
String status_OLED;

// ==== Variables de inicialización JSON
String JSON_entrada;  // Variable que recibe al JSON en crudo de PagWeb
StaticJsonDocument<200> receiveJSON;

String JSON_salida;  // Variable que envía el JSON de datos
StaticJsonDocument<200> sendJSON;

void setup() {

  // ==== Inicialización de Comunicación Serie ====
  Serial.begin(115200);
  Serial.println("Serial inicializado TouchDot...");

  // ==== Inicialización de Neopixel ====
  pixels.begin();  // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels.clear();

  // ==== Chequeo e inicialización de OLED por I2C ====
  Wire.begin(PIN_SDA, PIN_SCL);  // Inicialización de I2C de OLED

  if (!i2cCheckDevice(0x3C)) {
    Serial.println("SSD1306 no encontrada en I2C");
    status_OLED = "FAIL";
  } else {
    Serial.println("SSD1306 detectada");
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    status_OLED = "OK";
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, 0);
  display.println(F("Test de Prueba"));
  display.setCursor(30, 10);
  display.println(F("TouchDot"));
  display.display();  // Show initial text

  // ==== Declaración de Modos en GPIOs ====
  pinMode(LED_BUILTIN, OUTPUT);
  //pinMode(PIN_1, INPUT_PULLDOWN);
  //pinMode(PIN_2, INPUT_PULLDOWN);
  //pinMode(PIN_3, INPUT_PULLDOWN);
  //pinMode(PIN_4, INPUT_PULLDOWN);

  pinMode(PIN_8, INPUT);
  pinMode(PIN_9, INPUT);
  pinMode(PIN_10, INPUT);
  pinMode(PIN_11, INPUT);
  pinMode(PIN_17, INPUT);
  pinMode(PIN_18, INPUT);
}


void loop() {

  if (Serial.available()) {

    JSON_entrada = Serial.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);

    if (!error) {

      String Function = receiveJSON["Function"];

      int opc = 0;

      if (Function == "ping") opc = 1;       // {"Function":"ping"}
      else if (Function == "mac") opc = 2;   // {"Function":"mac"}
      else if (Function == "read") opc = 3;  // {"Function":"read", "PIN":"1"}

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
            sendJSON.clear();  // Limpia cualquier dato previo
            uint64_t chipid = ESP.getEfuseMac();

            // Formatear MAC a string tipo XX:XX:XX:XX:XX:XX
            char macStr[18];
            sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
                    (uint8_t)(chipid >> 40),
                    (uint8_t)(chipid >> 32),
                    (uint8_t)(chipid >> 24),
                    (uint8_t)(chipid >> 16),
                    (uint8_t)(chipid >> 8),
                    (uint8_t)(chipid));

            sendJSON["mac"] = macStr;

            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }

        case 3:
          {
            if (receiveJSON.containsKey("PIN")) {

              uint8_t NoPin = atoi(receiveJSON["PIN"]);
              pinMode(NoPin, INPUT);

              delay(50);
              int value = 0;
              int items = 10;

              for (int i = 0; i < items; i++) {
                value += analogRead(NoPin);
                delay(20);
              }

              int average = value / items;

              sendJSON.clear();
              sendJSON["PIN"] = NoPin;
              sendJSON["value"] = average;

              serializeJson(sendJSON, Serial);
              Serial.println();
            } else {
              sendJSON.clear();
              sendJSON["error"] = "PIN not provided";
              serializeJson(sendJSON, Serial);
              Serial.println();
            }

            break;
          }

        default:
          Serial.print("Opción no válida");
          break;
      }
    }


  }

  else if (analogRead(PIN_1) > 1100) {
    pixels.setPixelColor(0, pixels.Color(20, 0, 0));
    pixels.show();
  } else if (analogRead(PIN_2) > 1100) {
    pixels.setPixelColor(0, pixels.Color(0, 20, 0));
    pixels.show();
  }

  else {
  }

}

// Función de escritura de status en OLED D2 y D3
bool i2cCheckDevice(uint8_t address) {
  Wire.beginTransmission(address);
  byte error = Wire.endTransmission();
  return (error == 0);
}


void ledDemo() {
  int tm_delay = 150;

  digitalWrite(LED_BUILTIN, HIGH);
  pixels.setPixelColor(0, pixels.Color(20, 0, 0));
  pixels.show();
  delay(tm_delay);

  digitalWrite(LED_BUILTIN, LOW);
  pixels.setPixelColor(0, pixels.Color(0, 20, 0));
  pixels.show();
  delay(tm_delay);

  digitalWrite(LED_BUILTIN, HIGH);
  pixels.setPixelColor(0, pixels.Color(0, 0, 20));
  pixels.show();
  delay(tm_delay);

  digitalWrite(LED_BUILTIN, LOW);
  pixels.setPixelColor(0, pixels.Color(20, 0, 20));
  pixels.show();
  delay(tm_delay);
}