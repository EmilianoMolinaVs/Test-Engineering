#include <Wire.h>
#include <HardwareSerial.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include "veml3328.h"  // Se modificó la libreria original

#define ADDR_VEML3328 0x10  // >> Dirección de I2C del Sensor de Luz

// ==== CONFIGURACIÓN DE PINES ====
#define RUN_BUTTON 4  // >> Arranque por Botonera
#define SDA_PIN 6     // >> GPIO06 SDA de I2C
#define SCL_PIN 7     // >> GPIO07 SCL de I2C
#define RX2 15        // >> GPIO15 como RX de UART2
#define TX2 19        // >> GPIO19 como TX de UART2

// ==== CREACIÓN DE OBJETOS ====
HardwareSerial PagWeb(1);

String JSON_entrada;                   ///< Buffer para recibir JSON desde PagWeb
StaticJsonDocument<1024> receiveJSON;  ///< Documento JSON para parsear datos recibidos

String JSON_lectura;                ///< Buffer para transmitir JSON de respuesta
StaticJsonDocument<1024> sendJSON;  ///< Documento JSON para armar respuestas

void serialDebug(String str) {
  str.replace("\"", "\\\"");  // Escapa comillas
  Serial.println("{\"debug\": \"" + str + "\"}");
}

void pagwebDebug(String str) {
  str.replace("\"", "\\\"");  // Escapa comillas
  PagWeb.println("{\"debug\": \"" + str + "\"}");
}

void setup() {
  // ==== Inicialización de Comunicación Serie ====
  Serial.begin(115200);
  PagWeb.begin(115200, SERIAL_8N1, RX2, TX2);
  delay(100);
  serialDebug("Serial Initilized...");
  pagwebDebug("Test initialized...");

  // ==== Inicialización de Bus I2C ====
  Wire.begin(SDA_PIN, SCL_PIN);
}

void loop() {

  if (digitalRead(RUN_BUTTON) == HIGH) {
    delay(100);
    sendJSON.clear();  // Limpia cualquier dato previo

    if (digitalRead(RUN_BUTTON) == LOW) {
      serialDebug("Arranque por botonera");
      sendJSON["Run"] = "OK";           // Envio de corriente JSON para corto
      serializeJson(sendJSON, PagWeb);  // Envío de datos por JSON a la PagWeb
      PagWeb.println();
    }
    s
  }

  if (PagWeb.available()) {

    JSON_entrada = PagWeb.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);

    String Function = receiveJSON["Function"];
    int opc = 0;

    if (Function == "ping") opc = 1;             // {"Function":"ping"}
    else if (Function == "initSensor") opc = 2;  // {"Function":"initSensor"}

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
        {
          // ==== Inicialización del Sensor de Luz ====
          sendJSON.clear();

          Veml3328.begin(ADDR_VEML3328);
          uint8_t idDevice = Veml3328.deviceID();
          if (idDevice != 0x28) {
            serialDebug("ERROR: it could not initialize sensor light i2c");
          } else {
            serialDebug("OK: sensor light VEML3328 initialized");
            sendJSON["Result"] = "OK";

            // ==== Configuración de Canales de Lectura RGB ====
            serialDebug("Configurando canales...");
            Veml3328.wake();                // Despierta el chip completo
            Veml3328.rbWakeup();            // Asegura que canales Rojo y Azul estén encendidos
            Veml3328.setGain(gain_x1);      // Aumentamos la ganancia y el tiempo para que no de 0 en interiores
            Veml3328.setIntTime(time_400);  // 400ms de integración (más sensibilidad)
          }
          sendJSON["id"] = "0x" + String(idDevice, HEX);
          serializeJson(sendJSON, PagWeb);
          PagWeb.println();
          break;
        }

      case 3:
        {
          uint8_t checkID = Veml3328.deviceID();

          if (checkID != 0x28) {
            Serial.println("¡ERROR! Conexión I2C perdida. Reintentando bus...");
            Wire.begin(I2C_SDA, I2C_SCL);
            delay(500);
          } else {
          }
        }

      default: break;
    }
  }
}
