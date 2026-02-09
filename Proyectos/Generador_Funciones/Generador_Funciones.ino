/*
Código básico para Generar Funciones Seno/Triangular/Cuadrada empleando el generador MD_AD9833
por medio de JSON en Monitor Serie
*/

#include <Arduino.h>
#include <ArduinoJson.h>
#include <MD_AD9833.h>
#include <SPI.h>

// ==== Declaración de pines de comunicación SPI ====
const uint8_t PIN_DATA = 7;    ///< SPI Data pin number
const uint8_t PIN_CLK = 6;     ///< SPI Clock pin number
const uint8_t PIN_FSYNC = 18;  ///< SPI Load pin number (FSYNC in AD9833 usage)

// ==== Creación de Objeto Gen Func ====
MD_AD9833 AD(PIN_DATA, PIN_CLK, PIN_FSYNC);

// ==== Declaración de JSON====
String JSON_entrada;
StaticJsonDocument<200> receiveJSON;

String JSON_salida;  // Variable que envía el JSON de datos
StaticJsonDocument<200> sendJSON;

// ==== Declaración de variables y constantes ====


void setup(void) {
  Serial.begin();
  Serial.println("Serial Inicializado...");

  AD.begin();
  AD.setMode(MD_AD9833::MODE_OFF);
}

void loop(void) {

  if (Serial.available()) {

    JSON_entrada = Serial.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);

    if (!error) {
      String Function = receiveJSON["Function"];
      int opc = 0;

      if (Function == "Sine") opc = 1;       // {"Function":"Sine"}
      else if (Function == "Sqr1") opc = 2;  // {"Function":"Sqr1"}
      else if (Function == "Sqr2") opc = 3;  // {"Function":"Sqr2"}
      else if (Function == "Trg") opc = 4;   // {"Function":"Trg"}
      else if (Function == "Off") opc = 5;   // {"Function":"Off"}

      switch (opc) {
        case 1:
          {
            AD.setMode(MD_AD9833::MODE_SINE);
            break;
          }

        case 2:
          {
            AD.setMode(MD_AD9833::MODE_SQUARE1);
            break;
          }

        case 3:
          {
            AD.setMode(MD_AD9833::MODE_SQUARE2);
            break;
          }

        case 4:
          {
            AD.setMode(MD_AD9833::MODE_TRIANGLE);
            break;
          }

        case 5:
          {
            AD.setMode(MD_AD9833::MODE_OFF);
            break;
          }
      }
    }
  }
}
