
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
#include "ue_i2c_icp_10111_sen.h"

ICP101xx sensor;

// ===== DEFINES =====
// Configuración de OLED I2C
#define SCL_OLED 23                                                        // SCL pin
#define SDA_OLED 22                                                        // SDA pin
#define OLED_RESET -1                                                      // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_WIDTH 128                                                   // OLED display width, in pixels
#define SCREEN_HEIGHT 64                                                   // OLED display height, in pixels
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);  // Objeto de la OLED
String stateI2C;                                                           // Estado de inicialización OLED
bool debugOLED = false;                                                    // Bandera de debug en OLED
unsigned long contador = 0;


// Función de escaneo de Dir I2C
bool i2cCheckDevice(uint8_t address) {
  Wire.beginTransmission(address);
  byte error = Wire.endTransmission();
  return (error == 0);
}


// ===== SETUP =====
void setup() {
  // Inicializar comunicación serial UART0
  Serial.begin(115200);

  Wire.begin(SDA_OLED, SCL_OLED);

  // Initialize sensor.
  if (!sensor.begin(&Wire)) {
    Serial.println("ERROR: Could not initialize sensor!");
    Serial.println("Check I2C wiring and connections.");
  }

  // Inicialización de OLED por I2C
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
    display.println(F("Test I2C Expander"));
    display.display();  // Mostrar texto inicial
  }
}

void loop() {
  contador++;

  if (debugOLED) {
    display.clearDisplay();

    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println(F("Test I2C Expander"));

    display.setTextSize(2);
    display.setCursor(0, 20);
    display.print("No: ");
    display.println(contador);

    display.display();
  }

  delay(100);  // Ajusta velocidad del conteo


  sensor.measure(sensor.NORMAL);

  float pressure = sensor.getPressurePa();
  float temperature = sensor.getTemperatureC();

  Serial.print("Pressure: ");
  Serial.print(pressure);
  Serial.println(" Pa");

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" °C");
  Serial.println("");

  delay(100);
}