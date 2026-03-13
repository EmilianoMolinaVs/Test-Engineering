
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
#define NEOP_ON 17   // Alimentación del Neopixel
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
#define PIN_2 2
#define PIN_3 3
#define PIN_8 8
#define PIN_9 9
#define PIN_14 14
#define PIN_11 11
#define PIN_10 10
#define PIN_15 15

// GPIOS Analógicos para lectura de temperatura
#define PIN_ANALOG0 26
#define PIN_ANALOG1 27
#define PIN_ANALOG2 28
#define PIN_ANALOG3 29


Adafruit_ST7735 display = Adafruit_ST7735(CS_PIN, DC_PIN, SDA_PIN, SCL_PIN, RST_PIN);

// Objeto NeoPixel: 1 LED, en pin NEOP_PIN, tipo GRB + 800KHz
Adafruit_NeoPixel np(1, NEOP_PIN, NEO_GRB + NEO_KHZ800);

// Objeto DRV en I2C y declaraciones
Adafruit_DRV2605 drv;
String statusI2C;
bool stateI2C = false;

// ==== Declaración de variables ====
String JSON_entrada;                  // Cadena para almacenar datos JSON entrantes
StaticJsonDocument<200> receiveJSON;  // Documento JSON para recibir datos
String JSON_salida;                   // Variable que envía el JSON de datos
StaticJsonDocument<200> sendJSON;     // Documento JSON para enviar datos

// ===== PROTOTIPOS DE FUNCIONES =====
void demoLED();  // Función de demostración para LEDs
bool i2cCheckDevice(uint8_t);
void hapticMode();
float readTempC(uint8_t TMP_PIN);
bool testGpios(uint8_t gpioA, uint8_t gpioB);
bool testSequence(uint8_t gpioOut, uint8_t gpioIn);


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
  display.print("RP2040");


  // ==== Chequeo e inicialización de Haptic Motor por I2C ====
  Wire.setSDA(SDA_OLED);
  Wire.setSCL(SCL_OLED);
  Wire.begin();
  if (!drv.begin()) {
    Serial.println("Could not find DRV2605");
    stateI2C = false;
    statusI2C = "FAIL";
  } else {
    stateI2C = true;
    statusI2C = "OK";
    Serial.println("Haptic Motor inicializado...");
    drv.selectLibrary(1);
    drv.setMode(DRV2605_MODE_INTTRIG);
  }

  // Configurar pin del LED integrado como salida
  pinMode(LED_BUIL, OUTPUT);
  pinMode(NEOP_ON, OUTPUT); 
  digitalWrite(NEOP_ON, HIGH); 

  // Inicializar NeoPixel
  np.begin();  // Inicialización del objeto NeoPixel
  np.clear();  // Limpiar estado inicial (apagar todos los LEDs)
  np.show();   // Mostrar cambios
}


void loop() {
  // Verificar si hay datos disponibles en Serial1 (comunicación con ESP32)
  if (Serial.available() || Serial1.available()) {

    if (Serial.available()) {
      JSON_entrada = Serial.readStringUntil('\n');  // Leer la cadena JSON hasta encontrar un salto de línea
    } else {
      JSON_entrada = Serial1.readStringUntil('\n');  // Leer la cadena JSON hasta encontrar un salto de línea
    }

    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);  // Deserializar el JSON recibido

    if (!error) {  // Si no hay error en la deserialización

      String Function = receiveJSON["Function"];  // Extraer la función solicitada del JSON

      int opc = 0;
      if (Function == "ping_rp") opc = 1;       // {"Function":"ping_rp"}
      else if (Function == "hmotor") opc = 2;   // {"Function":"hmotor"}
      else if (Function == "analog") opc = 3;   // {"Function":"analog"}
      else if (Function == "testAll") opc = 4;  // {"Function":"testAll"}

      switch (opc) {
        case 1:  // Función ping
          {
            sendJSON.clear();                  // Limpiar documento JSON de salida
            sendJSON["ping"] = "pong";         // Respuesta de ping
            serializeJson(sendJSON, Serial1);  // Serializar y enviar por Serial1
            Serial1.println();                 // Enviar salto de línea
            break;
          }

        case 2:
          {
            sendJSON.clear();
            if (stateI2C) {
              Serial.println("Inicio de Haptic Motor...");
              for (int i = 0; i < 5; i++) {
                hapticMode();
                delay(200);
              }
            } else {
              sendJSON["error"] = "haptic motor no initialized";
              serializeJson(sendJSON, Serial);
              Serial.println();

              serializeJson(sendJSON, Serial1);  // Serializar y enviar por Serial1
              Serial1.println();                 // Enviar salto de línea
            }
            break;
          }

        case 3:  // Lectura de valores analógicos
          {
            int delay_ms = 200;

            float analog0 = readTempC(PIN_ANALOG0);
            delay(delay_ms);
            float analog1 = readTempC(PIN_ANALOG1);
            delay(delay_ms);
            float analog2 = readTempC(PIN_ANALOG2);
            delay(delay_ms);
            float analog3 = readTempC(PIN_ANALOG3);
            delay(delay_ms);

            Serial.println("Lectura analógico 0: " + String(analog0));
            Serial.println("Lectura analógico 1: " + String(analog1));
            Serial.println("Lectura analógico 2: " + String(analog2));
            Serial.println("Lectura analógico 3: " + String(analog3));
            sendJSON["analog0"] = String(analog0);
            sendJSON["analog1"] = String(analog1);
            sendJSON["analog2"] = String(analog2);
            sendJSON["analog3"] = String(analog3);

            serializeJson(sendJSON, Serial1);  // Serializar y enviar por Serial1
            Serial1.println();                 // Enviar salto de línea
            break;
          }

        case 4:  // Test Completo
          {
            sendJSON.clear();
            int delay_ms = 100;

            display.fillScreen(ST77XX_BLACK);  // Limpiar pantalla
            display.setCursor(10, 10);
            display.print("Test de GPIOS RP2040");

            // Línea separadora
            display.drawLine(0, 25, 160, 25, ST77XX_GREEN);

            // ==== Estado de Bloque I2C ====
            sendJSON["i2c_rp"] = statusI2C;
            display.setCursor(10, 35);
            display.print("I2C: ");
            display.println(statusI2C);
            for (int i = 0; i < 5; i++) {
              hapticMode();
              delay(200);
            }

            // ==== Chequeo de GPIOs digitales con secuencia lógica ====
            display.setCursor(10, 45);
            bool stateDig0 = testGpios(PIN_2, PIN_3);
            delay(delay_ms);
            bool stateDig1 = testGpios(PIN_8, PIN_9);
            delay(delay_ms);
            bool stateDig2 = testGpios(PIN_10, PIN_15);
            delay(delay_ms);
            bool stateDig3 = testGpios(PIN_11, PIN_14);
            delay(delay_ms);

            if (stateDig0 && stateDig1 && stateDig2 && stateDig3) {
              sendJSON["dig_rp"] = "OK";
              display.println("Gpios Dig: OK");
            } else {
              sendJSON["dig_rp"] = "FAIL";
              display.println("Gpios Dig: FAIL");

              JsonArray err = sendJSON.createNestedArray("error_dig");

              if (!stateDig0) err.add("GPIO2-3");
              if (!stateDig1) err.add("GPIO8-9");
              if (!stateDig2) err.add("GPIO10-15");
              if (!stateDig3) err.add("GPIO11-14");
            }

            // ==== Chequeo de GPIOs analógicos con lectura de temperatura ====
            display.setCursor(10, 55);
            float analog0 = readTempC(PIN_ANALOG0);
            delay(delay_ms);
            float analog1 = readTempC(PIN_ANALOG1);
            delay(delay_ms);
            float analog2 = readTempC(PIN_ANALOG2);
            delay(delay_ms);
            float analog3 = readTempC(PIN_ANALOG3);
            delay(delay_ms);

            // Validar rango esperado
            bool a0 = (analog0 > 15 && analog0 < 35);
            bool a1 = (analog1 > 15 && analog1 < 35);
            bool a2 = (analog2 > 15 && analog2 < 35);
            bool a3 = (analog3 > 15 && analog3 < 35);

            if (a0 && a1 && a2 && a3) {
              sendJSON["analog_rp"] = "OK";
              display.println("Gpios Analog: OK");
            } else {
              sendJSON["analog_rp"] = "FAIL";
              display.println("Gpios Analog: FAIL");
              JsonArray errA = sendJSON.createNestedArray("error_analog");
              if (!a0) errA.add("A0");
              if (!a1) errA.add("A1");
              if (!a2) errA.add("A2");
              if (!a3) errA.add("A3");
            }

            serializeJson(sendJSON, Serial);
            Serial.println();

            serializeJson(sendJSON, Serial1);  // Serializar y enviar por Serial1
            Serial1.println();                 // Enviar salto de línea
          }

        default: break;  // No hacer nada para opciones no reconocidas
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
  np.setPixelColor(0, np.Color(50, 0, 0));  // Rojo
  np.show();
  delay(delay_ms);

  // Verde: LED integrado apagado, NeoPixel verde
  digitalWrite(LED_BUIL, LOW);
  np.setPixelColor(0, np.Color(0, 50, 0));  // Verde
  np.show();
  delay(delay_ms);

  // Azul: LED integrado encendido, NeoPixel azul
  digitalWrite(LED_BUIL, HIGH);
  np.setPixelColor(0, np.Color(0, 0, 50));  // Azul
  np.show();
  delay(delay_ms);

  // Blanco: LED integrado apagado, NeoPixel blanco
  digitalWrite(LED_BUIL, LOW);
  np.setPixelColor(0, np.Color(50, 0, 50));  // Blanco
  np.show();
  delay(delay_ms);
}

// Función de escaneo de Dir I2C
bool i2cCheckDevice(uint8_t address) {
  Wire.beginTransmission(address);
  byte error = Wire.endTransmission();
  return (error == 0);
}

// Función de modo de operación de HapticMotor
void hapticMode() {
  drv.setWaveform(0, 118);  // vibración fuerte
  drv.setWaveform(1, 0);    // fin
  drv.go();
  delay(100);
}

// Función de lectura analógica de sensor TMP235
float readTempC(uint8_t TMP_PIN) {
  const int N = 20;
  long sum_adc = 0;

  for (int i = 0; i < N; i++) {
    sum_adc += analogRead(TMP_PIN);
    //Serial.println("analogRead: " + String(analogRead(TMP_PIN)));
    delay(5);
  }

  float adc = sum_adc / (float)N;
  float v_mv = adc * (3300.0 / 4095.0);
  float tempC = (v_mv - 500.0f) / 10.0f;  // Curva: 0°C = 500mV, 10mV/°C
  return tempC;
}


/*
 * testGpios(uint8_t gpioA, uint8_t gpioB): Prueba integridad bidireccional
 * Realiza pruebas en ambas direcciones: A->B y B->A
 * 
 * Parámetros:
 *   - gpioA: Primer pin GPIO a probar
 *   - gpioB: Segundo pin GPIO a probar
 * Retorna:
 *   - true si ambas pruebas son exitosas
 *   - false si hay fallos en cualquier dirección
 */
bool testGpios(uint8_t gpioA, uint8_t gpioB) {

  // Prueba A -> B
  bool resultAB = testSequence(gpioA, gpioB);
  if (!resultAB) return false;

  delay(10);  // Pausa entre pruebas

  // Prueba B -> A (bidireccional)
  bool resultBA = testSequence(gpioB, gpioA);
  if (!resultBA) return false;

  return true;  // Ambas pruebas correctas
}

/*
 * testSequence(uint8_t gpioOut, uint8_t gpioIn): Prueba secuencia de bit
 * Envía un patrón de bits a través de gpioOut y verifica lectura en gpioIn
 * 
 * Parámetros:
 *   - gpioOut: Pin configurado como salida
 *   - gpioIn: Pin configurado como entrada
 * Retorna:
 *   - true si todos los bits coinciden
 *   - false si hay algún error de correspondencia
 */
bool testSequence(uint8_t gpioOut, uint8_t gpioIn) {

  // Configuración de pines
  pinMode(gpioOut, OUTPUT);
  pinMode(gpioIn, INPUT_PULLDOWN);  // Configuración con resistencia pull-down

  // Patrón de prueba: secuencia de bits con patrones de repetición
  uint8_t testPattern[] = { 1, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1, 1 };

  // Envío y verificación de cada bit del patrón
  for (int i = 0; i < sizeof(testPattern); i++) {

    digitalWrite(gpioOut, testPattern[i]);  // Envía bit
    delay(10);                              // Espera de estabilización

    int readValue = digitalRead(gpioIn);  // Lee bit

    // Si la lectura no coincide con lo enviado, fallido
    if (readValue != testPattern[i]) {
      return false;
    }
  }

  return true;  // Todos los bits coincidieron correctamente
}