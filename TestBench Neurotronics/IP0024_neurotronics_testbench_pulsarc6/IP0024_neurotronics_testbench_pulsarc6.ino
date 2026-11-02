/*
TestBench Neurotronix con Pulsar C6

Validación de UART y Accionamiento de Relevadores por GPIOS

Este firmware se carga en una pulsar c6 conectada en sus gpios al conector de 40 pines de la 
tarjeta neurotronics para comunicar uart, alimentación, gnd y gpios
Recibe instrucciones JSON por UART proveniente del frontend a traves de un ch340 conectado
a un arnes con db9 y convertidor ttl a rs232 hembra

*/

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <HardwareSerial.h>

#define RX2 D0  // >> GPIO15 como RX de UART2 - Recepción de datos desde interfaz web
#define TX2 D1  // >> GPIO19 como TX de UART2 - Transmisión de datos a interfaz web

#define E1 9
#define E2 15
#define E3 19
#define E4 20
#define E5 21
#define E6 18
#define E7 7
#define E8 2
#define E9 6
#define E10 14
#define E11 0
#define E12 1

const int enable[] = { 9, 19, 21, 15, 20, 18, 7, 6, 0, 2, 14, 1 };
int noEnable = sizeof(enable) / sizeof(enable[0]);

HardwareSerial PagWeb(1);  // Crear objeto para UART2 en PULSAR como PagWeb

String JSON_entrada;                   ///< Buffer para recibir JSON desde PagWeb
StaticJsonDocument<1024> receiveJSON;  ///< Documento JSON para parsear datos recibidos

String JSON_salida;                 ///< Buffer para transmitir JSON de respuesta
StaticJsonDocument<1024> sendJSON;  ///< Documento JSON para armar respuestas


// ==== FUNCIONES DE UTILIDAD ====
void serialDebug(String str) {
  str.replace("\"", "\\\"");  // Escapa comillas para JSON válido
  Serial.println("{\"debug\": \"" + str + "\"}");
}

void pagwebDebug(String str) {
  str.replace("\"", "\\\"");  // Escapa comillas
  PagWeb.println("{\"debug\": \"" + str + "\"}");
}


void setup() {

  // ==== Inicialización de Comunicación Serie ====
  Serial.begin(115200);                        // Comunicación serial para depuración
  PagWeb.begin(115200, SERIAL_8N1, RX2, TX2);  // UART para interfaz web
  delay(100);
  serialDebug("Serial Initialized...");
  pagwebDebug("Test Multi LM2596 Initialized...");

  for (int i = 0; i < noEnable; i++) {
    pinMode(enable[i], OUTPUT);
    delay(50);
    digitalWrite(enable[i], LOW);
  }
}

void loop() {

  if (PagWeb.available()) {

    JSON_entrada = PagWeb.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);

    if (error) {
      sendJSON.clear();
      sendJSON["state"] = "FAIL";
      sendJSON["error"] = error.c_str();
      serializeJson(sendJSON, Serial);
      Serial.println();

      serializeJson(sendJSON, PagWeb);
      PagWeb.println();
    } else {
      String Function = receiveJSON["Function"];
      int j = receiveJSON["Relay"] | 0;

      int opc = 0;
      if (Function == "ping") opc = 1;        // {"Function":"ping"}
      else if (Function == "on") opc = 2;     // {"Function":"on", "Relay": 1}
      else if (Function == "off") opc = 3;    // {"Function":"off"}
      else if (Function == "sweep") opc = 4;  //  {"Function":"sweep"}

      switch (opc) {
        case 1:
          {
            sendJSON.clear();
            sendJSON["ping"] = "pong";
            serializeJson(sendJSON, PagWeb);
            PagWeb.println();
            break;
          }

        case 2:
        case 3:
          {
            sendJSON.clear();
            uint8_t state = (opc == 3) ? LOW : HIGH;
            digitalWrite(enable[j], state);
            break;
          }

        case 4:
          for (int k = 0; k < noEnable; k++) {
            digitalWrite(enable[k], HIGH);
            delay(200);
          }

          delay(1000);
          for (int d = 11; d >= 0; d--) {
            digitalWrite(enable[d], LOW);
            delay(200);
          }
          break;


        default: break;
      }
    }
  }

  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    PagWeb.println(cmd);
  }
}
