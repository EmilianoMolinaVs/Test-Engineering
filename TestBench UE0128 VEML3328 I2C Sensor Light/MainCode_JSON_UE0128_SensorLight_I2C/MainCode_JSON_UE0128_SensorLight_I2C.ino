#include <Wire.h>
#include <HardwareSerial.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include "veml3328.h"  // Se modificó la libreria original de MicroChip
#include <Adafruit_NeoPixel.h>

#define ADDR_VEML3328 0x10  // >> Dirección de I2C del Sensor de Luz
#define NUMPIXELS 128       // Popular NeoPixel ring size

// ==== CONFIGURACIÓN DE PINES ====
#define NEOPIXEL_PIN 2  // >> GPIO02 Control de Neopixel
#define RUN_BUTTON 4    // >> GPIO04 Arranque por Botonera
#define SDA_PIN 6       // >> GPIO06 SDA de I2C
#define SCL_PIN 7       // >> GPIO07 SCL de I2C
#define RX2 15          // >> GPIO15 como RX de UART2
#define TX2 19          // >> GPIO19 como TX de UART2

// ==== CREACIÓN DE OBJETOS ====
HardwareSerial PagWeb(1);
Adafruit_NeoPixel matrix(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

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

  // ==== Inicializaciones de I2C y Neopixel ====
  Wire.begin(SDA_PIN, SCL_PIN);
  matrix.begin();
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
  }

  if (PagWeb.available()) {

    JSON_entrada = PagWeb.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);

    // ==== Valores recibios por el JSON ====
    String Function = receiveJSON["Function"];
    String Color = receiveJSON["Color"] | "red";
    int Intensity = receiveJSON["Int"] | 30;

    int opc = 0;
    if (Function == "ping") opc = 1;             // {"Function":"ping"}
    else if (Function == "initSensor") opc = 2;  // {"Function":"initSensor"}
    else if (Function == "readSensor") opc = 3;  // {"Function":"readSensor", "Color":"blue", "Int":30} || Case Fijo
    else if (Function == "sweep") opc = 4;       // {"Function":"sweep"}
    else if (Function == "cleanColor") opc = 5;  // {"Function":"cleanColor"}

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
            Veml3328.setGain(gain_x1_2);    // Aumentamos la ganancia y el tiempo para que no de 0 en interiores
            Veml3328.setIntTime(time_200);  // 400ms de integración (más sensibilidad)
          }
          sendJSON["id"] = "0x" + String(idDevice, HEX);
          serializeJson(sendJSON, PagWeb);
          PagWeb.println();
          break;
        }

      case 3:
        {
          sendJSON.clear();
          matrix.clear();

          uint8_t checkID = Veml3328.deviceID();
          if (checkID != 0x28) {
            serialDebug("Lost i2c bus communication...");
            pagwebDebug("Lost i2c bus communication...");
            Wire.begin(SDA_PIN, SCL_PIN);
            delay(500);
          } else {
            sendJSON["Result"] = "OK";
            if (Color == "red" || Color == "blue" || Color == "green") {
              int r = 0, g = 0, b = 0;
              // ==== Color designado en matrix Neopixel ====
              if (Color == "red") r = Intensity;
              else if (Color == "green") g = Intensity;
              else if (Color == "blue") b = Intensity;
              for (int i = 0; i < NUMPIXELS; i++) {
                matrix.setPixelColor(i, matrix.Color(r, g, b));
              }
              matrix.show();  // Send the updated pixel colors to the hardware.
            }

            delay(500);
            uint16_t red_sensor = Veml3328.getRed();
            uint16_t green_sensor = Veml3328.getGreen();
            uint16_t blue_sensor = Veml3328.getBlue();
            uint16_t ir_sensor = Veml3328.getIR();

            JsonArray spectre = sendJSON.createNestedArray("spectre");
            JsonObject RGB = spectre.createNestedObject();
            RGB["R"] = red_sensor;
            RGB["G"] = green_sensor;
            RGB["B"] = blue_sensor;
            RGB["IR"] = ir_sensor;
          }

          serializeJson(sendJSON, PagWeb);
          PagWeb.println();
          break;
        }

      case 4:
        {
          sendJSON.clear();
          matrix.clear();
          int delay_ms = 50;

          uint8_t checkID = Veml3328.deviceID();
          if (checkID != 0x28) {
            serialDebug("Lost i2c bus communication...");
            pagwebDebug("Lost i2c bus communication...");
            Wire.begin(SDA_PIN, SCL_PIN);
            delay(500);
          } else {
            sendJSON["Result"] = "OK";
            serializeJson(sendJSON, PagWeb);
            PagWeb.println();

            // ==== SWEEP COLOR ====
            int step = 5;
            int lim = 60;
            // ---- RED ----
            for (int j = 0; j < lim + 40; j += step) {
              for (int i = 0; i < NUMPIXELS; i++) {
                matrix.setPixelColor(i, matrix.Color(j, 0, 0));
              }
              matrix.show();  // Send the updated pixel colors to the hardware.

              delay(delay_ms);
              sendJSON.clear();
              serialDebug("Intensidad: " + String(j));
              uint16_t red_sensor = Veml3328.getRed();
              uint16_t green_sensor = Veml3328.getGreen();
              uint16_t blue_sensor = Veml3328.getBlue();
              uint16_t ir_sensor = Veml3328.getIR();

              JsonArray spectre = sendJSON.createNestedArray("spectre");
              JsonObject RGB = spectre.createNestedObject();
              RGB["R"] = red_sensor;
              RGB["G"] = green_sensor;
              RGB["B"] = blue_sensor;
              RGB["IR"] = ir_sensor;
              serializeJson(sendJSON, PagWeb);
              PagWeb.println();
            }
            for (int i = 0; i < NUMPIXELS; i++) {
              matrix.setPixelColor(i, matrix.Color(0, 0, 0));
            }
            matrix.show();
            // ---- GREEN ----
            for (int j = 0; j < lim; j += step) {
              for (int i = 0; i < NUMPIXELS; i++) {
                matrix.setPixelColor(i, matrix.Color(0, j, 0));
              }
              matrix.show();  // Send the updated pixel colors to the hardware.

              delay(delay_ms);
              sendJSON.clear();
              serialDebug("Intensidad: " + String(j));
              uint16_t red_sensor = Veml3328.getRed();
              uint16_t green_sensor = Veml3328.getGreen();
              uint16_t blue_sensor = Veml3328.getBlue();
              uint16_t ir_sensor = Veml3328.getIR();

              JsonArray spectre = sendJSON.createNestedArray("spectre");
              JsonObject RGB = spectre.createNestedObject();
              RGB["R"] = red_sensor;
              RGB["G"] = green_sensor;
              RGB["B"] = blue_sensor;
              RGB["IR"] = ir_sensor;
              serializeJson(sendJSON, PagWeb);
              PagWeb.println();
            }
            for (int i = 0; i < NUMPIXELS; i++) {
              matrix.setPixelColor(i, matrix.Color(0, 0, 0));
            }
            matrix.show();
            // ---- BLUE ----
            for (int j = 0; j < lim; j += step) {
              for (int i = 0; i < NUMPIXELS; i++) {
                matrix.setPixelColor(i, matrix.Color(0, 0, j));
              }
              matrix.show();  // Send the updated pixel colors to the hardware.

              delay(delay_ms);
              sendJSON.clear();
              serialDebug("Intensidad: " + String(j));
              uint16_t red_sensor = Veml3328.getRed();
              uint16_t green_sensor = Veml3328.getGreen();
              uint16_t blue_sensor = Veml3328.getBlue();
              uint16_t ir_sensor = Veml3328.getIR();

              JsonArray spectre = sendJSON.createNestedArray("spectre");
              JsonObject RGB = spectre.createNestedObject();
              RGB["R"] = red_sensor;
              RGB["G"] = green_sensor;
              RGB["B"] = blue_sensor;
              RGB["IR"] = ir_sensor;
              serializeJson(sendJSON, PagWeb);
              PagWeb.println();
            }
            for (int i = 0; i < NUMPIXELS; i++) {
              matrix.setPixelColor(i, matrix.Color(0, 0, 0));
            }
            matrix.show();
          }
          break;
        }


      case 5:
        for (int i = 0; i < NUMPIXELS; i++) {
          matrix.setPixelColor(i, matrix.Color(0, 0, 0));
        }
        matrix.show();
        break;

      default: break;
    }
  }
}
