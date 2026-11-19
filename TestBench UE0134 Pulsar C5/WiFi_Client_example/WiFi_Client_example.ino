#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char *ssid = "IngPruebas-Master";
const char *password = "cachirula";
const char *serverUrl = "http://192.168.4.1/ping";

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false); // Evita microcortes que provocan el error -11
  WiFi.begin(ssid, password);
  Serial.print("Conectando al Master");

  for (int i = 0; i < 50; i++) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
    } else break;
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n¡Conectado a la red de Pruebas!");
  } else {
    Serial.println("\nNo se pudo conectar al Master.");
  }
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.setReuse(false);   // Fuerza conexión limpia en cada ciclo
    http.setTimeout(5000);  // Tiempo de espera explícito (5 segundos)
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");

    JsonDocument docReq;
    docReq["dispositivo"] = "Esclavo_B";
    docReq["mensaje"] = "Ping para validar conexion";

    String jsonRequest;
    serializeJson(docReq, jsonRequest);
    Serial.println("\nEnviando: " + jsonRequest);

    int httpResponseCode = http.POST(jsonRequest);

    if (httpResponseCode > 0) {
      String payload = http.getString();

      JsonDocument docRes;
      DeserializationError error = deserializeJson(docRes, payload);

      if (!error) {
        String respuestaMaster = docRes["respuesta"] | "sin respuesta";
        unsigned long uptimeMaster = docRes["master_uptime_ms"] | 0;

        Serial.printf("Master respondió (HTTP %d):\n", httpResponseCode);
        Serial.printf(" - Mensaje: %s\n", respuestaMaster.c_str());
        Serial.printf(" - Uptime Master: %lu ms\n", uptimeMaster);
      } else {
        Serial.println("Error al parsear el JSON recibido del Master.");
      }
    } else {
      Serial.printf("Error en la petición HTTP. Código: %d\n", httpResponseCode);
    }

    http.end();
  }

  delay(500);
}