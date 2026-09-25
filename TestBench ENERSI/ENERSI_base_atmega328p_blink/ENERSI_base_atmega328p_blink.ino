/*
Firmware Test para Placa base de ENERSI
Este firmware se encarga de testear los perifericos existentes para el microcontrolador
atmega328p en la placa base de ENERSI
*/

// ==== BIBLIOTECAS ====
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <SoftwareSerial.h>

// ==== DECLARACIÓN DE GPIOS (Nombres Cortos) ====
#define BLINK_IN 2   // PD2 / Pin 04: entrada de blink en puente con PD3
#define BLINK_OUT 3  // PD3 / Pin 05: salida de blink en puente con PD2
#define RELAY1 4     // PD4 / Pin 06: control del relevador 1
#define RELAY2 5     // PD5 / Pin 11: control del relevador 2
#define RST_BLINK 6  // PD6 / Pin 12: señal RESET, activa un blink comandado
#define HC12_LED 7   // PD7 / Pin 13: LED nativo del módulo HC12
#define RELAY3 8     // PB0 / Pin 14: control del relevador 3
#define RS485_TX 9   // PB1 / Pin 15: RO del módulo RS485 montado (RX Ext)
#define HC12_RX 10   // PB2 / Pin 16: TX del módulo HC12, receptor inalambrico (TX Ext)
#define PCB_LED 13   // PB5 / Pin 19: SCK de programación / blink sobre PCB base
#define TEMP_PIN A1  // PC1 / Pin 24: entrada analógica del sensor LM35
#define SC1 A2       // PC2 / Pin 25: salida hacia pin 1 de SC1
#define SC2 A3       // PC3 / Pin 26: salida hacia pin 1 de SC2

// ==== VARIABLES GLOBALES ====
const uint8_t EEPROM_ADDR = 0x50;
unsigned long previousMillis = 0;
const long blinkInterval = 500;
bool ledState = LOW;

// Objeto para comunicación serial con dispositivo externo (RX=9, TX=10)
SoftwareSerial extSerial(HC12_RX, RS485_TX);

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

  extSerial.begin(9600);
  extSerial.setTimeout(150);  // Evita bloqueos largos al leer cadenas

  // Configuración de pines de hardware
  pinMode(PCB_LED, OUTPUT);
  pinMode(HC12_LED, OUTPUT);
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(RELAY3, OUTPUT);

  pinMode(RST_BLINK, OUTPUT);
  pinMode(SC1, OUTPUT);
  pinMode(SC2, OUTPUT);

  pinMode(BLINK_OUT, OUTPUT);
  pinMode(BLINK_IN, INPUT);

  pinMode(HC12_RX, INPUT_PULLUP);

  JsonDocument startupDoc;
  startupDoc["status"] = "ready";
  startupDoc["device"] = "ENERSI_Base";
  serializeJson(startupDoc, Serial);
  Serial.println();
}

void loop() {
  // 1. ESCUCHAR DISPOSITIVO EXTERNO (RX/TX en pines 9 y 10)
  if (extSerial.available()) {
    String incomingData = extSerial.readStringUntil('\n');

    // Filtro contra ruido espurio (\u0000 o basura)
    String cleanData = "";
    for (int i = 0; i < incomingData.length(); i++) {
      char c = incomingData.charAt(i);
      // Solo aceptamos caracteres ASCII imprimibles (esto incluye llaves y comillas de JSON)
      if (c >= 32 && c <= 126) {
        cleanData += c;
      }
    }

    cleanData.trim();

    // Solo procesamos si quedó una cadena válida
    if (cleanData.length() > 0) {
      JsonDocument extDoc;        // Documento que enviaremos a la PC
      JsonDocument receivedJSON;  // Documento para decodificar lo que llegó del transmisor

      // Intentamos parsear la cadena limpia como JSON
      DeserializationError error = deserializeJson(receivedJSON, cleanData);

      if (!error) {
        // Parseo exitoso. Asignamos el JSON recibido dentro de nuestro JSON de respuesta.
        // ArduinoJson es inteligente y anidará el objeto automáticamente.
        extDoc["event"] = "ext_data_received";
        extDoc["data"] = receivedJSON;

        int opc = 0;
        if (receivedJSON["msg"] == "blink") opc = 1;

        switch (opc) {
          case 1:
            {
              int delay_ms = 50;
              for (int i = 0; i < 10; i++) {
                digitalWrite(HC12_LED, HIGH);
                delay(delay_ms);
                digitalWrite(HC12_LED, LOW);
                delay(delay_ms);
              }
              break;
            }

          default:
            extDoc["error_switch"] = "invalid option";
        }


      } else {
        // Si lo que llegó no era un JSON válido (basura o texto plano)
        extDoc["event"] = "ext_data_error";
        extDoc["error"] = error.c_str();
        extDoc["raw_data"] = cleanData;
      }

      serializeJson(extDoc, Serial);
      Serial.println();
    }
  }

  // 2. ESCUCHAR COMANDOS DEL HOST (UART Nativo)
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
      String gpio_blink = receiveJSON["gpio"] | "";  // Se lee como String ("6", "A2", "A3")
      int opc = 0;

      if (Function == "ping") opc = 1;              // {"Function":"ping"}
      else if (Function == "scan_i2c") opc = 2;     // {"Function":"scan_i2c"}
      else if (Function == "onRelay") opc = 3;      // {"Function":"onRelay", "noRelay":1}
      else if (Function == "offRelay") opc = 4;     // {"Function":"offRelay", "noRelay":1}
      else if (Function == "eeprom") opc = 5;       // {"Function":"eeprom"}
      else if (Function == "test_bridge") opc = 6;  // {"Function":"test_bridge"}
      else if (Function == "blink_on") opc = 7;     // {"Function":"blink_on", "gpio": "A2"}
      else if (Function == "blink_off") opc = 8;    // {"Function":"blink_off", "gpio": "A2"}
      else if (Function == "send_ext") opc = 9;     // {"Function":"send_ext", "data": "Hola"}
      else if (Function == "read_temp") opc = 10;   // {"Function":"read_temp"}

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

        case 6:  // TEST BRIDGE DIGITAL: Comprueba lógica ALTA y BAJA
          {
            sendJSON["Function"] = "test_bridge";

            digitalWrite(BLINK_OUT, HIGH);
            delay(5);
            bool testHigh = digitalRead(BLINK_IN);

            digitalWrite(BLINK_OUT, LOW);
            delay(5);
            bool testLow = digitalRead(BLINK_IN);

            if (testHigh == HIGH && testLow == LOW) {
              sendJSON["status"] = "OK";
              sendJSON["message"] = "Puente fisico detectado correctamente";
            } else {
              sendJSON["status"] = "FAIL";
              sendJSON["error"] = "Fallo en lectura logica del puente";
              sendJSON["read_high"] = testHigh;
              sendJSON["read_low"] = testLow;
            }
            break;
          }

        case 7:
        case 8:  // BLINK ON / BLINK OFF
          {
            sendJSON["Function"] = Function;
            sendJSON["gpio"] = gpio_blink;

            uint8_t state = (opc == 7) ? HIGH : LOW;
            bool validGpio = true;

            if (gpio_blink == "6") digitalWrite(RST_BLINK, state);
            else if (gpio_blink == "A2") digitalWrite(SC1, state);
            else if (gpio_blink == "A3") digitalWrite(SC2, state);
            else validGpio = false;

            if (validGpio) {
              sendJSON["status"] = "OK";
              sendJSON["state"] = (opc == 7) ? "ON" : "OFF";
            } else {
              sendJSON["status"] = "FAIL";
              sendJSON["error"] = "Invalid GPIO";
            }
            break;
          }

        case 9:  // SEND EXT: Envia cadena al dispositivo externo
          {
            String dataToSend = receiveJSON["data"] | "";
            sendJSON["Function"] = "send_ext";

            if (dataToSend.length() > 0) {
              extSerial.println(dataToSend);
              sendJSON["status"] = "OK";
              sendJSON["sent"] = dataToSend;
            } else {
              sendJSON["status"] = "FAIL";
              sendJSON["error"] = "No data provided";
            }
            break;
          }

        case 10:  // READ TEMP: Lectura del LM35
          {
            sendJSON["Function"] = "read_temp";

            // Promedio de 5 lecturas para estabilizar
            int adcSum = 0;
            for (int i = 0; i < 5; i++) {
              adcSum += analogRead(TEMP_PIN);
              delay(2);
            }
            float adcProm = adcSum / 5.0;

            // Fórmula LM35: 10mV/°C. (V = ADC * 5V / 1024). Temp = V * 100.
            float tempC = (adcProm * 500.0) / 1024.0;

            sendJSON["status"] = "OK";
            sendJSON["temp_celsius"] = tempC;
            sendJSON["adc_raw"] = adcProm;
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