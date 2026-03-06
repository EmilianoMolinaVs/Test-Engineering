#include <ArduinoJson.h>

String jsonEntrada;

StaticJsonDocument<256> docIn;
StaticJsonDocument<128> docOut;

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
    String start   = docIn["Start"]   | "";

    if (funcion == "Other" && start == "Read") {

      // ===== Lecturas simuladas =====
      float voltaje   = 3;
      float corriente = 0.1;
      float potencia  = voltaje * corriente;

      // ===== Formato solicitado =====
      String medicion = String(voltaje, 2) + ";" +
                        String(corriente, 2) + ";" +
                        String(potencia, 2);

      docOut.clear();
      docOut["medicion"] = medicion;

      serializeJson(docOut, Serial);
      Serial.println();
    }
  }
}
