/*
Firmware testbench para Pulsar C6
Puente entre la interfaz de pruebas (PagWeb) y el target, reenviando comandos
JSON passthrough por UART y ejecutando el control interno de relevadores e INA219.


      {"Function":"ping"}
      {"Function":"mac"}
      {"Function":"i2c_scan"}
      {"Function":"mgn"}
      {"Function":"bat"}
      {"Function":"press"}
      {"Function":"prox"}
      {"Function":"sd"}
      {"Function":"wifi"}
      {"Function":"gpios"}
      {"Function":"restart"}

      {"Function":"relayON"}
      {"Function":"relayOFF"}
      {"Function":"currentSensor"}
      {"Function":"powerON"}
      {"Function":"powerOFF"}
      {"Function":"hi"}

*/

// --- BIBLIOTECAS ---
#include <Arduino.h>
#include <Wire.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include <Adafruit_INA219.h>
#include <math.h>

// ==== Declaración de pines ====
#define RUN_BUTTON 2  // Botón de Arranque
#define RX2 4         // RX de UART1 hacia el target
#define TX2 5         // TX de UART1 hacia el target
#define SDA_PIN 6     // SDA I2C (INA219)
#define SCL_PIN 7     // SCL I2C (INA219)
#define RELAYA 8      // Nota: si está en corto físico con GPIO9, déjalo en INPUT
#define RELAYB 9      // Relevador de accionamiento de fuente
#define RELAY_PIN 20  // Relevador de alimentación

// ==== Inicialización de objetos ====
HardwareSerial UART(1);
Adafruit_INA219 ina219_in(0x40);
Adafruit_INA219 ina219_out(0x41);

// ==== Buffers JSON y estado ====
StaticJsonDocument<512> receiveJSON;
StaticJsonDocument<512> sendJSON;
StaticJsonDocument<512> filterJSON;

bool waitingResponse = false;
unsigned long sendTime = 0;
unsigned long activeTimeout = 5000;
const unsigned long TIMEOUT_DEFAULT = 5000;
const unsigned long TIMEOUT_WIFI = 30000;

const float shuntOffset_mV = 0.0f;
const float R_SHUNT = 0.05f;  // 50 mΩ

// Estado para antirrebote no bloqueante del botón
bool lastButtonState = LOW;
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_MS = 50;

// ==== FUNCIONES DE UTILIDAD ====
const char *passthroughCmds[] = {
  "ping", "mac", "i2c_scan", "mgn", "bat",
  "press", "prox", "sd", "wifi", "gpios", "restart"
};

bool esPassthrough(const String &cmd) {
  for (const char *p : passthroughCmds) {
    if (cmd == p) return true;
  }
  return false;
}

void serialDebug(const String &str) {
  sendJSON.clear();
  sendJSON["debug"] = str;
  serializeJson(sendJSON, Serial);
  Serial.println();
}

void pagwebDebug(const String &str) {
  sendJSON.clear();
  sendJSON["debug"] = str;
  serializeJson(sendJSON, UART);
  UART.println();
}

float current_in() {
  float shunt_mV = ina219_in.getShuntVoltage_mV();
  shunt_mV -= shuntOffset_mV;
  float shunt_V = shunt_mV / 1000.0f;
  return shunt_V / R_SHUNT;  // Corriente en Amperes
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(100);
  delay(100);

  UART.begin(115200, SERIAL_8N1, RX2, TX2);
  UART.setTimeout(100);

  Wire.begin(SDA_PIN, SCL_PIN);

  if (!ina219_in.begin(&Wire)) {
    serialDebug("Current sensor INA219_in 0x40 not initialized...");
    while (1) {
      delay(500);
    }
  }
  serialDebug("Test Pulsar C6 Ready...");

  // ---- Definición de entradas y salidas ----
  pinMode(RUN_BUTTON, INPUT_PULLDOWN);
  pinMode(RELAYA, OUTPUT);  // Cambiar a INPUT si hay puente físico con RELAYB
  pinMode(RELAYB, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);

  digitalWrite(RELAYA, LOW);
  digitalWrite(RELAYB, LOW);
  digitalWrite(RELAY_PIN, HIGH);
}

void loop() {
  // ---- BOTÓN (flanco de bajada no bloqueante) ----
  bool reading = digitalRead(RUN_BUTTON);
  if (reading != lastButtonState && (millis() - lastDebounceTime) > DEBOUNCE_MS) {
    lastDebounceTime = millis();
    if (lastButtonState == HIGH && reading == LOW) {
      sendJSON.clear();
      sendJSON["Run"] = "OK";
      serializeJson(sendJSON, Serial);
      Serial.println();
    }
    lastButtonState = reading;
  }

  // ---- PagWeb -> Testbench / Target ----
  if (Serial.available()) {
    String JSON_entrada = Serial.readStringUntil('\n');
    JSON_entrada.trim();

    if (JSON_entrada.length() > 0) {
      receiveJSON.clear();
      DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);

      if (!error) {
        String Function = receiveJSON["Function"] | "";

        if (esPassthrough(Function)) {
          sendJSON.clear();
          sendJSON["Function"] = Function;
          serializeJson(sendJSON, UART);
          UART.println();

          waitingResponse = true;
          sendTime = millis();
          activeTimeout = (Function == "wifi") ? TIMEOUT_WIFI : TIMEOUT_DEFAULT;
        } else {
          int opc = 0;

          if (Function == "relayON") opc = 10;
          else if (Function == "relayOFF") opc = 11;
          else if (Function == "currentSensor") opc = 12;
          else if (Function == "powerON") opc = 13;
          else if (Function == "powerOFF") opc = 14;
          else if (Function == "hi") opc = 15;

          switch (opc) {
            case 10:
              digitalWrite(RELAY_PIN, HIGH);
              break;

            case 11:
              digitalWrite(RELAY_PIN, LOW);
              break;

            case 12:
              {
                sendJSON.clear();
                const float minCurrent = 0.150f;
                const float maxCurrent = 0.280f;
                delay(50);
                float corriente = fabsf(current_in());
                sendJSON["current"] = corriente;
                sendJSON["Result"] = (corriente > minCurrent && corriente < maxCurrent) ? "OK" : "FAIL";
                serializeJson(sendJSON, Serial);
                Serial.println();
                break;
              }

            case 13:
              digitalWrite(RELAYB, LOW);
              digitalWrite(RELAYA, LOW);
              break;

            case 14:
              digitalWrite(RELAYB, HIGH);
              digitalWrite(RELAYA, HIGH);
              break;

            case 15:
              sendJSON.clear();
              sendJSON["debug"] = "hola mundo";
              serializeJson(sendJSON, Serial);
              Serial.println();
              break;

            default:
              sendJSON.clear();
              sendJSON["status"] = "FAIL";
              sendJSON["error"] = "Invalid function requested";
              serializeJson(sendJSON, Serial);
              Serial.println();
              break;
          }
        }
      }
    }
  }

  // ---- Target (UART) -> PagWeb (Serial) ----
  if (UART.available()) {
    String rxLine = UART.readStringUntil('\n');
    rxLine.trim();

    if (rxLine.length() > 0) {
      filterJSON.clear();
      DeserializationError error = deserializeJson(filterJSON, rxLine);

      if (!error) {
        serializeJson(filterJSON, Serial);
        Serial.println();
        waitingResponse = false;
      }
    }
  }

  // ---- TIMEOUT de respuesta del target ----
  if (waitingResponse && (millis() - sendTime >= activeTimeout)) {
    waitingResponse = false;
    sendJSON.clear();
    sendJSON["Result"] = "FAIL";
    sendJSON["error"] = "Target UART timeout";
    serializeJson(sendJSON, Serial);
    Serial.println();
  }
}