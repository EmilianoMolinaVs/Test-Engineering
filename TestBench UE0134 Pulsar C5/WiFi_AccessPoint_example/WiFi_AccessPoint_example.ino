#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

const char *ssid = "IngPruebas-Master";
const char *password = "cachirula";

WebServer server(80);

void handlePing() {
  if (!server.hasArg("plain")) {
    server.send(400, "text/plain", "No se recibió JSON");
    return;
  }

  String body = server.arg("plain");
  JsonDocument docReq;
  DeserializationError error = deserializeJson(docReq, body);

  if (error) {
    server.send(400, "text/plain", "JSON Inválido");
    return;
  }

  // Usar las mismas llaves que envía el Cliente
  String dispositivo = docReq["dispositivo"] | "desconocido";
  String mensajeRecibido = docReq["mensaje"] | "sin mensaje";
  Serial.println("Mensaje de " + dispositivo + ": " + mensajeRecibido);

  // Responder con las llaves exactas que espera el Cliente
  JsonDocument docRes;
  docRes["respuesta"] = "pong - conexion validada";
  docRes["master_uptime_ms"] = millis();

  String jsonResponse;
  serializeJson(docRes, jsonResponse);

  server.sendHeader("Connection", "close");
  server.send(200, "application/json", jsonResponse);
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false); // Evita latencia y timeouts -11
  WiFi.softAP(ssid, password);

  Serial.print("Master AP IP: ");
  Serial.println(WiFi.softAPIP());

  server.on("/ping", HTTP_POST, handlePing);
  server.begin();

  Serial.println("Servidor HTTP iniciado. Esperando pings...");
}

void loop() {
  server.handleClient();
}