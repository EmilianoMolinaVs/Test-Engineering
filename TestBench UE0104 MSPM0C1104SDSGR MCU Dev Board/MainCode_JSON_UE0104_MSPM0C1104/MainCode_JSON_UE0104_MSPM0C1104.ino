
/* 
===== CÓDIGO DE INTEGRACIÓN CONTROL UE0104 TexasInstruments JSON ====
Este firmware sirve como puente entre el mcu dev board de texas instuments ue0104 y la interfaz
de pruebas para realizar su test. 
Se recibe el JSON de entrada por uart de Result OK y se lee el blink de LED programado en PA0 del
Texas Instruments
*/

// --- BIBLIOTECAS ---
#include <Wire.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include <Arduino.h>

// ==== Declaración de pines
#define RX2 D0        // -> GPIO04 como RXD
#define TX2 D1        // -> GPIO05 como TXD
#define RUN_BUTTON 4  // -> Botón de Arranque
#define PIN_PA0 5     // -> GPIO05 PIN de Lectura de BLINK en PA0

// ==== Inicialización de objetos
HardwareSerial PagWeb(1);  // Objeto para UART2 en PULSAR como PagWeb

// ==== Variables de inicialización
String JSON_entrada;  // Variable que recibe al JSON en crudo de PagWeb
StaticJsonDocument<200> receiveJSON;

String JSON_lectura;  // Variable que envía el JSON de datos
StaticJsonDocument<200> sendJSON;

void serialDebug(const String &message) {
  String escaped = message;
  escaped.replace("\"", "\\\"");
  Serial.println("{\"debug\": \"" + escaped + "\"}");
}

void setup() {

  Serial.begin(115200);                        // Serial enlaza la PagWeb
  PagWeb.begin(115200, SERIAL_8N1, RX2, TX2);  // Bus de comunicación con el CH552

  pinMode(RUN_BUTTON, INPUT);
  pinMode(PIN_PA0, INPUT);
}

void loop() {

  if (digitalRead(RUN_BUTTON) == HIGH) {
    sendJSON.clear();
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

      int opc = 0;
      if (Function == "uart") opc = 1;           // {"Function":"uart"}
      else if (Function == "blinkPA0") opc = 2;  // {"Function":"blinkPA0"}

      switch (opc) {
        case 1:
          {
            serialDebug("Inicio de lectura en bus de UART...");
            sendJSON.clear();
            bool stateUART = false;
            bool foundResult = false;
            bool foundLedOff = false;
            bool foundLedOn = false;

            for (int i = 0; i < 10; i++) {

              while (PagWeb.available()) {
                String jsonLectura = PagWeb.readStringUntil('\n');
                jsonLectura.trim();

                if (jsonLectura.indexOf("{Result: OK}") != -1) foundResult = true;
                if (jsonLectura.indexOf("LED OFF") != -1) foundLedOff = true;
                if (jsonLectura.indexOf("LED ON") != -1) foundLedOn = true;
              }

              if (foundResult && foundLedOff && foundLedOn) {
                sendJSON["Result"] = "OK";
                serializeJson(sendJSON, Serial);
                Serial.println();

                // Limpiar el buffer residual por si siguen llegando datos
                while (PagWeb.available()) { PagWeb.read(); }

                stateUART = true;
                break;  // Rompe el ciclo for porque ya se encontraron las claves
              }

              delay(500);
            }

            if (!stateUART) {
              serialDebug("No hay comunicación por UART...");
            }
            break;
          }

        case 2:
          {
            serialDebug("Inicio de lectura de blink...");
            sendJSON.clear();
            bool last_state = (digitalRead(PIN_PA0) == HIGH);
            int flank = 0;
            bool debugBlink = true;
            String stateBlink = "";
            delay(100);
            for (int i = 0; i < 12; i++) {

              if (debugBlink) {
                if (digitalRead(PIN_PA0) == HIGH) stateBlink = "HIGH";
                else stateBlink = "LOW";
                serialDebug("Blink " + stateBlink);
              }

              bool current_state = (digitalRead(PIN_PA0) == HIGH);  // Leemos el estado actual en esta iteración
              if (current_state != last_state) {                    // Estados diferentes = cambio de flanco
                flank++;
                last_state = current_state;
              }
              delay(500);
            }
            serialDebug("Número de flancos detectados: " + String(flank));
            if (flank >= 2) sendJSON["Result"] = "OK";
            else sendJSON["Result"] = "FAIL";
            sendJSON["flank"] = String(flank);
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }
      }
    }
  }
}