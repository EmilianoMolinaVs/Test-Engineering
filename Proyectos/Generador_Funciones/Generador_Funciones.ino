/*
  Generador de Funciones DDS con AD9833 JSON

  Soporta:
  - Modo fijo
  - Modo sweep (barrido)
  - Seno / Triángulo / Cuadrada
  - Frecuencia
  - Fase

*/

#include <Arduino.h>
#include <ArduinoJson.h>
#include <MD_AD9833.h>
#include <SPI.h>

// ===================== PINES SPI =====================
const uint8_t PIN_DATA = 7;
const uint8_t PIN_CLK = 6;
const uint8_t PIN_FSYNC = 18;

// ===================== OBJETO DDS ====================
MD_AD9833 AD(PIN_DATA, PIN_CLK, PIN_FSYNC);

// ===================== JSON ==========================
StaticJsonDocument<256> receiveJSON;

// ===================== SWEEP =========================
bool sweepActive = false;
float sweepFreq = 0;
float sweepStart = 0;
float sweepStop = 0;
float sweepStep = 0;
unsigned long sweepInterval = 50;
unsigned long lastSweepTime = 0;

// ====================================================
// ===== FUNCIONES AUXILIARES =========================
// ====================================================

MD_AD9833::mode_t getWaveform(String w) {
  w.toLowerCase();
  if (w == "sine") return MD_AD9833::MODE_SINE;
  if (w == "triangle") return MD_AD9833::MODE_TRIANGLE;
  if (w == "square1") return MD_AD9833::MODE_SQUARE1;
  if (w == "square2") return MD_AD9833::MODE_SQUARE2;
  return MD_AD9833::MODE_OFF;
}

// ----------------------------------------------------

void applyFixedMode(JsonDocument& doc) {
  float freq = doc["frequency_hz"] | 1000.0;
  float phase = doc["phase_deg"] | 0.0;
  String wave = doc["waveform"] | "sine";

  AD.setMode(getWaveform(wave));
  AD.setFrequency(MD_AD9833::CHAN_0, freq);
  AD.setPhase(MD_AD9833::CHAN_0, (uint16_t)(phase * 10));

  sweepActive = false;

  Serial.println(F("Modo FIXED aplicado"));
}

// ----------------------------------------------------

void setupSweep(JsonObject sweep) {
  sweepStart = sweep["start_frequency_hz"];
  sweepStop = sweep["stop_frequency_hz"];
  sweepStep = sweep["step_hz"];
  sweepInterval = sweep["interval_ms"] | 50;

  sweepFreq = sweepStart;
  sweepActive = true;

  Serial.println(F("Modo SWEEP iniciado"));
}

// ----------------------------------------------------

void runSweep() {
  if (!sweepActive) return;

  if (millis() - lastSweepTime >= sweepInterval) {
    lastSweepTime = millis();

    AD.setFrequency(MD_AD9833::CHAN_0, sweepFreq);
    sweepFreq += sweepStep;

    if (sweepFreq > sweepStop) {
      sweepActive = false;
      Serial.println(F("Sweep terminado"));
    }
  }
}

// ====================================================
// ===================== SETUP =========================
// ====================================================

void setup() {
  Serial.begin(115200);
  Serial.println(F("Generador DDS AD9833 listo"));

  AD.begin();
  AD.setMode(MD_AD9833::MODE_OFF);
}

// ====================================================
// ====================== LOOP =========================
// ====================================================

void loop() {

  // ====== LECTURA JSON ======
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');

    DeserializationError error = deserializeJson(receiveJSON, input);
    if (error) {
      Serial.println(F("Error JSON"));
      return;
    }

    String mode = receiveJSON["mode"];

    // ---------- MODO FIJO ----------
    if (mode == "fixed") {
      applyFixedMode(receiveJSON);
    }

    // ---------- MODO SWEEP ----------
    else if (mode == "sweep") {
      String wave = receiveJSON["waveform"] | "sine";
      float phase = receiveJSON["phase_deg"] | 0.0;

      AD.setMode(getWaveform(wave));
      AD.setPhase(MD_AD9833::CHAN_0, (uint16_t)(phase * 10));

      setupSweep(receiveJSON["sweep"]);
    }

    else {
      Serial.println(F("Modo desconocido"));
    }
  }

  // ====== EJECUCIÓN SWEEP ======
  runSweep();
}
