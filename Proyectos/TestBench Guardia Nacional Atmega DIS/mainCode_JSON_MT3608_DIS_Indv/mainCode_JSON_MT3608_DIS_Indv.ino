// --- BIBLIOTECAS ---
#include <ArduinoJson.h>
#include <Arduino.h>

#define RUN_BUTTON 4  // Botón de Arranque
#define RELAYA 8      // Relé A - Control de fuente de alimentación
#define RELAYB 9      // Relé B - Control de fuente de alimentación

// ==== Variables de inicialización
String JSON_entrada;  // Variable que recibe al JSON en crudo de PagWeb
StaticJsonDocument<200> receiveJSON;

String JSON_lectura;  // Variable que envía el JSON de datos
StaticJsonDocument<200> sendJSON;

void setup() {
  Serial.begin(115200);
  pinMode(RUN_BUTTON, INPUT);
  // Configurar pines como salida (relés)
  pinMode(RELAYA, OUTPUT);
  pinMode(RELAYB, OUTPUT);

  // Estado inicial: todos los relés apagados (LOW = relés desactivados)
  digitalWrite(RELAYA, LOW);
  digitalWrite(RELAYB, LOW);
}

void loop() {
  if (digitalRead(RUN_BUTTON) == HIGH) {
    sendJSON.clear();
    delay(100);

    if (digitalRead(RUN_BUTTON) == LOW) {
      sendJSON["Run"] = "OK_MT";        // Envio de corriente JSON para corto
      serializeJson(sendJSON, Serial);  // Envío de datos por JSON a la PagWeb
      Serial.println();
    }
  }
}
