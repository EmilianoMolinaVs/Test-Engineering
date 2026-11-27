#include <Wire.h>

#define SDA_PIN 6
#define SCL_PIN 7
#define PIN_SDO D1
#define PIN_CS D0
#define I2C_FREQ 400000  // 400 kHz (Fast Mode)

void setup() {
  Serial.begin(115200);
  delay(100);

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(I2C_FREQ);
  Serial.println("I2C scan starting...");

  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("I2C device found at 0x");
      if (addr < 16) Serial.print("0");
      Serial.println(addr, HEX);
    }
  }

  Serial.println("I2C first scan done");
}

void loop() {

  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("I2C device found at 0x");
      if (addr < 16) Serial.print("0");
      Serial.println(addr, HEX);
    }
  }

  Serial.println("====================");
  delay(2000);
}
