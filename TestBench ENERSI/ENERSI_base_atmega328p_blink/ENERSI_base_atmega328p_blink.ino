/*
Firmware Test para Placa base de ENERSI
Este firmware se encarga de testear los perifericos existentes para el microcontrolador
atmega328p en la placa base de ENERSI

Descripción de perifericos y funciones de gpios:
-> PC6 Pin 1 RESET para debug
-> PD0 Pin 02 RX Serial UART nativo
-> PD1 Pin 03 TX Serial UART nativo
-> PD2 Pin 04 como entrada de blink en puente con PD3
-> PD3 Pin 05 como salida de blink en puente con PD2
-> PD4 Pin 06 controlador de Relevador 1
-> PD5 Pin 11 controlador de Relevador 2
-> PD6 Pin 12 etiquetado como RESET (para dispositivo externo) activa un blink comandado
-> PD7 Pin 13 blink nativo a LED en HC12
-> PB0 Pin 14 controlador de Relevador 3
-> PB1 Pin 15 se conecta a RO del módulo RS485 montado *
-> PB2 Pin 16 conectado a TX del módulo HC12, es un receptor de datos inalámbricos
-> PB3 Pin 17 MOSI de programación
-> PB4 Pin 18 MISO de programación
-> PB5 Pin 19 SCK de programación | Blink sobre PCB Base
-> PC0 Pin 23 sin pista ruteada
-> PC1 Pin 24 entrada analógica de sensor LM35
-> PC2 Pin 25 conexión a pin 1 de SC1 *
-> PC3 Pin 26 conexión a pin 1 de SC2 *
-> PD4 Pin 27 SDA de bus I2C
-> PC5 Pin 28 SCL de bus I2C
*/

// ==== BIBLIOTECAS ====
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <SoftwareSerial.h>  // Para la transmisión de cadenas en el puente

// ==== DECLARACIÓN DE GPIOS (Nombres Cortos) ====
#define BLINK_IN 2   // PD2 / Pin 04: entrada de blink en puente con PD3 | OK
#define BLINK_OUT 3  // PD3 / Pin 05: salida de blink en puente con PD2 | OK
#define RELAY1 4     // PD4 / Pin 06: control del relevador 1 | OK
#define RELAY2 5     // PD5 / Pin 11: control del relevador 2 |OK
#define RST_BLINK 6  // PD6 / Pin 12: señal RESET, activa un blink comandado
#define HC12_LED 7   // PD7 / Pin 13: LED nativo del módulo HC12 | OK
#define RELAY3 8     // PB0 / Pin 14: control del relevador 3 | OK
#define RS485_RX 9   // PB1 / Pin 15: RO del módulo RS485 montado
#define HC12_TX 10   // PB2 / Pin 16: TX del módulo HC12, receptor inalambrico
#define PCB_LED 13   // PB5 / Pin 19: SCK de programación / blink sobre PCB base| OK
#define TEMP_PIN A1  // PC1 / Pin 24: entrada analógica del sensor LM35
#define SC1 A2       // PC2 / Pin 25: salida hacia pin 1 de SC1
#define SC2 A3       // PC3 / Pin 26: salida hacia pin 1 de SC2

// ==== VARIABLES GLOBALES ====
const uint8_t EEPROM_ADDR = 0x50;
unsigned long previousMillis = 0;
const long blinkInterval = 500;
bool ledState = LOW;

// Objeto para comunicación serial en el puente físico (RX=BLINK_IN, TX=BLINK_OUT)
SoftwareSerial bridgeSerial(BLINK_IN, BLINK_OUT);

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
    digitalWrite(PCB_LED, ledState);
    digitalWrite(HC12_LED, ledState);
  }
}

// ==== PROGRAMA PRINCIPAL ====
void setup() {
  Serial.begin(115200);
  Wire.begin();
  bridgeSerial.begin(9600);  // Inicializa puerto de prueba del puente

  pinMode(PCB_LED, OUTPUT);
  pinMode(HC12_LED, OUTPUT);
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(RELAY3, OUTPUT);

  JsonDocument startupDoc;
  startupDoc["status"] = "ready";
  startupDoc["device"] = "ENERSI_Base";
  serializeJson(startupDoc, Serial);
  Serial.println();
}

void loop() {
  if (Serial.available()) {
    String inJSON = Serial.readStringUntil('\n');
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

      if (Function == "ping") opc = 1;              // {"Function":"ping"}
      else if (Function == "scan_i2c") opc = 2;     // {"Function":"scan_i2c"}
      else if (Function == "onRelay") opc = 3;      // {"Function":"onRelay", "noRelay":1}
      else if (Function == "offRelay") opc = 4;     // {"Function":"offRelay", "noRelay":1}
      else if (Function == "eeprom") opc = 5;       // {"Function":"eeprom"}
      else if (Function == "test_bridge") opc = 6;  // {"Function":"test_bridge"}

      switch (opc) {
        case 1:
          {
            sendJSON["Function"] = "ping";
            sendJSON["status"] = "OK";
            sendJSON["ping"] = "pong";
            break;
          }

        case 2:
          {
            sendJSON["Function"] = "scan_i2c";
            JsonArray devices = sendJSON["devices"].to<JsonArray>();
            for (byte addr = 1; addr < 127; addr++) {
              Wire.beginTransmission(addr);
              if (Wire.endTransmission() == 0) {
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

        case 3:
        case 4:
          {
            sendJSON["Function"] = Function;
            sendJSON["noRelay"] = noRelay;

            uint8_t state = (opc == 3) ? HIGH : LOW;
            bool validRelay = true;

            if (noRelay == 1) digitalWrite(RELAY1, state);
            else if (noRelay == 2) digitalWrite(RELAY2, state);
            else if (noRelay == 3) digitalWrite(RELAY3, state);
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

        case 5:
          {
            sendJSON["Function"] = "eeprom";
            uint16_t testAddr = 10;
            writeByte(testAddr, 'Z');
            byte c = readByte(testAddr);

            const char* msg = "Hola AT24C256!.";
            writeString(15, msg);
            char buf[64];
            readData(15, buf, strlen(msg));

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

        case 6:  // TEST BRIDGE: Valida envío/recepción de cadena
          {
            sendJSON["Function"] = "test_bridge";
            String testString = "ENERSI_TEST_OK";

            // Limpia el buffer de entrada por si había basura
            while (bridgeSerial.available()) bridgeSerial.read();

            // Envía la cadena por BLINK_OUT (TX)
            bridgeSerial.println(testString);

            // Espera breve para asegurar que los bits hayan viajado a BLINK_IN (RX)
            delay(50);

            String receivedString = "";
            while (bridgeSerial.available()) {
              receivedString += (char)bridgeSerial.read();
            }

            // Limpiamos los caracteres de salto de línea de println (\r\n)
            receivedString.trim();

            // Verificamos coincidencia
            if (receivedString == testString) {
              sendJSON["status"] = "OK";
              sendJSON["message"] = "Cadena puenteada correctamente";
            } else {
              sendJSON["status"] = "FAIL";
              sendJSON["error"] = "Error en el puente. Recibido: " + receivedString;
            }
            break;
          }

        default:
          {
            sendJSON["status"] = "FAIL";
            sendJSON["error"] = "Unknown Function";
            break;
          }
      }
      serializeJson(sendJSON, Serial);
      Serial.println();
    }
  } else {
    demo();
  }
}