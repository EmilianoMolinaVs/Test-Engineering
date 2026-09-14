/*
Firmware Test para Placa base de ENERSI
*/

// ==== BIBLIOTECAS ====
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Wire.h>

// ==== DECLARACIÓN DE GPIOS ====
#define LED_1 13                   // >> BLINK LED sobre la PCB [Verde]
#define LED_2 7                    // >> BLINK LED sobre tarjeta WH12 [Azul]
#define E1_PIN 4                   // >> GPIO04 Activación de Relay E1
#define E2_PIN 5                   // >> GPIO05 Activación de Relay E2
#define E3_PIN 8                   // >> GPIO08 Activación de Relay E3
const uint8_t EEPROM_ADDR = 0x50;  // A0..A2 = GND | ADDR DEFAULT

// ==== VARIABLES GLOBALES ====
unsigned long previousMillis = 0;
const long blinkInterval = 200;
bool ledState = LOW;

// ==== FUNCIONES DE UTILIDAD PARA EEPROM ====
void writeByte(uint16_t memAddr, byte data) {
  Wire.beginTransmission(EEPROM_ADDR);
  Wire.write((uint8_t)(memAddr >> 8));
  Wire.write((uint8_t)(memAddr & 0xFF));
  Wire.write(data);
  Wire.endTransmission();
  delay(10);
}

byte readByte(uint16_t memAddr) {
  Wire.beginTransmission(EEPROM_ADDR);
  Wire.write((uint8_t)(memAddr >> 8));
  Wire.write((uint8_t)(memAddr & 0xFF));
  Wire.endTransmission();
  Wire.requestFrom((int)EEPROM_ADDR, 1);
  if (Wire.available()) return Wire.read();
  return 0;
}

void writeString(uint16_t addr, const char* str) {
  const uint8_t PAGE_SIZE = 64;
  uint16_t i = 0;
  while (str[i]) {
    uint8_t pageOffset = addr % PAGE_SIZE;
    uint8_t canWrite = PAGE_SIZE - pageOffset;
    uint8_t count = 0;
    Wire.beginTransmission(EEPROM_ADDR);
    Wire.write((uint8_t)(addr >> 8));
    Wire.write((uint8_t)(addr & 0xFF));
    while (str[i] && count < canWrite) {
      Wire.write(str[i++]);
      count++;
    }
    Wire.endTransmission();
    delay(10);
    addr += count;
  }
}

void readData(uint16_t addr, char* out, size_t len) {
  size_t idx = 0;
  const size_t CHUNK = 32;
  while (idx < len) {
    size_t toRead = min(CHUNK, len - idx);
    Wire.beginTransmission(EEPROM_ADDR);
    Wire.write((uint8_t)((addr + idx) >> 8));
    Wire.write((uint8_t)((addr + idx) & 0xFF));
    Wire.endTransmission();
    Wire.requestFrom((int)EEPROM_ADDR, (int)toRead);
    size_t got = 0;
    while (Wire.available() && got < toRead) {
      out[idx++] = Wire.read();
      got++;
    }
  }
  out[len] = '\0';
}

// ==== RUTINA DEMO NO BLOQUEANTE ====
void demo() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= blinkInterval) {
    previousMillis = currentMillis;
    ledState = !ledState;
    digitalWrite(LED_1, ledState);
    digitalWrite(LED_2, ledState);
  }
}

// ==== PROGRAMA PRINCIPAL ====
void setup() {
  Serial.begin(115200);
  Wire.begin();

  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);
  pinMode(E1_PIN, OUTPUT);
  pinMode(E2_PIN, OUTPUT);
  pinMode(E3_PIN, OUTPUT);

  // Mensaje de inicio en formato JSON
  JsonDocument startupDoc;
  startupDoc["status"] = "ready";
  startupDoc["device"] = "ENERSI_Base";
  serializeJson(startupDoc, Serial);
  Serial.println();
}

void loop() {
  if (Serial.available()) {
    String inJSON = Serial.readStringUntil('\n');

    // Documentos locales para cada ciclo (se limpian automáticamente)
    JsonDocument receiveJSON;
    JsonDocument sendJSON;

    DeserializationError error = deserializeJson(receiveJSON, inJSON);

    if (error) {
      sendJSON["status"] = "FAIL";
      sendJSON["error"] = String("Invalid JSON: ") + error.c_str();
      serializeJson(sendJSON, Serial);
      Serial.println();
    } else {
      String Function = receiveJSON["Function"];
      int noRelay = receiveJSON["noRelay"] | 0;
      int opc = 0;

      if (Function == "ping") opc = 1;           // {"Function":"ping"}
      else if (Function == "scan_i2c") opc = 2;  // {"Function":"scan_i2c"}
      else if (Function == "onRelay") opc = 3;   // {"Function":"onRelay", "noRelay":1}
      else if (Function == "offRelay") opc = 4;  // {"Function":"offRelay", "noRelay":1}
      else if (Function == "eeprom") opc = 5;    // {"Function":"eeprom"}

      switch (opc) {
        case 1:  // PING
          {
            sendJSON["Function"] = "ping";
            sendJSON["status"] = "OK";
            sendJSON["ping"] = "pong";
            break;
          }

        case 2:  // SCAN I2C
          {
            sendJSON["Function"] = "scan_i2c";
            JsonArray devices = sendJSON["devices"].to<JsonArray>();

            for (byte addr = 1; addr < 127; addr++) {
              Wire.beginTransmission(addr);
              if (Wire.endTransmission() == 0) {
                // Convertir la dirección a formato Hexadecimal (ej: "0x50")
                char hexAddr[5];
                sprintf(hexAddr, "0x%02X", addr);
                devices.add(hexAddr);
              }
            }

            if (devices.size() > 0) {
              sendJSON["status"] = "OK";
              sendJSON["count"] = devices.size();
            } else {
              sendJSON["status"] = "FAIL";
              sendJSON["message"] = "No I2C devices found";
            }
            break;
          }

        case 3:  // ON RELAY
        case 4:  // OFF RELAY
          {
            sendJSON["Function"] = Function;
            sendJSON["noRelay"] = noRelay;

            uint8_t state = (opc == 3) ? HIGH : LOW;
            bool validRelay = true;

            if (noRelay == 1) digitalWrite(E1_PIN, state);
            else if (noRelay == 2) digitalWrite(E2_PIN, state);
            else if (noRelay == 3) digitalWrite(E3_PIN, state);
            else validRelay = false;

            if (validRelay) {
              sendJSON["status"] = "OK";
              sendJSON["state"] = (opc == 3) ? "ON" : "OFF";
            } else {
              sendJSON["status"] = "FAIL";
              sendJSON["error"] = "Invalid relay number";
            }
            break;
          }

        case 5:  // EEPROM TEST
          {
            sendJSON["Function"] = "eeprom";

            // Escribir y leer un byte
            uint16_t testAddr = 10;
            writeByte(testAddr, 'Z');
            byte c = readByte(testAddr);

            // Escribir y leer una cadena
            const char* msg = "Hola AT24C256!.";
            writeString(15, msg);

            char buf[64];
            readData(15, buf, strlen(msg));

            // Estructurar los resultados en el JSON
            sendJSON["status"] = "OK";

            JsonObject byteTest = sendJSON["byte_test"].to<JsonObject>();
            byteTest["address"] = testAddr;
            byteTest["written"] = "Z";
            byteTest["read"] = String((char)c);

            JsonObject stringTest = sendJSON["string_test"].to<JsonObject>();
            stringTest["address"] = 15;
            stringTest["written"] = msg;
            stringTest["read"] = String(buf);

            break;
          }

        default:  // COMANDO DESCONOCIDO
          {
            sendJSON["status"] = "FAIL";
            sendJSON["error"] = "Unknown Function";
            break;
          }
      }

      // Enviar la respuesta construida por el Switch (solo 1 vez)
      serializeJson(sendJSON, Serial);
      Serial.println();
    }
  } else {
    // Si no hay datos en el serial, mantiene los LEDs parpadeando sin bloquear el sistema
    demo();
  }
}