/*
Este firmware se encarga del control y accionamiento de relevadores y comandos para 
la ejecución del testbench ups enersi 
*/

// ====   BIBLIOTECAS ====
#include <Arduino.h>
#include <ArduinoJson.h>

// ==== CREACIÓN DE OBJETOS ====
StaticJsonDocument<512> receiveJSON;  ///< Documento JSON para parsear datos recibidos
StaticJsonDocument<512> sendJSON;     ///< Documento JSON para armar respuestas

int relay[] = { D0, D1, 9, 15, 19, 20, 21, 18 };
int noRelays = sizeof(relay) / sizeof(relay[0]);

int enable[] = { 7, 2 };

void setup() {

  // ==== INICIALIZACIONES ====
  Serial.begin(115200);
  delay(100);

  StaticJsonDocument<48> doc;
  doc["test"] = "ENERSI UPS";
  doc["state"] = "ready";
  serializeJson(doc, Serial);
  Serial.println();

  // ==== ENTRADAS Y SALIDAS DEL ESP32 ====
  for (int i = 0; i < noRelays; i++) {
    pinMode(relay[i], OUTPUT);
    delay(10);
    digitalWrite(relay[i], HIGH);
  }

  pinMode(enable[0], OUTPUT);
  pinMode(enable[1], OUTPUT);
  digitalWrite(enable[0], LOW);
  digitalWrite(enable[1], LOW);
}

void loop() {

  if (Serial.available()) {

    String inJSON = Serial.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, inJSON);

    if (error) {
      sendJSON["state"] = "FAIL";
      sendJSON["error"] = error.c_str();
      serializeJson(sendJSON, Serial);
      Serial.println();

    } else {
      String Function = receiveJSON["Function"];
      int noRelay = receiveJSON["noRelay"] | 0;
      int noEn = receiveJSON["noEn"] | 0;
      int opc = 0;


      if (Function == "ping") opc = 1;            // {"Function":"ping"}
      else if (Function == "onRelay") opc = 2;    // {"Function":"onRelay", "noRelay":1}
      else if (Function == "offRelay") opc = 3;   // {"Function":"offRelay", "noRelay":1}
      else if (Function == "onEnable") opc = 4;   // {"Function":"onEnable", "noEn":0}
      else if (Function == "offEnable") opc = 5;  // {"Function":"offEnable", "noEn":0}

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
        case 3:
          {
            sendJSON.clear();
            uint8_t state = (opc == 3) ? HIGH : LOW;
            digitalWrite(relay[noRelay], state);
            break;
          }

        case 4:
        case 5:
          {
            sendJSON.clear();
            uint8_t state = (opc == 5) ? HIGH : LOW;
            digitalWrite(enable[noEn], state);
            break;
          }
      }
    }
  }
}
