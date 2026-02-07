/*
=== MainCode JSON UE0109 BMA255 ===
Código de integración para BMA255 con lectura de I2C y SPI 
*/


#include <Wire.h>
#include <Arduino.h>
#include <SPI.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include "BMA250.h"


// ---- Pines Comunicación I2C ySPI ----
#define SDA_PIN 6   // MOSI
#define SCL_PIN 7   // SCl
#define SDO_PIN 21  // MISO
#define CS_PIN 18
#define PS_PIN 14

#define RUN_BUTTON 4  // Botón de Arranque

// ---- Creación de Objetos y Declaración de Variables ----
BMA250 accel_sensor;
int x, y, z;
double temp;

// ==== Declaración de JSON ====
String JSON_entrada;  // Variable que recibe al JSON en crudo de PagWeb
StaticJsonDocument<200> receiveJSON;

String JSON_salida;  // Variable que envía el JSON de datos
StaticJsonDocument<200> sendJSON;

void setup() {

  Serial.begin(115200);

  // ---- Declaración de pines de Entrada/Salida ----
  pinMode(PS_PIN, OUTPUT);
  digitalWrite(PS_PIN, HIGH);
}

void loop() {

  if (Serial.available()) {

    JSON_entrada = Serial.readStringUntil('\n');                              // Leer hasta newline (JSON en crudo)
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);  // Deserializa el JSON y guarda la información en datosJSON

    if (!error) {
      String Function = receiveJSON["Function"];  // Function es la variable de interés del JSON

      int opc = 0;

      if (Function == "ping") opc = 1;      // {"Function":"ping"}
      else if (Function == "i2c") opc = 2;  // {"Function":"i2c"}
      else if (Function == "spi") opc = 3;  // {"Function":"spi"}

      switch (opc) {

        // Validación de comunicación UART con la PagWeb <-> Pulsar
        case 1:
          {
            sendJSON.clear();
            sendJSON["ping"] = "pong";
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }

        // ---- Lectura de sensor con I2C 0x18 || 0x19 ----
        case 2:
          {
            Serial.println("Entro i2c");
            digitalWrite(PS_PIN, HIGH);
            digitalWrite(SDO_PIN, HIGH);
            delay(50);

            Wire.begin(SDA_PIN, SCL_PIN);
            sendJSON.clear();
            int result = 10;

            for (int i = 0; i < 10; i++) {
              result = accel_sensor.begin(BMA250_range_2g, BMA250_update_time_64ms);
              if (result == 0) {
                sendJSON["i2c"] = "OK";
                sendJSON["addr"] = String(accel_sensor.I2Caddress, HEX);
                Serial.print("Encontro dir: ");
                Serial.println(accel_sensor.I2Caddress, HEX);
                break;
              } else {
                sendJSON.clear();
                sendJSON["i2c"] = "Fail";
                serializeJson(sendJSON, Serial);
                Serial.println();
              }
              delay(10);
            }

            if (result == 0) {
              for (int j = 0; j < 25; j++) {
                accel_sensor.read();
                x = accel_sensor.X;
                y = accel_sensor.Y;
                z = accel_sensor.Z;
                temp = ((accel_sensor.rawTemp * 0.5) + 24.0);

                // Check if the BMA250 is not found or connected correctly
                if (x != -1 && y != -1 && z != -1) {
                  //showSerial();
                  showJSON();
                  serializeJson(sendJSON, Serial);
                  Serial.println();
                  delay(100);
                }
              }
            }


            Wire.end();
            Serial.println("Acabo i2c");
            break;
          }


        case 3:
          {
            sendJSON.clear();
            digitalWrite(PS_PIN, LOW);
            SPI.begin(SCL_PIN, SDO_PIN, SDA_PIN, CS_PIN);
            int result = accel_sensor.beginSPI(BMA250_range_2g, BMA250_update_time_64ms, CS_PIN, &SPI);

            if (result == 0) {
              sendJSON["spi"] = "OK";
              for (int j = 0; j < 25; j++) {
                accel_sensor.read();
                x = accel_sensor.X;
                y = accel_sensor.Y;
                z = accel_sensor.Z;
                temp = ((accel_sensor.rawTemp * 0.5) + 24.0);

                // Check if the BMA250 is not found or connected correctly
                if (x != -1 && y != -1 && z != -1) {
                  //showSerial();
                  showJSON();
                  serializeJson(sendJSON, Serial);
                  Serial.println();
                  delay(200);
                }
              }
            } else {
              Serial.println(" FAILED!");
            }

            SPI.end();
            break;
          }
      }
    }
  }


  delay(100);
}




// Prints the sensor values to the Serial Monitor
void showSerial() {
  Serial.print("X = ");
  Serial.print(x);

  Serial.print("  Y = ");
  Serial.print(y);

  Serial.print("  Z = ");
  Serial.print(z);

  Serial.print("  Temperature(C) = ");
  Serial.println(temp);
}

void showJSON() {
  sendJSON["x"] = x;
  sendJSON["y"] = y;
  sendJSON["z"] = z;
}