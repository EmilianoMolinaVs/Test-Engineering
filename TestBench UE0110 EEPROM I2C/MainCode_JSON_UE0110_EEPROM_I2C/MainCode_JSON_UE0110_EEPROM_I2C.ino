
#include <Wire.h>
#include <HardwareSerial.h>
#include <Arduino.h>
#include <ArduinoJson.h>

// ==== CONFIGURACIÓN DE PINES ====
#define SDA_PIN 6
#define SCL_PIN 7
#define A0_PIN 15     // GPIO15 - PIN A0 Selección Addr
#define A1_PIN 19     // GPIO19 - PIN A1 Selección Addr
#define A2_PIN 20     // GPIO20 - PIN A2 Selección Addr
#define WP_PIN 21     // GPIO21 - PIN SW Enable Escritura
#define RUN_BUTTON 4  // GPIO4  - Botón de arranque del TestBench

// ==== CONFIGURACIÓN y OBJETOS ====
#define EEPROM_BASE 0x50
uint8_t eepromAddr = EEPROM_BASE;

String JSON_entrada;                  ///< Buffer para recibir JSON desde PagWeb
StaticJsonDocument<256> receiveJSON;  ///< Documento JSON para parsear datos recibidos

String JSON_lectura;               ///< Buffer para transmitir JSON de respuesta
StaticJsonDocument<256> sendJSON;  ///< Documento JSON para armar respuestas

// ==== Constructores de Función ====

// ---------- Utilidades ----------
void eepromWaitReady(uint32_t timeout = 25) {
  uint32_t t0 = millis();
  while (millis() - t0 < timeout) {
    Wire.beginTransmission(eepromAddr);
    if (Wire.endTransmission() == 0) return;
    delay(1);
  }
}

bool eepromWritePaged(uint16_t mem, const uint8_t* data, size_t len, size_t page = 32) {
  size_t off = 0;
  while (off < len) {
    size_t pageSpace = page - ((mem + off) % page);
    size_t chunk = min(len - off, pageSpace);
    Wire.beginTransmission(eepromAddr);
    Wire.write((mem + off) >> 8);
    Wire.write((mem + off) & 0xFF);
    Wire.write(data + off, chunk);
    if (Wire.endTransmission() != 0) return false;
    eepromWaitReady();
    off += chunk;
  }
  return true;
}

bool eepromReadSeq(uint16_t mem, uint8_t* buf, size_t len) {
  Wire.beginTransmission(eepromAddr);
  Wire.write(mem >> 8);
  Wire.write(mem & 0xFF);
  if (Wire.endTransmission(false) != 0) return false;
  size_t got = 0;
  while (got < len) {
    size_t req = min((size_t)32, len - got);
    if (Wire.requestFrom((int)eepromAddr, (int)req) != (int)req) return false;
    for (size_t i = 0; i < req; i++) buf[got++] = Wire.read();
  }
  return true;
}

// ---------- Comandos ----------
// Verifica protección de escritura intentando cambiar y restaurar un byte
bool cmdWPCheck(uint16_t memAddr) {
  uint8_t orig = 0xFF, test;
  if (!eepromReadSeq(memAddr, &orig, 1)) {
    Serial.println("Lectura inicial falló.");
    return false;
  }

  test = (orig == 0xFF) ? 0x00 : 0xFF;         // valor diferente seguro
  if (!eepromWritePaged(memAddr, &test, 1)) {  // algunos chips con WP alto NACKean la escritura
    Serial.println("Escritura NACK → posible WP ACTIVA o bus error.");
    return false;
  }
  uint8_t after = 0xFF;
  if (!eepromReadSeq(memAddr, &after, 1)) {
    Serial.println("Lectura posterior falló.");
    return false;
  }

  if (after != test) {
    Serial.printf("WP ACTIVA: Byte no cambió (0x%02X -> 0x%02X leído 0x%02X)\n", orig, test, after);
    return true;
  }

  // Si sí cambió, WP está desactivada; restauramos el valor original.
  if (!eepromWritePaged(memAddr, &orig, 1)) {
    Serial.println("¡Atención! No se pudo restaurar el valor original.");
  } else {
    eepromReadSeq(memAddr, &after, 1);
  }
  Serial.printf("WP DESACTIVADA: se pudo escribir y restaurar (final=0x%02X)\n", after);
}

bool cmdRead(uint16_t addr, int len, String& out) {
  if (len <= 0 || len > 256) {
    printDebug("len max 256");
    return false;
  }

  uint8_t buf[256];

  if (!eepromReadSeq(addr, buf, len)) {
    printDebug("Error de lectura");
    return false;
  }

  out = "";
  for (int i = 0; i < len; i++) {
    char c = (buf[i] >= 32 && buf[i] < 127) ? (char)buf[i] : '.';
    out += c;
  }

  return true;
}

bool cmdWrite(uint16_t addr, const String& text) {
  const uint8_t* data = (const uint8_t*)text.c_str();
  size_t len = text.length();

  if (!eepromWritePaged(addr, data, len)) {
    printDebug("Write FAIL (I2C)");
    return false;
  }

  uint8_t verify[256];
  if (!eepromReadSeq(addr, verify, len)) {
    printDebug("Write FAIL (readback)");
    return false;
  }

  for (size_t i = 0; i < len; i++) {
    if (verify[i] != data[i]) {
      printDebug("Write FAIL (verify mismatch)");
      return false;
    }
  }

  printDebug("Write OK (verified)");
  return true;
}

void cmdErase() {
  printDebug("Borrando toda la EEPROM (0xFF)...");
  const int total = 4096;  // 32 Kbit = 4 KB
  uint8_t ff[32];
  memset(ff, 0xFF, sizeof(ff));
  for (int a = 0; a < total; a += 32) {
    if (!eepromWritePaged(a, ff, 32)) {
      printDebug("Fallo en 0x" + String(a));
      break;
    }
  }
  printDebug("Borrado completo.");
}

// Permite fijar manualmente la dirección encontrada
void cmdAddrSet(uint8_t addr) {
  eepromAddr = addr;
  Serial.printf("Dirección EEPROM seleccionada: 0x%02X\n", eepromAddr);
}


void printDebug(String str) {
  str.replace("\"", "\\\"");  // Escapa comillas
  Serial.println("{\"debug\": \"" + str + "\"}");
}

void setup() {

  Serial.begin(115200);
  delay(100);
  printDebug("Serial inicializado");

  Wire.begin(SDA_PIN, SCL_PIN);

  // Configuración de GPIOs para control de entrada/salida
  pinMode(RUN_BUTTON, INPUT);  ///< Entrada: Botón de arranque
  pinMode(A0_PIN, OUTPUT);
  pinMode(A1_PIN, OUTPUT);
  pinMode(A2_PIN, OUTPUT);
  pinMode(WP_PIN, OUTPUT);
  digitalWrite(A0_PIN, LOW);
  digitalWrite(A1_PIN, LOW);
  digitalWrite(A2_PIN, LOW);
  digitalWrite(WP_PIN, LOW);
}

void loop() {

  // ========== DETECCIÓN DEL BOTÓN DE ARRANQUE ==========
  // Lee el estado del botón con debounce simple
  if (digitalRead(RUN_BUTTON) == HIGH) {
    delay(150);
    if (digitalRead(RUN_BUTTON) == LOW) {
      sendJSON.clear();
      sendJSON["Run"] = "OK";  ///< Indica que se presionó el botón
      serializeJson(sendJSON, Serial);
      Serial.println();
    }
  }

  if (Serial.available()) {

    JSON_entrada = Serial.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);

    if (!error) {

      String Function = receiveJSON["Function"];
      String Value = receiveJSON["Value"];

      int opc = 0;
      if (Function == "ping") opc = 1;          // {"Function": "ping"}
      else if (Function == "address") opc = 2;  // {"Function": "address"}
      else if (Function == "write") opc = 3;    // {"Function": "write", "Value": "Hola Mundo :D"}
      else if (Function == "read") opc = 4;     // {"Function": "read"}
      else if (Function == "erase") opc = 5;    // {"Function": "erase"}
      else if (Function == "writep") opc = 6;   // {"Function": "writep"}
      else if (Function == "testAll") opc = 7;  // {"Function": "testAll"}

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
            printDebug("Escaneando addr 0x50 - 0x57...");

            JsonArray arr = sendJSON.createNestedArray("addr");

            for (int i = 0; i < 8; i++) {

              int a0 = (i >> 0) & 1;
              int a1 = (i >> 1) & 1;
              int a2 = (i >> 2) & 1;

              digitalWrite(A0_PIN, a0);
              digitalWrite(A1_PIN, a1);
              digitalWrite(A2_PIN, a2);
              delay(5);

              uint8_t addr = 0x50 | (a2 << 2) | (a1 << 1) | a0;

              Wire.beginTransmission(addr);
              if (Wire.endTransmission() == 0) {

                String hexAddr = "0x" + String(addr, HEX);
                hexAddr.toUpperCase();

                printDebug(hexAddr + " (EEPROM encontrada)");
                arr.add(hexAddr);
              }
            }

            if (arr.size() > 0) {
              eepromAddr = strtol(arr[0], NULL, 16);
              digitalWrite(A0_PIN, LOW);
              digitalWrite(A1_PIN, LOW);
              digitalWrite(A2_PIN, LOW);
            }

            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }

        case 3:
          {
            if (Value != "") {
              sendJSON.clear();
              bool state = cmdWrite(0, Value);
              sendJSON["ok"] = state;
              serializeJson(sendJSON, Serial);
              Serial.println();
            } else {
              printDebug("Valor de escritura vacío");
            }
            break;
          }

        case 4:
          {
            String data = "";
            sendJSON.clear();
            if (cmdRead(0, 13, data)) {
              sendJSON["ok"] = "true";
              sendJSON["data"] = data;
            } else {
              sendJSON["read"] = "false";
            }
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }

        case 5:
          {
            cmdErase();
          }

        case 6:
          {
            digitalWrite(WP_PIN, HIGH);  // Activar protección
            cmdWPCheck(0x0000);
            delay(1000);
            digitalWrite(WP_PIN, LOW);  // Des protección
            break;
          }

        case 7:
          {
            sendJSON.clear();
            digitalWrite(WP_PIN, LOW);
            String data = "";

            cmdErase(); // Test de Borrado
            delay(10);
            bool write = cmdWrite(0, "Hola Mundo :D");  // Test de Escritura
            bool read = cmdRead(0, 13, data); // Test de Lectura
            digitalWrite(WP_PIN, HIGH);  // Activar protección
            bool wp = cmdWPCheck(0x0000); // Test de Protección 
            delay(1000);
            digitalWrite(WP_PIN, LOW);  // Des protección
            if (read && write && wp) {
              sendJSON["state"] = "OK";
            } else {
              sendJSON["state"] = "FAIL";
            }
            sendJSON["write"] = write;
            sendJSON["read"] = read;
            sendJSON["wp"] = wp; 
            sendJSON["data"] = data;
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }

        default:
          printDebug("Comando desconocido");
          break;
      }
    }
  }
}
