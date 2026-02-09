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
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
static bool bOff = false;
static uint8_t m = 0;
static MD_AD9833::mode_t modes[] = {
  MD_AD9833::MODE_TRIANGLE,
  MD_AD9833::MODE_SQUARE2,
  MD_AD9833::MODE_SINE,
  MD_AD9833::MODE_SQUARE1
};

void setup(void) {
  Serial.begin();
  Serial.println("Serial Inicializado...");

  AD.begin();
}

void loop(void) {

  if (Serial.available()) {

    JSON_entrada = Serial.readStringUntil('\n'); 

    char cmd = Serial.read();
    int opc = 0;

    if (cmd == 'S') opc = 1;
    else if (cmd == 'a') opc = 2;
    else if (cmd == 'b') opc = 3;
    else if (cmd == 'T') opc = 4;
    else if (cmd == 'O') opc = 5;

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
