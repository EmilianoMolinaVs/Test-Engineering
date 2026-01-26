/* 
===== CÓDIGO DE INTEGRACIÓN CONTROL DIS GUARDIA NACIONAL ====
Este código funciona unicamente como un Passthrough entre la PagWeb y el arnés para DIS

-- Como conexiones, la PagWeb selecciona el COM de la Pulsar en el Test y el conector QWIIC del arnés
se conecta al bus de pines GPIO01 y GPIO02 UARTL
*/

// --- BIBLIOTECAS ---
#include <Wire.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include <Arduino.h>

// ==== Declaración de pines
#define RX2 D1        // GPIO como RXD
#define TX2 D0        // GPIO como TXD
#define RUN_BUTTON 4  // Botón de Arranque
#define RELAYCV 5     // Relay de control para demanda con CV

// Relevadores de Accionamiento de Fuente
#define RELAYA 8
#define RELAYB 9

// ==== Inicialización de objetos
HardwareSerial VSV(1);  // Objeto para UART2 en PULSAR como PagWeb

// ==== Variables de inicialización
String JSON_entrada;  // Variable que recibe al JSON en crudo de PagWeb
StaticJsonDocument<200> receiveJSON;

String JSON_lectura;  // Variable que envía el JSON de datos
StaticJsonDocument<200> sendJSON;

bool waitingResponse = false;
unsigned long sendTime = 0;
const unsigned long TIMEOUT = 3000;  // 3 segundos

String rxDIS = "";


void setup() {

  Serial.begin(115200);                     // Serial enlaza la PagWeb DIS 4800 || VSV 115200
  VSV.begin(115200, SERIAL_8N1, RX2, TX2);  // Bus de comunicación con el CH552

  pinMode(RUN_BUTTON, INPUT);
  pinMode(RELAYA, OUTPUT);
  pinMode(RELAYB, OUTPUT);
  pinMode(RELAYCV, OUTPUT);

  digitalWrite(RELAYA, LOW);
  digitalWrite(RELAYB, LOW);
  digitalWrite(RELAYCV, LOW);
}

void loop() {

  // ---- BOTÓN ----
  if (digitalRead(RUN_BUTTON) == HIGH) {
    delay(100);
    if (digitalRead(RUN_BUTTON) == LOW) {
      sendJSON["Run"] = "OK";
      serializeJson(sendJSON, Serial);
      Serial.println();
    }
  }


  // ---- PagWeb → VSV ----
  if (Serial.available()) {

    JSON_entrada = Serial.readStringUntil('\n');                              // Leer hasta newline (JSON en crudo)
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);  // Deserializa el JSON y guarda la información en datosJSON

    if (!error) {
      String Function = receiveJSON["Function"];  // Function es la variable de interés del JSON
      int opc = 0;                                // Variable de switcheo

      if (Function == "ping") opc = 1;          // {"Function":"ping"}
      else if (Function == "testAll") opc = 2;  // {"Function":"testAll"}
      else if (Function == "stepUp") opc = 3;   // {"Function":"testAll"}
      else if (Function == "reg") opc = 4;      // {"Function":"testAll"}

      switch (opc) {
        case 1:
          {
            sendJSON.clear();  // Limpia cualquier dato previo
            sendJSON["Function"] = "ping";
            serializeJson(sendJSON, VSV);  // Envío de datos por JSON a la PagWeb
            VSV.println();
            break;
          }

        case 2:  // Testeo de Datos de GPIOs
          {
            sendJSON.clear();  // Limpia cualquier dato previo
            sendJSON["Function"] = "testAll";
            serializeJson(sendJSON, VSV);  // Envío de datos por JSON a la PagWeb
            VSV.println();
            break;
          }

        case 3:  // Demanda de corriente en 5V StepUp
          {
            digitalWrite(RELAYCV, LOW);
            break;
          }

        case 4:  // Demanda de corriente en 3.3V regulador de voltaje
          {
            digitalWrite(RELAYCV, HIGH);
            break;
          }
      }
    }
  }





  // ---- VSV → SERIAL (respuesta) ----
  if (VSV.available()) {
    char c = VSV.read();
    Serial.write(c);

    waitingResponse = false;
  }

  // ---- TIMEOUT ----
  if (waitingResponse && (millis() - sendTime >= TIMEOUT)) {
    waitingResponse = false;

    Serial.println("{\"Result\":\"Fail\", \"uart\":\"Fail\", \"gpioIn\":\"Fail\", \"analog\":\"Fail\", \"sw\":\"Fail\", \"gpioOut\":\"Fail\"}");
  }
}
