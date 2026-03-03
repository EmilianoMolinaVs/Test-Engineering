

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

#define BUTTON_PIN 0  // Boton
#define PIN_1 1       // Pines Exteriores
#define PIN_2 2
#define PIN_3 3
#define PIN_4 4
#define PIN_8 8
#define PIN_9 9
#define PIN_10 10
#define PIN_11 11
#define PIN_17 17
#define PIN_18 18

#define PIN_D15 D15  // Pines interiores
#define PIN_D17 D17
#define PIN_D19 D19
#define PIN_D27 D27
#define PIN_D16 D16
#define PIN_D18 D18
#define PIN_D20 D20
#define PIN_D28 D28

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

uint32_t colors[] = {
  pixels.Color(128, 0, 0),
  pixels.Color(0, 128, 0),
  pixels.Color(0, 0, 128),
  pixels.Color(128, 128, 0),
  pixels.Color(0, 128, 128),
  pixels.Color(128, 0, 128),
  pixels.Color(128, 128, 128),
  pixels.Color(64, 64, 0),
  pixels.Color(0, 64, 64),
  pixels.Color(64, 0, 64),
  pixels.Color(0, 0, 0)
};

int colorIndex = 0;

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
    //Serial.println("SSD1306 detectada");
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
  pinMode(BUTTON_PIN, INPUT_PULLUP);
}


void loop() {

  bool currentButton = digitalRead(BUTTON_PIN);

  if (currentButton == LOW) {
    colorIndex = (colorIndex + 1) % (sizeof(colors) / sizeof(colors[0]));
    pixels.setPixelColor(0, colors[colorIndex]);
    pixels.show();
    delay(1000);
  }

  if (Serial.available()) {

    JSON_entrada = Serial.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);

    if (!error) {

      String Function = receiveJSON["Function"];

      int opc = 0;

      if (Function == "ping") opc = 1;          // {"Function":"ping"}
      else if (Function == "mac") opc = 2;      // {"Function":"mac"}
      else if (Function == "gpioIn") opc = 3;   // {"Function":"gpioIn"}
      else if (Function == "gpioOut") opc = 4;  // {"Function":"gpioOut"}

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

          // Testeo de gpios dentro de la PCB
        case 3:
          {
            sendJSON.clear();  // Limpia cualquier dato previo

            String state1 = testGpios(PIN_D15, PIN_D17);
            sendJSON["Int1"] = state1;

            String state2 = testGpios(PIN_D19, PIN_D27);
            sendJSON["Int2"] = state2;

            String state3 = testGpios(PIN_D16, PIN_D18);
            sendJSON["Int3"] = state3;

            String state4 = testGpios(PIN_D20, PIN_D28);
            sendJSON["Int4"] = state4;

            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }

          // Testeo de gpios en el exterior de la PCB
        case 4:
          {
            sendJSON.clear();  // Limpia cualquier dato previo

            String state1 = testGpios(PIN_1, PIN_2);
            sendJSON["Ext1"] = state1;

            String state2 = testGpios(PIN_3, PIN_4);
            sendJSON["Ext2"] = state2;

            String state3 = testGpios(PIN_8, PIN_9);
            sendJSON["Ext3"] = state3;

            String state4 = testGpios(PIN_10, PIN_11);
            sendJSON["Ext4"] = state4;

            String state5 = testGpios(PIN_17, PIN_18);
            sendJSON["Ext5"] = state5;

            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }



        default:
          Serial.print("Opción no válida");
          break;
      }
    }


  } else {
    ledDemo();
  }
}

// Función de escritura de status en OLED D2 y D3
bool i2cCheckDevice(uint8_t address) {
  Wire.beginTransmission(address);
  byte error = Wire.endTransmission();
  return (error == 0);
}

String testGpios(uint8_t gpioA, uint8_t gpioB) {

  bool resultAB = testSequence(gpioA, gpioB);
  if (!resultAB) return "Fail";

  delay(10);

  bool resultBA = testSequence(gpioB, gpioA);
  if (!resultBA) return "Fail";

  return "OK";
}

bool testSequence(uint8_t gpioOut, uint8_t gpioIn) {

  pinMode(gpioOut, OUTPUT);
  pinMode(gpioIn, INPUT_PULLDOWN);  // Declaración de GPIO con PULLDOWN

  uint8_t testPattern[] = { 1, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1, 1 };

  for (int i = 0; i < sizeof(testPattern); i++) {

    digitalWrite(gpioOut, testPattern[i]);
    delay(10);

    int readValue = digitalRead(gpioIn);

    if (readValue != testPattern[i]) {
      return false;
    }
  }

  return true;
}


void ledDemo() {
  int tm_delay = 100;

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