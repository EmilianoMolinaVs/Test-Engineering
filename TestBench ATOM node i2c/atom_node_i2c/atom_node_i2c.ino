/**
 * @file I2C_Network_Monitor.ino
 * @brief Monitor continuo de múltiples sensores en I2C mostrado en SSD1306 OLED
 * Hardware: ESP32-C6 (SDA = GPIO 6, SCL = GPIO 7)
 */

#include <Arduino.h>
#include <Wire.h>
#include <DevLab_PhotoInterrupter.h>
#include <DevLabDDP.h>
#include <DevLab_I2C_Orchestrator.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "ue_i2c_icp_10111_sen.h"

// --- Pines y Configuración I2C (ESP32-C6) ---
constexpr uint8_t I2C_SDA = 6;
constexpr uint8_t I2C_SCL = 7;
constexpr uint32_t I2C_FREQ = 50000;  // 100kHz para compatibilidad total con la red

// --- Direcciones I2C ---
constexpr uint8_t TEMT6000_ADDR = 0x20;
constexpr uint8_t ITR523S_ADDR = 0x32;
constexpr uint8_t OLED_ADDRESS = 0x3C;

// --- Configuración OLED y Gráfica ---
constexpr int OLED_WIDTH = 128;
constexpr int OLED_HEIGHT = 64;
constexpr int GRAPH_LEFT = 27;
constexpr int GRAPH_WIDTH = OLED_WIDTH - GRAPH_LEFT;
// Reducimos la gráfica a la mitad inferior para dar espacio al texto
constexpr int GRAPH_TOP = 32;
constexpr int GRAPH_BOTTOM = OLED_HEIGHT - 1;
constexpr uint16_t GRAPH_ADC_MIN = 0;  // Ajustado para ver el rango desde cero
constexpr uint16_t GRAPH_ADC_MAX = 4095;
constexpr uint16_t GRAPH_ADC_MIDDLE = GRAPH_ADC_MIN + ((GRAPH_ADC_MAX - GRAPH_ADC_MIN) / 2);

// --- Instancias de Hardware ---
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
DevLab_I2C_Orchestrator bus(Wire, I2C_FREQ);
DevLabDDP::Master master(bus, DevLabDDP::DEVICE_TEMT6000);
DevLab_PhotoInterrupter itr(Wire, ITR523S_ADDR, I2C_FREQ);
ICP101xx icp_sensor;

// --- Variables de Estado ---
bool temtVerified = false;
bool temtOk = false;

// Datos de sensores
uint16_t samples[GRAPH_WIDTH] = { 0 };
uint16_t lastAdc = 0;
bool isBlocked = false;
uint16_t itrCount = 0;
float currentPressure = 0.0;
float currentTemp = 0.0;

// Temporizadores no bloqueantes
uint32_t lastTemtTime = 0;
uint32_t lastIcpTime = 0;
uint32_t lastItrTime = 0;

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

  // Fila 1: Sensor ITR523S
  display.setCursor(0, 0);
  display.print("ITR: ");
  display.print(isBlocked ? "BLOCK" : "FREE ");
  display.print(" Cnt:");
  display.print(itrCount);

  // Fila 2: Presión y Temperatura (ICP10111)
  display.setCursor(0, 10);
  display.print("T:");
  display.print(currentTemp, 1);
  display.print("C P:");
  display.print(currentPressure, 0);  // Omitimos decimales por espacio

  // Fila 3: Valor actual del ADC (TEMT6000)
  display.setCursor(0, 20);
  if (temtOk) {
    display.print("Luz: ");
    display.print(lastAdc);
    display.print(" (");
    display.print((lastAdc * 3.3f) / 4095.0f, 1);
    display.print("V)");
  } else {
    display.print("Luz: Sin respuesta");
  }

  // --- Dibujar Gráfica ---
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

  // 1. Tiempo de estabilización eléctrica para que los módulos enciendan
  delay(2000);

  // 2. Recuperación del bus (Desbloquea esclavos atascados en SDA LOW)
  if (!bus.beginRecovered(I2C_SDA, I2C_SCL, 20000, false)) {
    Serial.println("Error Crítico: El bus I2C está bloqueado físicamente.");
  }

  // Forzar el reloj global para la red
  Wire.setClock(I2C_FREQ);

  // 3. Inicializar OLED primero
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("Error: No se encontró OLED SSD1306");
  } else {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print("Inicializando I2C...");
    display.display();
  }

  delay(500);  // Pausa entre módulos

  // 4. Inicializar ITR523S con verificación
  for (int i = 0; i < 5; i++) {
    if (!itr.isConnected()) {
      Serial.println("Error: ITR523S no detectado en 0x32");
    }
    delay(500);
  }

  delay(200);

  // 5. Inicializar TEMT6000
  DevLabDDP::DeviceInfo info;
  temtVerified = master.matchesExpectedDevice(TEMT6000_ADDR, &info);
  if (!temtVerified) {
    Serial.println("Error: TEMT6000 no detectado en 0x20");
  }

  delay(200);

  // 6. Inicializar ICP101xx
  if (!icp_sensor.begin(&Wire)) {
    Serial.println("Error: ICP10111 no detectado");
  }
}
void loop() {
  uint32_t now = millis();

  // TAREA 1: Leer el Interrumptor Óptico (cada 10 ms para respuesta rápida)
  if (now - lastItrTime >= 10) {
    lastItrTime = now;
    if (itr.isConnected()) {
      itr.readBlocked(isBlocked);
      itr.readCount(itrCount);
    }
  }

  // TAREA 2: Leer ADC del TEMT6000 y actualizar OLED (cada 50 ms para fluidez)
  if (now - lastTemtTime >= 50) {
    lastTemtTime = now;
    uint16_t adcVal = 0;
    temtOk = readADC0(adcVal);

    if (temtOk) {
      lastAdc = adcVal;
      addSample(lastAdc);
    }

    // Actualizamos la pantalla al mismo ritmo de la gráfica
    updateDisplay();
  }

  // TAREA 3: Leer Presión y Temperatura (cada 1000 ms, cambios más lentos)
  if (now - lastIcpTime >= 1000) {
    lastIcpTime = now;
    // Ejecutar medición no bloqueante (modo NORMAL)
    icp_sensor.measure(icp_sensor.NORMAL);
    currentPressure = icp_sensor.getPressurePa();
    currentTemp = icp_sensor.getTemperatureC();
  }
}