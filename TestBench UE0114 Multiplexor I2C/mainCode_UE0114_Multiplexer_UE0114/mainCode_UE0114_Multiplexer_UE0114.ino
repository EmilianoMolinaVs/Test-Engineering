/* 
MainCode UE0114 TestBench DevLab: I2C TCA9548A Multiplexer Module

*/

#include <Wire.h>
#include <HardwareSerial.h>
#include <Arduino.h>
#include <ArduinoJson.h>


// ==== CONFIGURACIÓN DE PINES ====
#define SDA_PIN 6
#define SCL_PIN 7


// ==== VARIABLES DE CONFIGURACIÓN ====
uint8_t ADD[8] = { 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77 };  // Direcciones del Multiplexor


String JSON_entrada;                  ///< Buffer para recibir JSON desde PagWeb
StaticJsonDocument<256> receiveJSON;  ///< Documento JSON para parsear datos recibidos

String JSON_lectura;               ///< Buffer para transmitir JSON de respuesta
StaticJsonDocument<256> sendJSON;  ///< Documento JSON para armar respuestas



void printDebug(String str) {
  str.replace("\"", "\\\"");  // Escapa comillas
  Serial.println("{\"debug\": \"" + str + "\"}");
}

void tcaselect(uint8_t i) {
  if (i > 7) return;

  Wire.beginTransmission(ADD[i]);
  Wire.write(1 << i);
  Wire.endTransmission();
}

void setup() {

  // ==== Inicialización de comunicación serie ====
  Serial.begin(115200);
  delay(100);
  printDebug("Serial initialized...");

  // ==== Inicialización de comunicación I2C ====
  Wire.begin(SDA_PIN, SCL_PIN);
  printDebug("I2C Multiplexer...");
}

void loop() {

  if (Serial.available()) {

    JSON_entrada = Serial.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);

    if (!error) {

      String Function = receiveJSON["Function"];

      int opc = 0;
      if (Function == "ping") opc = 1;          // {"Function":"ping"}
      else if (Function == "scanAdd") opc = 2;  // {"Function":"scanAdd"}

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

            // Creamos un arreglo JSON para almacenar las direcciones encontradas
            JsonArray foundMuxes = sendJSON.createNestedArray("tca_found");

            // Hacemos el barrido únicamente sobre las direcciones configuradas del Mux
            for (uint8_t i = 0; i < 8; i++) {
              Wire.beginTransmission(ADD[i]);
              byte error = Wire.endTransmission();

              if (error == 0) {
                // Si el dispositivo responde (ACK), formateamos la dirección a string HEX (ej. "0x70")
                char hexAddr[5];
                sprintf(hexAddr, "0x%02X", ADD[i]);
                foundMuxes.add(hexAddr);
              }
            }

            // Agregamos un flag de estado para facilitar el parseo en tu frontend/script de control
            if (foundMuxes.size() > 0) {
              sendJSON["status"] = "success";
            } else {
              sendJSON["status"] = "not_found";
            }

            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }


        default: break;
      }
    }
  }
}
