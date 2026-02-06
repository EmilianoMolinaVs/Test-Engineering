/*
=== MainCode JSON UE0109 BMA255 ===
Código de integración para BMA255 con lectura de I2C y SPI 
*/



#include <Wire.h>
#include "BMA250.h"

// I2C pins for ESP32-C6 (change as needed)
#define SDA_PIN 22
#define SCL_PIN 23

// Accelerometer sensor variables
BMA250 accel_sensor;
int x, y, z;
double temp;

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 5000) delay(10);
  
  Serial.println("=== BMA250 I2C Local Library Example ===");
  
  // Initialize I2C with custom pins
  Wire.begin(SDA_PIN, SCL_PIN);
  Serial.print("I2C initialized with SDA=");
  Serial.print(SDA_PIN);
  Serial.print(", SCL=");
  Serial.println(SCL_PIN);
  
  // Initialize BMA250 with I2C (using working parameters)
  Serial.print("Initializing BMA250 via I2C...");
  int result = accel_sensor.begin(BMA250_range_2g, BMA250_update_time_64ms);
  
  if (result == 0) {
    Serial.println(" SUCCESS!");
    Serial.print("I2C Address: 0x");
    Serial.println(accel_sensor.I2Caddress, HEX);
  } else {
    Serial.println(" FAILED!");
    Serial.println("Scanning I2C bus...");
    
    // Scan for I2C devices
    int deviceCount = 0;
    for (byte addr = 1; addr < 127; addr++) {
      Wire.beginTransmission(addr);
      if (Wire.endTransmission() == 0) {
        Serial.print("Device found at 0x");
        if (addr < 16) Serial.print("0");
        Serial.println(addr, HEX);
        deviceCount++;
      }
    }
    
    if (deviceCount == 0) {
      Serial.println("No I2C devices found!");
      Serial.println("Check connections:");
      Serial.print("- SDA → GPIO"); Serial.println(SDA_PIN);
      Serial.print("- SCL → GPIO"); Serial.println(SCL_PIN);
    }
  }
  
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
    Serial.println("ERROR! NO BMA250 DETECTED via I2C!");
    Serial.println("Check I2C connections:");
    Serial.print("- SDA → GPIO"); Serial.println(SDA_PIN);
    Serial.print("- SCL → GPIO"); Serial.println(SCL_PIN);
    Serial.println("- SDO → GND (addr 0x18) or VCC (addr 0x19)");
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