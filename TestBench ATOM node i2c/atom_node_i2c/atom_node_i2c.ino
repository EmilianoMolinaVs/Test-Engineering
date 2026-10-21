/**
 * @file I2C_Network_Monitor.ino
 * @brief Monitor continuo de múltiples sensores en I2C mostrado en SSD1306 OLED
 * Hardware: ESP32-C6 (SDA = GPIO 6, SCL = GPIO 7)
 */

#include <Arduino.h>
#include <Wire.h>
#include <DevLab_Button.h>
#include <DevLabDDP.h>
#include <DevLab_I2C_Orchestrator.h>
#include <DevLab_GT365xx.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "ue_i2c_icp_10111_sen.h"

// --- Pines y Configuración I2C (ESP32-C6) ---
constexpr uint8_t I2C_SDA = 6;
constexpr uint8_t I2C_SCL = 7;
constexpr uint32_t I2C_FREQ = 100000;  // 100kHz para estabilidad de red con múltiples nodos

// --- Direcciones I2C ---
constexpr uint8_t TEMT6000_ADDR = 0x20;
constexpr uint8_t LDR_ADDRESS = 0x26U;
constexpr uint8_t OLED_ADDRESS = 0x3C;

// --- Configuración OLED y Gráfica ---
constexpr int OLED_WIDTH = 128;
constexpr int OLED_HEIGHT = 64;
constexpr int GRAPH_LEFT = 27;
constexpr int GRAPH_WIDTH = OLED_WIDTH - GRAPH_LEFT;
constexpr int GRAPH_TOP = 32;
constexpr int GRAPH_BOTTOM = OLED_HEIGHT - 1;
constexpr uint16_t GRAPH_ADC_MIN = 0;
constexpr uint16_t GRAPH_ADC_MAX = 4095;
constexpr uint16_t GRAPH_ADC_MIDDLE = GRAPH_ADC_MIN + ((GRAPH_ADC_MAX - GRAPH_ADC_MIN) / 2);

// --- Instancias de Hardware ---
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
DevLab_I2C_Orchestrator bus(Wire, I2C_FREQ);
DevLabDDP::Master master(bus, DevLabDDP::DEVICE_TEMT6000);
DevLab_Button button(Wire, DevLab_Button::DEFAULT_ADDRESS, I2C_FREQ);
DevLab_GT365xx ldrSensor(Wire, LDR_ADDRESS, I2C_FREQ);
ICP101xx icp_sensor;

// --- Variables de Estado ---
bool temtVerified = false;
bool temtOk = false;
bool ldrOk = false;

// Datos de sensores
uint16_t samples[GRAPH_WIDTH] = { 0 };
uint16_t lastAdc = 0;  // TEMT6000
uint16_t ldrAdc = 0;   // LDR GT36537
float currentPressure = 0.0;
float currentTemp = 0.0;

// Variables de estado del botón
uint32_t pressCount = 0;
bool isPressed = false;

// Temporizadores no bloqueantes
uint32_t lastBtnTime = 0;
uint32_t lastTemtTime = 0;
uint32_t lastLdrTime = 0;
uint32_t lastIcpTime = 0;

// --- Funciones Auxiliares ---
bool readADC0(uint16_t &value) {
  uint8_t bytes[2];
  if (!temtVerified || !master.readCommand(TEMT6000_ADDR, CMD_READ_ADC0, bytes, 2U, 30U)) {
    return false;
  }
  value = (uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8);
  return true;
}

int adcToY(uint16_t value) {
  uint16_t visibleValue = constrain(value, GRAPH_ADC_MIN, GRAPH_ADC_MAX);
  return map(visibleValue, GRAPH_ADC_MIN, GRAPH_ADC_MAX, GRAPH_BOTTOM, GRAPH_TOP);
}

void addSample(uint16_t value) {
  for (int x = 0; x < GRAPH_WIDTH - 1; ++x) {
    samples[x] = samples[x + 1];
  }
  samples[GRAPH_WIDTH - 1] = value;
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Fila 1: Botón I2C
  display.setCursor(0, 0);
  display.print("BTN: ");
  display.print(isPressed ? "PRESS  " : "RELEASE");
  display.print(" Cnt:");
  display.print(pressCount);

  // Fila 2: Presión y Temperatura (ICP10111)
  display.setCursor(0, 10);
  display.print("T:");
  display.print(currentTemp, 1);
  display.print("C P:");
  display.print(currentPressure, 0);

  // Fila 3: Valores de Luz (TEMT6000 y LDR)
  display.setCursor(0, 20);
  display.print("T:");
  if (temtOk) display.print(lastAdc);
  else display.print("---");

  display.print(" L:");
  if (ldrOk) {
    display.print(ldrAdc);
    display.print("(");
    display.print((ldrAdc * 100) / 4095);  // Porcentaje relativo
    display.print("%)");
  } else {
    display.print("---");
  }

  // --- Dibujar Gráfica (TEMT6000) ---
  const int graphMiddle = adcToY(GRAPH_ADC_MIDDLE);
  display.setCursor(0, GRAPH_TOP);
  display.print(GRAPH_ADC_MAX);
  display.setCursor(0, GRAPH_BOTTOM - 7);
  display.print(GRAPH_ADC_MIN);

  // Ejes
  display.drawFastVLine(GRAPH_LEFT - 2, GRAPH_TOP, GRAPH_BOTTOM - GRAPH_TOP + 1, SSD1306_WHITE);
  display.drawFastHLine(GRAPH_LEFT, GRAPH_TOP, GRAPH_WIDTH, SSD1306_WHITE);

  // Línea punteada central
  for (int x = GRAPH_LEFT; x < OLED_WIDTH; x += 4) {
    display.drawPixel(x, graphMiddle, SSD1306_WHITE);
  }

  // Trazar histórico
  for (int x = 1; x < GRAPH_WIDTH; ++x) {
    display.drawLine(
      GRAPH_LEFT + x - 1, adcToY(samples[x - 1]),
      GRAPH_LEFT + x, adcToY(samples[x]),
      SSD1306_WHITE);
  }

  display.display();
}

void setup() {
  Serial.begin(115200);
  delay(2000);  // Tiempo de estabilización eléctrica

  // Recuperación global del bus I2C
  if (!bus.beginRecovered(I2C_SDA, I2C_SCL, 20000, false)) {
    Serial.println("Error Crítico: El bus I2C está bloqueado físicamente.");
  }
  Wire.setClock(I2C_FREQ);

  // 1. Inicializar OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("Error: No se encontró OLED SSD1306");
  } else {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print("Iniciando red I2C...");
    display.display();
  }
  delay(200);

  // 2. Inicializar Botón
  if (!button.beginRecovered(I2C_SDA, I2C_SCL)) {
    Serial.println("Button module not found; check wiring, power and firmware");
  }
  delay(200);
  if (!button.isConnected()) {
    Serial.println("Error: Botón I2C no detectado");
  }
  delay(200);

  // 3. Inicializar TEMT6000
  DevLabDDP::DeviceInfo info;
  temtVerified = master.matchesExpectedDevice(TEMT6000_ADDR, &info);
  if (!temtVerified) {
    Serial.println("Error: TEMT6000 no detectado en 0x20");
  }
  delay(200);

  // 4. Inicializar LDR GT36537
  ldrSensor.beginRecovered(I2C_SDA, I2C_SCL);
  if (!ldrSensor.busReady()) {
    Serial.println("ERROR: I2C bus is blocked");
  }
  if (!ldrSensor.isConnected()) {
    Serial.println("Error: LDR GT36537 no detectado en 0x26");
  }
  delay(200);

  // 5. Inicializar ICP101xx
  if (!icp_sensor.begin(&Wire)) {
    Serial.println("Error: ICP10111 no detectado");
  }
}

void loop() {
  uint32_t now = millis();

  // TAREA 1: Actualizar estado del botón (10 ms)
  if (now - lastBtnTime >= 10) {
    lastBtnTime = now;
    if (button.isConnected() && button.update()) {
      if (button.wasPressed()) {
        pressCount++;
        isPressed = true;
      }
      if (button.wasReleased()) {
        isPressed = false;
      }
    }
  }

  // TAREA 2: Leer LDR GT36537 (250 ms)
  if (now - lastLdrTime >= 250) {
    lastLdrTime = now;
    if (ldrSensor.isConnected()) {
      ldrOk = ldrSensor.readRaw(ldrAdc);
    } else {
      ldrOk = false;
    }
  }

  // TAREA 3: Leer ADC del TEMT6000 y actualizar OLED (50 ms)
  if (now - lastTemtTime >= 50) {
    lastTemtTime = now;
    uint16_t adcVal = 0;
    temtOk = readADC0(adcVal);

    if (temtOk) {
      lastAdc = adcVal;
      addSample(lastAdc);
    }

    // La pantalla se refresca aquí aprovechando los 50ms de actualización
    updateDisplay();
  }

  // TAREA 4: Leer Presión y Temperatura (1000 ms)
  if (now - lastIcpTime >= 1000) {
    lastIcpTime = now;
    icp_sensor.measure(icp_sensor.NORMAL);
    currentPressure = icp_sensor.getPressurePa();
    currentTemp = icp_sensor.getTemperatureC();
  }
}