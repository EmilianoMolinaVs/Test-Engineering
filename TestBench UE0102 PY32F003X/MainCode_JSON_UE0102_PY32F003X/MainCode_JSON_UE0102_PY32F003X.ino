/* 
===== CÓDIGO DE INTEGRACIÓN CONTROL UE0102 PUYA JSON ====
Este código funciona unicamente como un Passthrough entre la PagWeb y el UE0102 

-- Como conexiones, la PagWeb selecciona el COM de la Pulsar en el Test y el conector QWIIC del arnés
se conecta al bus de pines D0 y D1 UARTL

El firmware de prueba que se carga al PUYA está dentro del repo del proyecto
*/

// --- BIBLIOTECAS ---
#include <Wire.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include <Arduino.h>

// ==== Declaración de pines
#define RX2 D0        // GPIO04 como RXD
#define TX2 D1        // GPIO05 como TXD
#define RUN_BUTTON 4  // Botón de Arranque
#define RELAYCom 20   // Relevador de puerto USB C

// ==== Inicialización de objetos
HardwareSerial PY32(1);  // Objeto para UART2 en PULSAR como PagWeb

// ==== Variables de inicialización
String JSON_entrada;  // Variable que recibe al JSON en crudo de PagWeb
StaticJsonDocument<200> receiveJSON;

String JSON_lectura;  // Variable que envía el JSON de datos
StaticJsonDocument<200> sendJSON;

String readTargetResponse() {
  String response = "";
  unsigned long timeout = millis() + 500;  // 500 ms timeout

  while (millis() < timeout) {
    while (PY32.available()) {
      char c = PY32.read();
      response += c;
    }
  }

  response.trim();
  return response;
}


void setup() {

  Serial.begin(115200);                      // Serial enlaza la PagWeb
  PY32.begin(115200, SERIAL_8N1, RX2, TX2);  // Bus de comunicación con el CH552

  pinMode(RELAYCom, OUTPUT);
  pinMode(RUN_BUTTON, INPUT);
  digitalWrite(RELAYCom, HIGH);
}

void loop() {

  if (digitalRead(RUN_BUTTON) == HIGH) {
    delay(100);

    if (digitalRead(RUN_BUTTON) == LOW) {
      sendJSON["Run"] = "OK";           // Envio de corriente JSON para corto
      serializeJson(sendJSON, Serial);  // Envío de datos por JSON a la PagWeb
      Serial.println();
    }
  }

  if (Serial.available()) {  // If anything comes in Serial (USB),

    JSON_entrada = Serial.readStringUntil('\n');                              // Leer hasta newline (JSON en crudo)
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);  // Deserializa el JSON y guarda la información en datosJSON

    if (!error) {

      String Function = receiveJSON["Function"];

      if (Function == "testAll") {  // {"Function":"testAll"}

        sendJSON.clear();

        /* ======================
        1️⃣ TEST ADC
        ====================== */

        delay(50);
        PY32.write("a");

        String respuestaADC = readTargetResponse();

        int rawValue = 0;
        int mvValue = 0;
        String adcState = "Fail";

        int colonIndex = respuestaADC.indexOf(":");
        int pipeIndex = respuestaADC.indexOf("|");
        int mvIndex = respuestaADC.indexOf("mV");

        if (colonIndex != -1 && pipeIndex != -1 && mvIndex != -1) {

          String rawStr = respuestaADC.substring(colonIndex + 1, pipeIndex);
          rawStr.trim();
          rawValue = rawStr.toInt();

          String mvStr = respuestaADC.substring(pipeIndex + 1, mvIndex);
          mvStr.trim();
          mvValue = mvStr.toInt();

          if (mvValue > 2500 && mvValue < 4000) {
            adcState = "OK";
          }
        }

        /* ======================
        2️⃣ TEST UID
        ====================== */

        delay(50);
        PY32.write("u");

        String respuestaUID = readTargetResponse();

        String uidStr = "";
        int colonIndexUID = respuestaUID.indexOf(":");

        if (colonIndexUID != -1) {
          uidStr = respuestaUID.substring(colonIndexUID + 1);
          uidStr.trim();
        }

        /* ======================
        3️⃣ JSON FINAL
        ====================== */

        //sendJSON["adc_raw"] = rawValue;
        sendJSON["adc_mv"] = mvValue;
        //sendJSON["adc_state"] = adcState;
        sendJSON["uid"] = uidStr;

        if (adcState == "OK" && uidStr.length() > 0) {
          sendJSON["global_state"] = "OK";
        } else {
          sendJSON["global_state"] = "Fail";
        }

        serializeJson(sendJSON, Serial);
        Serial.println();
      } else {
        Serial.println("Modo no válido");
      }
    }
  }

  if (PY32.available()) {
    Serial.write(PY32.read());
  }
}