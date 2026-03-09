
/*
 * Blink ESP32 UE0002 DualMCU
 *
 * Este sketch implementa un firmware de demostración para el módulo ESP32 en el TestBench UE0002 MCUDual.
 * Maneja comunicación JSON a través de UART0 y puentea comandos al RP2040 vía UART2.
 * Incluye control básico de LEDs RGB y soporte para comandos como ping, passthrough y obtener MAC.
 *
 * Autor: Emiliano Molina
 * Fecha: 09 Marzo, 2026
 * Versión: 1.0
 */

// ===== INCLUDES =====
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <HardwareSerial.h>
#include <Adafruit_SSD1306.h>

// ===== DEFINES =====
// Configuración de OLED
#define SCL_OLED 22                                                        // SCL pin
#define SDA_OLED 21                                                        // SDA pin
#define OLED_RESET -1                                                      // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_WIDTH 128                                                   // OLED display width, in pixels
#define SCREEN_HEIGHT 64                                                   // OLED display height, in pixels
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);  // Objeto de la OLED
String status_OLED;                                                        // Estado de inicialización OLED
bool debugOLED = false;                                                    // Bandera de debug en OLED

// Pines para LEDs RGB (activos a BAJAS)
#define RGB_RED 25
#define RGB_GREEN 26
#define RGB_BLUE 4

// Pines para comunicación serial con RP2040
#define RX2 16  // RX Serial 2 para puente con RP2040
#define TX2 17  // TX Serial 2 para puente con RP2040

// ===== VARIABLES GLOBALES =====
// Comunicación JSON
String JSON_entrada;                  // Buffer para JSON recibido
StaticJsonDocument<200> receiveJSON;  // Documento JSON para recepción

String JSON_salida;                // Buffer para JSON a enviar
StaticJsonDocument<200> sendJSON;  // Documento JSON para envío

// Objeto para comunicación serial con RP2040
HardwareSerial Bridge(1);

// ===== PROTOTIPOS DE FUNCIONES =====
void demoLED();  // Función de demostración para LEDs
bool i2cCheckDevice(uint8_t);

// ===== SETUP =====
void setup() {
  // Inicializar comunicación serial UART0
  Serial.begin(115200);
  Serial.println("UART0 listo para comunicación...");

  // Configurar WiFi en modo estación (necesario para obtener MAC)
  WiFi.mode(WIFI_MODE_STA);

  // Inicialización de OLED por I2C
  Wire.begin(SDA_OLED, SCL_OLED);
  if (!i2cCheckDevice(0x3C)) {
    Serial.println("SSD1306 no encontrada en I2C");
    status_OLED = "FAIL";
  } else {
    Serial.println("SSD1306 detectada");
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    status_OLED = "OK";
    debugOLED = true;
  }

  if (debugOLED) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(20, 0);
    display.println(F("Test de Prueba"));
    display.setCursor(30, 10);
    display.println(F("DualMCU"));
    display.display();  // Show initial text
  }

    // Inicializar comunicación serial UART2 para puente con RP2040
    Bridge.begin(115200, SERIAL_8N1, RX2, TX2);

  // Configurar pines de LEDs RGB como salidas
  pinMode(RGB_RED, OUTPUT);
  pinMode(RGB_GREEN, OUTPUT);
  pinMode(RGB_BLUE, OUTPUT);
}

// ===== LOOP =====
void loop() {
  // Verificar si hay datos disponibles en UART0
  if (Serial.available()) {
    // Leer la cadena JSON hasta el salto de línea
    JSON_entrada = Serial.readStringUntil('\n');

    // Deserializar el JSON recibido
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);

    if (!error) {
      // Extraer la función solicitada del JSON
      String Function = receiveJSON["Function"];

      // Determinar la opción basada en la función
      int opc = 0;
      if (Function == "ping") opc = 1;              // Comando ping: {"Function":"ping"}
      else if (Function == "passthrough") opc = 2;  // Comando passthrough: {"Function":"passthrough"}
      else if (Function == "mac") opc = 3;          // Comando MAC: {"Function":"mac"}

      // Ejecutar la acción correspondiente
      switch (opc) {
        case 1:  // Ping
          {
            sendJSON.clear();
            sendJSON["ping"] = "pong";
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }

        case 2:  // Passthrough
          {
            sendJSON.clear();
            sendJSON["Function"] = "passthrough";
            serializeJson(sendJSON, Bridge);
            Bridge.println();
            break;
          }

        case 3:  // Obtener MAC
          {
            sendJSON.clear();
            String mac = WiFi.macAddress();
            sendJSON["mac"] = mac;
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }
      }
    } else {
      // Error en la deserialización del JSON
      sendJSON.clear();
      sendJSON["error"] = "invalid option in UART";
      serializeJson(sendJSON, Serial);
      Serial.println();
    }
  }

  // Condicional para recibir resultados de Test de RP2040
  if (Bridge.available()) {  // JSON de entrada desde RP2040
    JSON_entrada = Bridge.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);
    Serial.println(JSON_entrada);
  }

  // Condicional de Demo
  if (!Serial.available() && !Bridge.available()) {
    demoLED();
  }
}

// ===== FUNCIONES ADICIONALES =====
// Función de demostración para LEDs
void demoLED() {
  int delay_ms = 200;

  digitalWrite(RGB_RED, LOW);
  digitalWrite(RGB_GREEN, HIGH);
  digitalWrite(RGB_BLUE, HIGH);
  delay(delay_ms);

  digitalWrite(RGB_RED, HIGH);
  digitalWrite(RGB_GREEN, LOW);
  digitalWrite(RGB_BLUE, HIGH);
  delay(delay_ms);

  digitalWrite(RGB_RED, HIGH);
  digitalWrite(RGB_GREEN, HIGH);
  digitalWrite(RGB_BLUE, LOW);
  delay(delay_ms);
}


// Función de escaneo de Dir I2C
bool i2cCheckDevice(uint8_t address) {
  Wire.beginTransmission(address);
  byte error = Wire.endTransmission();
  return (error == 0);
}
