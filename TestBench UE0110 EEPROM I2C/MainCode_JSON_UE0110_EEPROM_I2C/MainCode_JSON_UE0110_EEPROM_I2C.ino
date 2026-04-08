
#include <Wire.h>
#include <HardwareSerial.h>
#include <Arduino.h>
#include <ArduinoJson.h>

// ==== CONFIGURACIÓN DE PINES ====
#define SDA_PIN 6
#define SCL_PIN 7
#define A0_PIN 15     // GPIO15 - PIN A0 Selección Addr
#define A1_PIN 19     // GPIO19 - PIN A1 Selección Addr
#define A0_PIN 20     // GPIO20 - PIN A2 Selección Addr
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

void dumpHex(const uint8_t* buf, size_t n) {
  for (size_t i = 0; i < n; i++) {
    if (i && i % 16 == 0) Serial.println();
    Serial.printf("%02X ", buf[i]);
  }
  Serial.println();
}

// ---------- Comandos ----------
void cmdScan() {
  Serial.println("\n[I2C] Escaneando...");
  bool any = false;
  bool picked = false;
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      any = true;
      Serial.printf("  - Dispositivo en 0x%02X", addr);
      if (addr >= 0x50 && addr <= 0x57) {
        Serial.print("  (EEPROM candidata)");
        if (!picked) {
          eepromAddr = addr;
          picked = true;
        }
      }
      Serial.println();
    }
    delay(2);
  }
  if (!any) Serial.println("  (Nada encontrado)");
  if (picked) Serial.printf("Usando EEPROM en 0x%02X\n", eepromAddr);
  else Serial.println("No se detectó EEPROM (0x50–0x57).");
}

// Verifica protección de escritura intentando cambiar y restaurar un byte
void cmdWPCheck(uint16_t memAddr) {
  uint8_t orig = 0xFF, test;
  if (!eepromReadSeq(memAddr, &orig, 1)) {
    Serial.println("Lectura inicial falló.");
    return;
  }

  test = (orig == 0xFF) ? 0x00 : 0xFF;         // valor diferente seguro
  if (!eepromWritePaged(memAddr, &test, 1)) {  // algunos chips con WP alto NACKean la escritura
    Serial.println("Escritura NACK → posible WP ACTIVA o bus error.");
    return;
  }
  uint8_t after = 0xFF;
  if (!eepromReadSeq(memAddr, &after, 1)) {
    Serial.println("Lectura posterior falló.");
    return;
  }

  if (after != test) {
    Serial.printf("WP ACTIVA: Byte no cambió (0x%02X -> 0x%02X leído 0x%02X)\n", orig, test, after);
    return;
  }

  // Si sí cambió, WP está desactivada; restauramos el valor original.
  if (!eepromWritePaged(memAddr, &orig, 1)) {
    Serial.println("¡Atención! No se pudo restaurar el valor original.");
  } else {
    eepromReadSeq(memAddr, &after, 1);
  }
  Serial.printf("WP DESACTIVADA: se pudo escribir y restaurar (final=0x%02X)\n", after);
}

void cmdRead(uint16_t addr, int len) {
  if (len <= 0 || len > 256) {
    Serial.println("len max 256");
    return;
  }
  uint8_t buf[256];
  if (eepromReadSeq(addr, buf, len)) {
    Serial.println("HEX:");
    dumpHex(buf, len);
    Serial.print("ASCII: ");
    for (int i = 0; i < len; i++) Serial.print((buf[i] >= 32 && buf[i] < 127) ? (char)buf[i] : '.');
    Serial.println();
  } else Serial.println("Error de lectura");
}

void cmdWrite(uint16_t addr, const String& text) {
  if (eepromWritePaged(addr, (const uint8_t*)text.c_str(), text.length()))
    Serial.printf("Escrito en 0x%04X\n", addr);
  else
    Serial.println("Error de escritura (WP alto, dirección o bus).");
}

void cmdErase() {
  Serial.println("Borrando toda la EEPROM (0xFF)...");
  const int total = 4096;  // 32 Kbit = 4 KB
  uint8_t ff[32];
  memset(ff, 0xFF, sizeof(ff));
  for (int a = 0; a < total; a += 32) {
    if (!eepromWritePaged(a, ff, 32)) {
      Serial.printf("Fallo en 0x%04X\n", a);
      break;
    }
  }
  Serial.println("Borrado completo.");
}

// Permite fijar manualmente la dirección encontrada
void cmdAddrSet(uint8_t addr) {
  eepromAddr = addr;
  Serial.printf("Dirección EEPROM seleccionada: 0x%02X\n", eepromAddr);
}

// ---------- Parser ----------
String input;
void handleCommand(String line) {
  line.trim();
  if (line.length() == 0) return;

  if (line == "scan") {
    cmdScan();
    return;
  }

  if (line.startsWith("addr ")) {
    String s = line.substring(5);
    uint8_t v = s.startsWith("0x") ? strtoul(s.c_str(), nullptr, 16) : (uint8_t)s.toInt();
    cmdAddrSet(v);
    return;
  }

  if (line.startsWith("read ")) {
    int sp1 = line.indexOf(' ');
    int sp2 = line.indexOf(' ', sp1 + 1);
    if (sp1 < 0 || sp2 < 0) {
      Serial.println("Uso: read <addr> <len>");
      return;
    }
    String a = line.substring(sp1 + 1, sp2), b = line.substring(sp2 + 1);
    uint16_t addr = a.startsWith("0x") ? strtoul(a.c_str(), nullptr, 16) : (uint16_t)a.toInt();
    int len = b.toInt();
    cmdRead(addr, len);
    return;
  }

  if (line.startsWith("write ")) {
    int sp1 = line.indexOf(' ');
    int sp2 = line.indexOf(' ', sp1 + 1);
    if (sp1 < 0 || sp2 < 0) {
      Serial.println("Uso: write <addr> <texto>");
      return;
    }
    String a = line.substring(sp1 + 1, sp2), text = line.substring(sp2 + 1);
    uint16_t addr = a.startsWith("0x") ? strtoul(a.c_str(), nullptr, 16) : (uint16_t)a.toInt();
    cmdWrite(addr, text);
    return;
  }

  if (line == "erase") {
    cmdErase();
    return;
  }

  if (line.startsWith("wpcheck")) {
    // wpcheck [addr]
    uint16_t m = 0x0000;
    int sp = line.indexOf(' ');
    if (sp > 0) {
      String a = line.substring(sp + 1);
      if (a.length()) m = a.startsWith("0x") ? strtoul(a.c_str(), nullptr, 16) : (uint16_t)a.toInt();
    }
    cmdWPCheck(m);
    return;
  }

  Serial.println("Comandos:");
  Serial.println("  scan");
  Serial.println("  addr <0x50..0x57>");
  Serial.println("  read <addr> <len>     (addr admite 0x..)");
  Serial.println("  write <addr> <texto>");
  Serial.println("  erase");
  Serial.println("  wpcheck [addr]        (prueba WP; por defecto 0x0000)");
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

      int opc = 0;
      if (Function == "ping") opc = 1;           // {"Function": "ping"}
      else if (Function == "testAddr") opc = 2;  // {"Function": "testAddr"}

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

for 


            for (uint8_t addr = 1; addr < 127; addr++) {
              Wire.beginTransmission(addr);
              if (Wire.endTransmission() == 0) {
                if (addr >= 0x50 && addr <= 0x57) {
                  String hexAddr = "0x" + String(addr, HEX);
                  hexAddr.toUpperCase();
                  printDebug(hexAddr + " (EEPROM candidata)");
                  arr.add(hexAddr);
                }
              }
              delay(20);
            }

            serializeJson(sendJSON, Serial);
            Serial.println();

            break;
          }
      }
    }
  }
}
