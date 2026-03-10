
/**
 * Firmware para RP2040 en TestBench UE0002 MCUDual
 * 
 * Este sketch maneja la comunicación JSON entre el RP2040 y otros dispositivos,
 * incluyendo el ESP32 a través de Serial1. Implementa funciones de ping/pong
 * y una demostración de LEDs con NeoPixel.
 * 
 * Autor: [Tu Nombre]
 * Fecha: [Fecha]
 */

#include <Arduino.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <HardwareSerial.h>
#include <Adafruit_SSD1306.h>

// Definiciones de pines
#define NEOP_PIN 16  // Pin para el NeoPixel
#define LED_BUIL 25  // Pin para el LED integrado
#define SCL_OLED 13  // SCL OLED GPIO 13 JST
#define SDA_OLED 12  // SDA OLED GPIO 12 JST

// GPIOS a validar por medio de secuencia
#define PIN_14 14
#define PIN_11 11
#define PIN_10 10
#define PIN_15 15
#define PIN_22 22
#define PIN_23 23
#define PIN_6 6
#define PIN_25 25

// Objeto NeoPixel: 1 LED, en pin NEOP_PIN, tipo GRB + 800KHz
Adafruit_NeoPixel np(1, NEOP_PIN, NEO_GRB + NEO_KHZ800);

// Objeto OLED y declaraciones
#define OLED_RESET -1                                                      // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_WIDTH 128                                                   // OLED display width, in pixels
#define SCREEN_HEIGHT 64                                                   // OLED display height, in pixels
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);  // Objeto de la OLED
String statusOLED;                                                         // Resultado de inicialización I2C
bool stateOLED = false;                                                    // Estado inicial del bloque I2C

// ==== Declaración de variables ====
String JSON_entrada;                  // Cadena para almacenar datos JSON entrantes
StaticJsonDocument<200> receiveJSON;  // Documento JSON para recibir datos
String JSON_salida;                   // Variable que envía el JSON de datos
StaticJsonDocument<200> sendJSON;     // Documento JSON para enviar datos

// ===== PROTOTIPOS DE FUNCIONES =====
void demoLED();  // Función de demostración para LEDs
bool i2cCheckDevice(uint8_t);


void setup() {

  // Inicializar comunicación serial
  Serial.begin(115200);   // Puerto serial principal (USB)
  Serial1.begin(115200);  // Puerto serial secundario (GPIO0 GPIO1) para puente con ESP32

  // ==== Chequeo e inicialización de OLED por I2C ====
  Wire.setSDA(SDA_OLED);
  Wire.setSCL(SCL_OLED);
  Wire.begin();  // Inicialización de I2C de OLED

  if (!i2cCheckDevice(0x3C)) {
    Serial.println("SSD1306 no encontrada en I2C");
    statusOLED = "FAIL";
  } else {
    Serial.println("SSD1306 detectada");
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    statusOLED = "OK";
    stateOLED = true;
  }

  if (stateOLED) {  // Debug de inicialización de OLED
    display.clearDisplay();
    display.setTextSize(1.5);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(20, 0);
    display.println(F("Test de Prueba"));
    display.setCursor(30, 10);
    display.println(F("MCU DualOne"));
    display.display();  // Show initial text
  }


  // Configurar pin del LED integrado como salida
  pinMode(LED_BUIL, OUTPUT);

  // Inicializar NeoPixel
  np.begin();  // Inicialización del objeto NeoPixel
  np.clear();  // Limpiar estado inicial (apagar todos los LEDs)
  np.show();   // Mostrar cambios
}


void loop() {
  // Verificar si hay datos disponibles en Serial1 (comunicación con ESP32)
  if (Serial1.available()) {
    // Leer la cadena JSON hasta encontrar un salto de línea
    JSON_entrada = Serial1.readStringUntil('\n');
    // Deserializar el JSON recibido
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);

    // Si no hay error en la deserialización
    if (!error) {
      // Extraer la función solicitada del JSON
      String Function = receiveJSON["Function"];

      // Determinar la opción basada en la función
      int opc = 0;
      if (Function == "ping") opc = 1;
      else if (Function == "passthrough") opc = 2;

      // Ejecutar la acción correspondiente
      switch (opc) {
        case 1:  // Función ping
          {
            sendJSON.clear();                  // Limpiar documento JSON de salida
            sendJSON["ping"] = "pong";         // Respuesta de ping
            serializeJson(sendJSON, Serial1);  // Serializar y enviar por Serial1
            Serial1.println();                 // Enviar salto de línea
            break;
          }

        case 2:  // Función passthrough
          {
            sendJSON.clear();
            sendJSON["ping"] = "pong_RP2040";  // Respuesta específica del RP2040
            serializeJson(sendJSON, Serial1);
            Serial1.println();
            break;
          }

        default: break;  // No hacer nada para opciones no reconocidas
      }
    }
  }

  // Verificar si hay datos disponibles en Serial (comunicación local/USB)
  if (Serial.available()) {
    // Leer la cadena JSON hasta encontrar un salto de línea
    JSON_entrada = Serial.readStringUntil('\n');
    // Deserializar el JSON recibido
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);

    // Si no hay error en la deserialización
    if (!error) {
      // Extraer la función solicitada del JSON
      String Function = receiveJSON["Function"];

      // Determinar la opción basada en la función
      int opc = 0;
      if (Function == "ping") opc = 1;

      // Ejecutar la acción correspondiente
      switch (opc) {
        case 1:  // Función ping local
          {
            sendJSON.clear();                 // Limpiar documento JSON de salida
            sendJSON["ping"] = "pong_local";  // Respuesta local
            serializeJson(sendJSON, Serial);  // Serializar y enviar por Serial
            Serial.println();                 // Enviar salto de línea
            break;
          }
      }
    }
  }


  // Si no hay datos en ninguno de los puertos seriales, ejecutar demostración de LEDs
  if (!Serial.available() && !Serial1.available()) {
    demoLED();
  }
}


// ===== FUNCIONES ADICIONALES =====
// Función de demostración para LEDs
void demoLED() {
  // Retraso entre cambios de color en milisegundos
  int delay_ms = 200;

  // Secuencia de colores para la demostración
  // Rojo: LED integrado encendido, NeoPixel rojo
  digitalWrite(LED_BUIL, HIGH);
  np.setPixelColor(0, np.Color(255, 0, 0));  // Rojo
  np.show();
  delay(delay_ms);

  // Verde: LED integrado apagado, NeoPixel verde
  digitalWrite(LED_BUIL, LOW);
  np.setPixelColor(0, np.Color(0, 255, 0));  // Verde
  np.show();
  delay(delay_ms);

  // Azul: LED integrado encendido, NeoPixel azul
  digitalWrite(LED_BUIL, HIGH);
  np.setPixelColor(0, np.Color(0, 0, 255));  // Azul
  np.show();
  delay(delay_ms);

  // Blanco: LED integrado apagado, NeoPixel blanco
  digitalWrite(LED_BUIL, LOW);
  np.setPixelColor(0, np.Color(255, 255, 255));  // Blanco
  np.show();
  delay(delay_ms);
}

// Función de escaneo de Dir I2C
bool i2cCheckDevice(uint8_t address) {
  Wire.beginTransmission(address);
  byte error = Wire.endTransmission();
  return (error == 0);
}
