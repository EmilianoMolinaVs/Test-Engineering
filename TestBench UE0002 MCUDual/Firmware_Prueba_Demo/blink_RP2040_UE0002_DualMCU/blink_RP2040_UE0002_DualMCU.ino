
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
#include <SPI.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <HardwareSerial.h>
// #include <Adafruit_SSD1306.h>
#include <Adafruit_DRV2605.h>
#include <Adafruit_ST7735.h>  // Librería para ST7735
#include "Adafruit_GFX.h"


// Definiciones de pines
#define NEOP_PIN 16  // Pin para el NeoPixel
#define LED_BUIL 25  // Pin para el LED integrado
#define SCL_OLED 13  // SCL OLED GPIO 13 JST
#define SDA_OLED 12  // SDA OLED GPIO 12 JST
#define CS_PIN 21    // Chip Select SPI Display
#define DC_PIN 23    // DC SPI Display
#define RST_PIN 22   // Reset SPI Display
#define SCL_PIN 18   // Señal de Reloj SPI Display
#define SDA_PIN 19   // Bus de datos SPI Display

// GPIOS a validar por medio de secuencia
#define PIN_14 14
#define PIN_11 11
#define PIN_10 10
#define PIN_15 15
#define PIN_22 22
#define PIN_23 23
#define PIN_6 6
#define PIN_25 25

Adafruit_ST7735 display = Adafruit_ST7735(CS_PIN, DC_PIN, SDA_PIN, SCL_PIN, RST_PIN);

// Objeto NeoPixel: 1 LED, en pin NEOP_PIN, tipo GRB + 800KHz
Adafruit_NeoPixel np(1, NEOP_PIN, NEO_GRB + NEO_KHZ800);

// Objeto DRV en I2C y declaraciones
Adafruit_DRV2605 drv;
String statusI2C;
bool stateI2c = false;

// ==== Declaración de variables ====
String JSON_entrada;                  // Cadena para almacenar datos JSON entrantes
StaticJsonDocument<200> receiveJSON;  // Documento JSON para recibir datos
String JSON_salida;                   // Variable que envía el JSON de datos
StaticJsonDocument<200> sendJSON;     // Documento JSON para enviar datos

// ===== PROTOTIPOS DE FUNCIONES =====
void demoLED();  // Función de demostración para LEDs
bool i2cCheckDevice(uint8_t);
void hapticMode();


void setup() {

  // Inicializar comunicación serial
  Serial.begin(115200);   // Puerto serial principal (USB)
  Serial1.begin(115200);  // Puerto serial secundario (GPIO0 GPIO1) para puente con ESP32
  Serial.println("Serial inicializado...");


  // ==== Chequeo e inicialización de Display por SPI ====
  display.initR(INITR_MINI160x80);
  display.setRotation(1);
  display.fillScreen(ST77XX_BLACK);

  // Barra superior
  display.fillRect(0, 0, 160, 15, ST77XX_BLUE);

  display.setTextColor(ST77XX_WHITE);
  display.setTextSize(1);
  display.setCursor(5, 4);
  display.print("UELECTRONICS");

  // Titulo grande
  display.setTextColor(ST77XX_CYAN);
  display.setTextSize(2);
  display.setCursor(10, 25);
  display.print("MCU");

  // Subtitulo
  display.setTextColor(ST77XX_YELLOW);
  display.setTextSize(2);
  display.setCursor(65, 25);
  display.print("DUAL");

  // Línea separadora
  display.drawLine(0, 50, 160, 50, ST77XX_GREEN);

  // Mensaje estado
  display.setTextSize(1);
  display.setTextColor(ST77XX_WHITE);
  display.setCursor(20, 60);
  display.print("RP2040 ONLINE");


  // ==== Chequeo e inicialización de Haptic Motor por I2C ====
  Wire.setSDA(SDA_OLED);
  Wire.setSCL(SCL_OLED);
  Wire.begin();
  if (!drv.begin()) {
    Serial.println("Could not find DRV2605");
  } else {
    stateI2c = true;
    Serial.println("Haptic Motor inicializado...");
    drv.selectLibrary(1);
    drv.setMode(DRV2605_MODE_INTTRIG);
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
      if (Function == "hp") opc = 2;  // {"Function":"hp"}

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

        case 2:
          {
            Serial.println("Vibrandoooo0o0o0o0");
            for (int i = 0; i < 5; i++) {
              hapticMode();
              delay(1000);
            }
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

void hapticMode() {
  drv.setWaveform(0, 118);  // vibración fuerte
  drv.setWaveform(1, 0);    // fin
  drv.go();
  delay(100);
}
