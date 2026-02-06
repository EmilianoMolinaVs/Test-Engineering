/*************************************************************************
 * BMA250 SPI Example:
 * This example demonstrates how to use the BMA250 accelerometer with SPI
 * communication instead of I2C on ESP32-C6
 * 
 * Hardware by: TinyCircuits
 * SPI Implementation by: Assistant
 * 
 * SPI Connections for ESP32-C6:
 * VCC  → 3.3V
 * GND  → GND
 * SCK  → GPIO6 (SPI Clock)
 * MOSI → GPIO7 (Master Out Slave In)
 * MISO → GPIO2 (Master In Slave Out)
 * CS   → GPIO18 (Chip Select)
 * 
 * Created: Sep. 5/2025
 ************************************************************************/

#include <SPI.h>
#include "BMA250.h"

// SPI pin definitions for ESP32-C6
#define CS_PIN 18
#define MOSI_PIN 7
#define MISO_PIN 2
#define SCK_PIN 6

// Accelerometer sensor variables
BMA250 accel_sensor;
int x, y, z;
double temp;

void setup() {
  // Initialize USB CDC Serial
  Serial.begin(115200);
  
  // Wait for USB CDC connection
  while (!Serial && millis() < 5000) {
    delay(10);
  }
  
  Serial.println("\n=== BMA250 SPI Accelerometer Test ===");
  Serial.println("Initializing SPI communication...");
  
  // Configure SPI pins for ESP32-C6
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);
  
  Serial.println("SPI Pin Configuration:");
  Serial.print("SCK:  GPIO");
  Serial.println(SCK_PIN);
  Serial.print("MISO: GPIO");
  Serial.println(MISO_PIN);
  Serial.print("MOSI: GPIO");
  Serial.println(MOSI_PIN);
  Serial.print("CS:   GPIO");
  Serial.println(CS_PIN);
  
  // Initialize BMA250 with SPI
  Serial.print("Initializing BMA250 via SPI...");
  int result = accel_sensor.beginSPI(BMA250_range_2g, BMA250_update_time_64ms, CS_PIN, &SPI);
  
  if (result == 0) {
    Serial.println(" SUCCESS!");
  } else {
    Serial.println(" FAILED!");
    Serial.println("Check SPI connections and try again.");
  }
  
  Serial.println("USB CDC Serial enabled");
  Serial.println("Setup complete!");
  Serial.println("=====================================");
}

void loop() {
  // Read new data from the accelerometer
  accel_sensor.read();

  // Get the acceleration values from the sensor
  x = accel_sensor.X;
  y = accel_sensor.Y;
  z = accel_sensor.Z;
  temp = ((accel_sensor.rawTemp * 0.5) + 24.0);

  // Check if the BMA250 is not found or connected correctly
  if (x == -1 && y == -1 && z == -1) {
    Serial.println("ERROR! NO BMA250 DETECTED via SPI!");
    Serial.println("Check SPI connections:");
    Serial.println("- VCC  → 3.3V");
    Serial.println("- GND  → GND");
    Serial.println("- SCK  → GPIO6");
    Serial.println("- MISO → GPIO2");
    Serial.println("- MOSI → GPIO7");
    Serial.println("- CS   → GPIO18");
  } else {
    // Print sensor readings
    showSerial();
  }

  // Delay to ensure proper sensor reading timing
  delay(250);
}

// Prints the sensor values to the Serial Monitor
void showSerial() {
  Serial.print("X = ");
  Serial.print(x);
  
  Serial.print("  Y = ");
  Serial.print(y);
  
  Serial.print("  Z = ");
  Serial.print(z);
  
  Serial.print("  Temperature(C) = ");
  Serial.println(temp);
}