/* 
===== CÓDIGO DE INTEGRACIÓN CONTROL UE0002 DualMCU ====
Puente para control de relevadores de alimentación de cargador de baterías con super capacitor y demanda de carga variable
Es un código unicamente de control de potencia, la comunicación se da mediante el arnés 

*/

// --- BIBLIOTECAS ---
#include <Wire.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include <Arduino.h>

// ==== Declaración de pines
#define RX2 4         // GPIO04 como RXD
#define TX2 5         // GPIO05 como TXD
#define RUN_BUTTON 2  // Botón de Arranque
#define RELAYCom 20   // Relevador de puerto USB C

// ==== Inicialización de objetos
HardwareSerial USB(1);  // Objeto para UART2 en PULSAR como PagWeb

// ==== Variables de inicialización
String JSON_entrada;  // Variable que recibe al JSON en crudo de PagWeb
StaticJsonDocument<200> receiveJSON;

String JSON_lectura;  // Variable que envía el JSON de datos
StaticJsonDocument<200> sendJSON;

void setup() {

  Serial.begin(115200);                     // Serial de enlace entre TestBench y PagWeb
  USB.begin(115200, SERIAL_8N1, RX2, TX2);  // Bus de comunicación con el UART de la DualMCU

  pinMode(RELAYCom, OUTPUT);
  pinMode(RUN_BUTTON, INPUT);

  digitalWrite(RELAYCom, HIGH);  // Estado inicial: Cargador alimentando DualMCU
}

void loop() {

  if (digitalRead(RUN_BUTTON) == HIGH) {
    delay(100);

    if (digitalRead(RUN_BUTTON) == LOW) {
      sendJSON.clear();
      sendJSON["Run"] = "OK";           // Envio de corriente JSON para corto
      serializeJson(sendJSON, Serial);  // Envío de datos por JSON a la PagWeb
      Serial.println();
    }
  }

  if (Serial.available()) {  // Instrucciones JSON desde PagWeb

    JSON_entrada = Serial.readStringUntil('\n');                              // Leer hasta newline (JSON en crudo)
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);  // Deserializa el JSON y guarda la información en datosJSON

    if (!error) {

      String Function = receiveJSON["Function"];

      int opc = 0;
      // ==== Operaciones de validación de ESP32 ====
      if (Function == "ping") opc = 1;             // {"Function":"ping"}
      else if (Function == "mac") opc = 2;         // {"Function":"mac"}
      else if (Function == "bme") opc = 3;         // {"Function":"bme"}
      else if (Function == "test_esp32") opc = 4;  // {"Function":"test_esp32"}
      else if (Function == "vn_sensor") opc = 5;   // {"Function":"vn_sensor"}
      // ==== Operaciones de validación de RP2040 ====
      else if (Function == "passthrough") opc = 6;  // Comando passthrough: {"Function":"passthrough"}
      else if (Function == "hmotor") opc = 7;       // Comando haptic motor: {"Function":"hmotor"}
      else if (Function == "analog_rp") opc = 8;    // Comando analógicos RP: {"Function":"analog_rp"}
      else if (Function == "test_rp") opc = 9;      // Comando Test Rp2040: {"Function":"test_rp"}
      // ==== Operaciones de control con TestBench ====
      else if (Function == "VCC_ON") opc = 10;   // Comando Test Rp2040: {"Function":"VCC_ON"}
      else if (Function == "VCC_OFF") opc = 11;  // Comando Test Rp2040: {"Function":"VCC_OFF"}


      switch (opc) {
        case 1:
          {
            sendJSON.clear();
            sendJSON["Function"] = "ping";
            serializeJson(sendJSON, USB);
            USB.println();
            break;
          }

        case 2:
          {
            sendJSON.clear();
            sendJSON["Function"] = "mac";
            serializeJson(sendJSON, USB);
            USB.println();
            break;
          }

        case 3:
          {
            sendJSON.clear();
            sendJSON["Function"] = "bme";
            serializeJson(sendJSON, USB);
            USB.println();
            break;
          }

        case 4:
          {
            sendJSON.clear();
            sendJSON["Function"] = "test_esp32";
            serializeJson(sendJSON, USB);
            USB.println();
            break;
          }

        case 5:
          {
            sendJSON.clear();
            sendJSON["Function"] = "vn_sensor";
            serializeJson(sendJSON, USB);
            USB.println();
            break;
          }

        case 6:
          {
            sendJSON.clear();
            sendJSON["Function"] = "passthrough";
            serializeJson(sendJSON, USB);
            USB.println();
            break;
          }

        case 7:
          {
            sendJSON.clear();
            sendJSON["Function"] = "hmotor";
            serializeJson(sendJSON, USB);
            USB.println();
            break;
          }

        case 8:
          {
            sendJSON.clear();
            sendJSON["Function"] = "analog_rp";
            serializeJson(sendJSON, USB);
            USB.println();
            break;
          }

        case 9:
          {
            sendJSON.clear();
            sendJSON["Function"] = "test_rp";
            serializeJson(sendJSON, USB);
            USB.println();
            break;
          }

        case 10:
          {
            digitalWrite(RELAYCom, HIGH);
            break;
          }

        case 11:
          {
            digitalWrite(RELAYCom, LOW);
            break;
          }
      }
    }
  }

  if (USB.available()) {
    Serial.write(USB.read());
  }
}