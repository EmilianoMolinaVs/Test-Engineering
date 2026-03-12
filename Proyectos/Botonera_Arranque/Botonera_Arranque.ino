// --- BIBLIOTECAS ---
#include <ArduinoJson.h>
#include <Arduino.h>

#define RUN_BUTTON 6  // Botón de Arranque

// ==== Variables de inicialización
String JSON_entrada;  // Variable que recibe al JSON en crudo de PagWeb
StaticJsonDocument<200> receiveJSON;

String JSON_lectura;  // Variable que envía el JSON de datos
StaticJsonDocument<200> sendJSON;

void setup() {
  Serial.begin(115200); 
  pinMode(RUN_BUTTON, INPUT);
}

void loop() {
  if (digitalRead(RUN_BUTTON) == HIGH) {
    sendJSON.clear();
    delay(100);

    if (digitalRead(RUN_BUTTON) == LOW) {
      sendJSON["Run"] = "OK";           // Envio de corriente JSON para corto
      serializeJson(sendJSON, Serial);  // Envío de datos por JSON a la PagWeb
      Serial.println();
    }
  }
}
