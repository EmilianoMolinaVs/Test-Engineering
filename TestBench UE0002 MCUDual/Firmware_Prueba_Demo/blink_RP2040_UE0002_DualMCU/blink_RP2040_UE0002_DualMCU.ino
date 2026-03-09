
#include <Arduino.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <HardwareSerial.h>


#define NEOP_PIN 16
#define LED_BUIL 25


Adafruit_NeoPixel np = Adafruit_NeoPixel(1, NEOP_PIN, NEO_GRB + NEO_KHZ800);  // Objeto NeoPixel


// ==== Declaración de variables ====
String JSON_entrada;
StaticJsonDocument<200> receiveJSON;

String JSON_salida;  // Variable que envía el JSON de datos
StaticJsonDocument<200> sendJSON;

// ---- Prototipos de Funciones ----
void demoLED();


void setup() {

  Serial1.begin(115200);
  Serial2.begin(115200);

  pinMode(LED_BUIL, OUTPUT);

  np.begin();  // Inicialización de objeto NeoPixel
  np.setBrightness(20);
}



void loop() {

  if (Serial1.available()) {

    JSON_entrada = Serial1.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);

    if (!error) {
      String Function = receiveJSON["Function"];

      int opc = 0;
      if (Function == "ping") opc = 1;
      else if (Function == "passthrough") opc = 2;

      switch (opc) {
        case 1:
          {
            sendJSON.clear();
            sendJSON["ping"] = "pong";
            serializeJson(sendJSON, Serial1);
            Serial1.println();
            break;
          }

        case 2:
          {
            sendJSON.clear();
            sendJSON["ping"] = "pongRP2040";
            serializeJson(sendJSON, Serial1);
            Serial1.println();
            break;
          }

        default: break;
      }
    }
  }

  if (Serial.available()) {
    JSON_entrada = Serial.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);

    if (!error) {
      String Function = receiveJSON["Function"];

      int opc = 0;
      if (Function == "ping") opc = 1;

      switch (opc) {
        case 1:
          {
            sendJSON.clear();
            sendJSON["ping"] = "pong";
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }
      }
    }
  }


  if (!Serial.available() && !Serial1.available()) {
    demoLED();
  }
}



void demoLED() {

  int delay_ms = 200;

  digitalWrite(LED_BUIL, HIGH);
  np.setPixelColor(0, np.Color(255, 0, 0));
  np.show();
  delay(delay_ms);

  digitalWrite(LED_BUIL, LOW);
  np.setPixelColor(0, np.Color(0, 255, 0));
  np.show();
  delay(delay_ms);

  digitalWrite(LED_BUIL, HIGH);
  np.setPixelColor(0, np.Color(0, 0, 255));
  np.show();
  delay(delay_ms);

  digitalWrite(LED_BUIL, LOW);
  np.setPixelColor(0, np.Color(255, 255, 255));
  np.show();
  delay(delay_ms);
}
