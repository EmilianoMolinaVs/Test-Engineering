
/*
Firmware Test para Placa base de ENERSI
*/

// ==== BIBLIOTECAS ====
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Wire.h>

// ==== DECLARACIÓN DE GPIOS ====
#define LED_1 13                   // >>  BLINK LED sobre la PCB [Verde]
#define LED_2 7                    // >> BLINK LED sobre tarjeta WH12 [Azul]
#define E1_PIN 4                   // >> GPIO04 Activación de Relay E1
#define E2_PIN 5                   // >> GPIO05 Activación de Relay E1
#define E3_PIN 8                   // >> GPIO08 Activación de Relay E1
const uint8_t EEPROM_ADDR = 0x50;  // A0..A2 = GND

// ==== DECLARACIÓN DE OBJETOS ====
StaticJsonDocument<128> sendJSON;
StaticJsonDocument<128> receiveJSON;

// ==== FUNCIONES DE UTILIDAD ====
void scanDevices() {
  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("Encontrado: 0x");
      if (addr < 16) Serial.print("0");
      Serial.println(addr, HEX);
    }
  }
}

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
  const uint8_t PAGE_SIZE = 64;  // tamaño de página del AT24C256
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
  const size_t CHUNK = 32;  // límite típico del buffer Wire en AVR (Arduino UNO)
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

void serialDebug(String cmd) {
  StaticJsonDocument<128> doc;
  doc.clear();
  doc["debug"] = cmd;
  serializeJson(doc, Serial);
  Serial.println();
}

void demo() {
  int delay_ms = 200;
  digitalWrite(LED_1, HIGH);
  digitalWrite(LED_2, HIGH);
  delay(delay_ms);

  digitalWrite(LED_1, LOW);
  digitalWrite(LED_2, LOW);
  delay(delay_ms);
}

// ==== PROGRAMA PRINCIPAL ====
void setup() {
  Serial.begin(115200);
  delay(1000);
  serialDebug("Hola mundo");

  Wire.begin();
  // scanDevices();

  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);
  pinMode(E1_PIN, OUTPUT);
  pinMode(E2_PIN, OUTPUT);
  pinMode(E3_PIN, OUTPUT);
}

void loop() {

  if (Serial.available()) {
    String inJSON = Serial.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, inJSON);

    if (error) {
      sendJSON.clear();
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
      else if (Function == "onRelay") opc = 3;   // {"Function":"onRelay", "noRelay": 1}
      else if (Function == "offRelay") opc = 4;  // {"Function":"offRelay", "noRelay": 1}
      else if (Function == "eeprom") opc = 5;    // {"Function":"eeprom"}

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
          {
            sendJSON.clear();
            for (byte addr = 1; addr < 127; addr++) {
              Wire.beginTransmission(addr);
              if (Wire.endTransmission() == 0) {
                Serial.print("Encontrado: 0x");
                if (addr < 16) Serial.print("0");
                Serial.println(addr, HEX);
              }
            }
            break;
          }

        case 3:
          {
            sendJSON.clear();
            if (noRelay == 1) digitalWrite(E1_PIN, HIGH);
            else if (noRelay == 2) digitalWrite(E2_PIN, HIGH);
            else if (noRelay == 3) digitalWrite(E3_PIN, HIGH);
            break;
          }

        case 4:
          {
            sendJSON.clear();
            if (noRelay == 1) digitalWrite(E1_PIN, LOW);
            else if (noRelay == 2) digitalWrite(E2_PIN, LOW);
            else if (noRelay == 3) digitalWrite(E3_PIN, LOW);
            break;
          }

        case 5:
          {
            sendJSON.clear();
            // Escribir un byte
            uint16_t testAddr = 10;
            writeByte(testAddr, 'Z');
            Serial.println("Escrito 'Z' en addr 10");

            // Leer el byte
            byte c = readByte(testAddr);
            Serial.print("Leído de addr 10: ");
            Serial.println((char)c);

            // Escribir una cadena (manejo de páginas)
            const char* msg = "Hola AT24C256! Prueba de escritura por paginas.";
            writeString(5, msg);
            Serial.println("Cadena escrita en addr 0");

            // Leer la cadena (tamaño de msg)
            char buf[120];
            readData(5, buf, strlen(msg));
            Serial.print("Cadena leída: ");
            Serial.println(buf);

            break;
          }
      }
    }

  } else {
    demo();
  }
}
