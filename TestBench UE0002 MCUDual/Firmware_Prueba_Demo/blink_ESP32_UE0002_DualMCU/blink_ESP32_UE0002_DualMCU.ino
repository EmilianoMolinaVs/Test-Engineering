
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
#include <SPI.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <HardwareSerial.h>
#include <Adafruit_SSD1306.h>
#include "bme68xLibrary.h"

// ===== DEFINES =====
// Configuración de OLED I2C
#define SCL_OLED 21                                                        // SCL pin
#define SDA_OLED 22                                                        // SDA pin
#define OLED_RESET -1                                                      // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_WIDTH 128                                                   // OLED display width, in pixels
#define SCREEN_HEIGHT 64                                                   // OLED display height, in pixels
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);  // Objeto de la OLED
String stateI2C;                                                           // Estado de inicialización OLED
bool debugOLED = false;                                                    // Bandera de debug en OLED

// Pines para LEDs RGB (activos a BAJAS)
#define RGB_RED 25
#define RGB_GREEN 26
#define RGB_BLUE 4

// Pines para comunicación serial con RP2040
#define RX2 16  // RX Serial 2 para puente con RP2040
#define TX2 17  // TX Serial 2 para puente con RP2040

// Pines para SPI BME688 Sensor de Temperatura
#define CS_PIN 23       // Chip Select para SPI
#define MOSI_PIN 19     // MOSI para SPI SDI
#define MISO_PIN 5      // MISO para SPI SDO
#define SCK_PIN 18      // SCK para SPI
SPIClass mySPI(VSPI);   // Bus SPI #0 para ESP32
Bme68x bme;             // Objeto de Sensor de Temperatura
bool stateSPI = false;  // Estado de inicialización de BME688

// Pines con evaluación de entrada y salida
#define PIN_15 15
#define PIN_02 2
#define PIN_39 39
#define PIN_36 36

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
String testGpios(uint8_t gpioA, uint8_t gpioB);
bool testSequence(uint8_t gpioOut, uint8_t gpioIn);
int readADCavg(int pin);

// ===== SETUP =====
void setup() {
  // Inicializar comunicación serial UART0
  Serial.begin(115200);

  // Inicializar comunicación serial UART2 para puente con RP2040
  Bridge.begin(115200, SERIAL_8N1, RX2, TX2);

  // Configurar WiFi en modo estación (necesario para obtener MAC)
  WiFi.mode(WIFI_MODE_STA);

  // Inicialización de OLED por I2C
  Wire.begin(SDA_OLED, SCL_OLED);
  if (!i2cCheckDevice(0x3C)) {
    stateI2C = "FAIL";
  } else {
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    stateI2C = "OK";
    debugOLED = true;
  }

  if (debugOLED) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, 0);
    display.println(F("Test DualMCU ESP32"));
    display.display();  // Mostrar texto inicial
  }

  // Inicializar SPI con pines personalizados
  mySPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN);
  bme.begin(CS_PIN, mySPI);  // Iniciar BME688 en modo SPI
  if (bme.checkStatus() == BME68X_ERROR) {
    Serial.println("Error: no se pudo inicializar el sensor.");
  } else if (bme.checkStatus() == BME68X_WARNING) {
    Serial.println("Advertencia: " + bme.statusString());
  } else {
    stateSPI = true;
    bme.setTPH();
    bme.setHeaterProf(300, 100);
  }

  analogSetPinAttenuation(36, ADC_11db);
  analogSetPinAttenuation(39, ADC_11db);

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

      // ==== Operaciones de validación de ESP32 ====
      if (Function == "ping") opc = 1;             // Comando ping: {"Function":"ping"}
      else if (Function == "mac") opc = 2;         // Comando MAC: {"Function":"mac"}
      else if (Function == "bme") opc = 3;         // Comando BME: {"Function":"bme"}
      else if (Function == "test_esp32") opc = 4;  // Comando de Test: {"Function":"test_esp32"}
      else if (Function == "vn_sensor") opc = 5;   // Comando de Sensores: {"Function":"vn_sensor"}

      // ==== Operaciones de validación de RP2040 ====
      else if (Function == "passthrough") opc = 6;  // Comando passthrough: {"Function":"passthrough"}
      else if (Function == "hmotor") opc = 7;       // Comando haptic motor: {"Function":"hmotor"}
      else if (Function == "analog_rp") opc = 8;    // Comando analógicos RP: {"Function":"analog_rp"}
      else if (Function == "test_rp") opc = 9;      // Comando Test Rp2040: {"Function":"test_rp"}

      // Ejecutar la acción correspondiente
      switch (opc) {
        case 1:  // Ping local
          {
            sendJSON.clear();
            sendJSON["ping"] = "pong_ESP32";
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }

        case 2:  // Obtener MAC
          {
            sendJSON.clear();
            String mac = WiFi.macAddress();
            sendJSON["mac"] = mac;
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }

        case 3:  // Lectura de sensor de temperatura
          {
            sendJSON.clear();

            if (!stateSPI) {
              sendJSON["stateSPI"] = "FAIL";
              serializeJson(sendJSON, Serial);
              Serial.println();
              break;
            }

            sendJSON["stateSPI"] = "OK";
            bme.setTPH();
            bme.setHeaterProf(300, 100);

            bme68xData data;

            for (int i = 0; i < 10; i++) {
              sendJSON.clear();
              bme.setOpMode(BME68X_FORCED_MODE);
              delayMicroseconds(bme.getMeasDur());

              if (bme.fetchData()) {
                bme.getData(data);
                sendJSON["temp"] = data.temperature;
                sendJSON["press"] = data.pressure;
                sendJSON["hum"] = data.humidity;
                sendJSON["gas"] = data.gas_resistance;
              } else {
                sendJSON["temp"] = "-";
                sendJSON["press"] = "-";
                sendJSON["hum"] = "-";
                sendJSON["gas"] = "-";
              }

              serializeJson(sendJSON, Serial);
              Serial.println();
              delay(500);
            }

            break;
          }


        case 4:  // Test completo de ESP32
          {
            sendJSON.clear();
            if (debugOLED) {
              display.clearDisplay();
              display.setCursor(10, 0);
              display.println(F("Test DualMCU ESP32"));
              display.display();  // Mostrar texto inicial
              display.setCursor(5, 15);
              display.print("I2C: ");
              display.println(stateI2C);
              display.display();  // Mostrar texto
            }
            sendJSON["i2c_esp32"] = stateI2C;

            // Estado de bus SPI
            if (debugOLED) display.setCursor(5, 25);
            if (debugOLED) display.print("SPI: ");
            if (stateSPI) {
              if (debugOLED) display.println("OK");
              sendJSON["spi_esp32"] = "OK";
            } else {
              if (debugOLED) display.println("FAIL");
              sendJSON["spi_esp32"] = "FAIL";
            }
            if (debugOLED) display.display();  // Mostrar texto

            // Evaluación Pines Par Digital
            String stateDig = testGpios(PIN_02, PIN_15);
            if (debugOLED) {
              display.setCursor(5, 35);
              display.print("GPIOS Dig: ");
              display.println(stateDig);
              display.display();  // Mostrar texto
            }
            sendJSON["dig_esp32"] = stateDig;

            // Evaluación Pines Analógicos ADC
            float analog1 = readADCavg(36);
            delay(500);
            float analog2 = readADCavg(39);
            //Serial.println(analog1);
            //Serial.println(analog2);

            if (debugOLED) {
              display.setCursor(5, 45);
              display.print("Analog: ");
              display.println(String(analog1) + "|" + String(analog2));
              display.display();  // Mostrar texto
            }

            String statusAnalog = "FAIL";
            if (analog1 > 3500 && analog2 > 3500) {
              statusAnalog = "OK";
            }

            sendJSON["analog_esp32"] = statusAnalog;
            //sendJSON["analog_esp32"] = String(analog1) + "|" + String(analog2);  // Corregido: enviar valores analógicos

            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }


        case 5:
          {
            for (int i = 0; i < 20; i++) {
              float analog1 = readADCavg(36);
              delay(1000);
              float analog2 = readADCavg(39);
              Serial.println("I36: " + String(analog1));
              Serial.println("I39: " + String(analog2));
              delay(100);
            }
            break;
          }


          // ==== Comandos de Test de RP2040 ====
        case 6:  // Passthrough
          {
            sendJSON.clear();
            sendJSON["Function"] = "ping_rp";
            serializeJson(sendJSON, Bridge);
            Bridge.println();
            break;
          }

        case 7:  // Control de Haptic Motor
          {
            sendJSON.clear();
            sendJSON["Function"] = "hmotor";
            serializeJson(sendJSON, Bridge);
            Bridge.println();
            break;
          }

        case 8:  // Test de Analógicos de RP2040
          {
            sendJSON.clear();
            sendJSON["Function"] = "analog";
            serializeJson(sendJSON, Bridge);
            Bridge.println();
            break;
          }

        case 9:  // Test Completo de GPIOS RP2040
          {
            sendJSON.clear();
            sendJSON["Function"] = "testAll";
            serializeJson(sendJSON, Bridge);
            Bridge.println();
            break;
          }

        default: break;
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

  // Rojo
  digitalWrite(RGB_RED, LOW);
  digitalWrite(RGB_GREEN, HIGH);
  digitalWrite(RGB_BLUE, HIGH);
  delay(delay_ms);

  // Verde
  digitalWrite(RGB_RED, HIGH);
  digitalWrite(RGB_GREEN, LOW);
  digitalWrite(RGB_BLUE, HIGH);
  delay(delay_ms);

  // Azul
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


/*
 * testGpios(uint8_t gpioA, uint8_t gpioB): Prueba integridad bidireccional
 * Realiza pruebas en ambas direcciones: A->B y B->A
 * 
 * Parámetros:
 *   - gpioA: Primer pin GPIO a probar
 *   - gpioB: Segundo pin GPIO a probar
 * Retorna:
 *   - "OK" si ambas pruebas son exitosas
 *   - "Fail" si hay fallos en cualquier dirección
 */
String testGpios(uint8_t gpioA, uint8_t gpioB) {

  // Prueba A -> B
  bool resultAB = testSequence(gpioA, gpioB);
  if (!resultAB) return "Fail";

  delay(10);  // Pausa entre pruebas

  // Prueba B -> A (bidireccional)
  bool resultBA = testSequence(gpioB, gpioA);
  if (!resultBA) return "Fail";

  return "OK";  // Ambas pruebas correctas
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

// Función de promedio de lecturas en ADC
int readADCavg(int pin) {
  long sum = 0;
  for (int i = 0; i < 8; i++) {
    analogRead(pin);  // Lectura dummy para estabilizar
    sum += analogRead(pin);
  }
  return sum / 8;
}