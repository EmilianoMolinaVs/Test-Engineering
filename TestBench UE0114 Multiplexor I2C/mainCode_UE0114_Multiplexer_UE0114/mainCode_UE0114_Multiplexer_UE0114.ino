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
#define A0_PIN 15  // --> GPIO15 para control de pin A0 Selector de Dirección I2C
#define A1_PIN 19  // --> GPIO19 para control de pin A1 Selector de Dirección I2C
#define A2_PIN 20  // --> GPIO20 para control de pin A2 Selector de Dirección I2C


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

  // ==== Declaración de GPIOS ====
  // ---- Salidas ----
  pinMode(A0_PIN, OUTPUT);
  pinMode(A1_PIN, OUTPUT);
  pinMode(A2_PIN, OUTPUT);
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
            JsonArray foundMuxes = sendJSON.createNestedArray("mux_addr");

            printDebug("Scanning dynamic TCA addresses...");

            // Iteramos sobre las 8 combinaciones posibles de hardware
            for (int step = 0; step < 8; step++) {

              // 1. Configuramos los pines lógicos
              digitalWrite(A0_PIN, (step >> 0) & 1);
              digitalWrite(A1_PIN, (step >> 1) & 1);
              digitalWrite(A2_PIN, (step >> 2) & 1);

              // 10ms de delay para que los niveles lógicos de 3.3V y el TCA se asienten
              delay(10);

              // 2. Verificamos SI responde a la dirección ESPERADA para esta combinación
              Wire.beginTransmission(ADD[step]);
              byte error = Wire.endTransmission();

              if (error == 0) {
                // Formateo usando String para que ArduinoJson reserve la memoria de forma segura
                String hexStr = "0x" + String(ADD[step], HEX);
                foundMuxes.add(hexStr);
              }
            }

            // Validamos que hayamos encontrado al menos una respuesta
            if (foundMuxes.size() == 8) {
              sendJSON["Result"] = "OK";
            } else {
              sendJSON["Result"] = "FAIL";
            }

            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }

        case 3:
          {
            sendJSON.clear();
            break;
          }


        default: break;
      }
    }
  }
}
