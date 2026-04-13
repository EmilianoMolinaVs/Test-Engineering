#include <ArduinoJson.h>

String jsonEntrada;

StaticJsonDocument<256> docIn;
StaticJsonDocument<128> docOut;

// ===== Lista de voltajes =====
float voltajes[2] = { 2.9, 4.2 };
int indiceVoltaje = 0;

// ===== Lista de corrientes =====
float corrientes[2] = { 0.4, 1.5 };
int indiceCorriente = 0;

float randomFloat(float min, float max) {
  return min + (random(0, 10000) / 10000.0) * (max - min);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Pulsar C6 listo para JSON");
}

void loop() {

  if (Serial.available()) {
    jsonEntrada = Serial.readStringUntil('\n');

    DeserializationError error = deserializeJson(docIn, jsonEntrada);
    if (error) {
      Serial.println("JSON inválido");
      return;
    }

    String funcion = docIn["Funcion"] | "";
    String start = docIn["Start"] | "";

    if (funcion == "Other" && start == "Read") {

      // ===== Voltaje secuencial =====
      float voltaje = voltajes[indiceVoltaje];

      // Avanzar índice (cíclico)
      indiceVoltaje++;
      if (indiceVoltaje >= 6) {
        indiceVoltaje = 0;
      }

      // ===== Corriente secuencial =====
      float corriente = corrientes[indiceCorriente];

      // Avanzar índice (cíclico)
      indiceCorriente++;
      if (indiceCorriente >= 6) {
        indiceVoltaje = 0;
      }

      float potencia = voltaje * corriente;

      // ===== Formato solicitado =====
      String medicion = String(voltaje, 2) + ";" + String(corriente, 2) + ";" + String(potencia, 2);

      docOut.clear();
      docOut["medicion"] = medicion;

      serializeJson(docOut, Serial);
      Serial.println();
    }
  }
}