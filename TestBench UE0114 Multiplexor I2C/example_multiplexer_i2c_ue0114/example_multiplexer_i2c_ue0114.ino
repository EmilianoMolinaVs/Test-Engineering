// --------------------------------------
// i2c_scanner
//TCA9545A
/* 
 *  16/AGO/2021
 */

#include <Wire.h>

#define ADD 0x72  // Direccion del Multiplexor

void setup() {
  Wire.begin(6, 7);

  Serial.begin(115200);
  Serial.println("\nI2C Scanner");
}


void tcaselect(uint8_t i) {
  if (i > 7) return;

  Wire.beginTransmission(ADD);
  Wire.write(1 << i);
  Wire.endTransmission();
}


void loop() {
  int nDevices = 0;

  Serial.println("Scanning...");
  for (byte t = 0; t < 4; t++) {
    tcaselect(t);
    Serial.print("TCA Port #");
    Serial.println(t);

    for (byte address = 1; address < 127; ++address) {
      // The i2c_scanner uses the return value of
      // the Write.endTransmisstion to see if
      // a device did acknowledge to the address.
      if (address == ADD) continue;
      Wire.beginTransmission(address);
      byte error = Wire.endTransmission();

      if (error == 0) {
        Serial.print("I2C device found at address 0x");
        if (address < 16) {
          Serial.print("0");
        }
        Serial.print(address, HEX);
        Serial.println("  !");

        ++nDevices;
      } else if (error == 4) {
        Serial.print("Unknown error at address 0x");
        if (address < 16) {
          Serial.print("0");
        }
        Serial.println(address, HEX);
      }
    }
  }

  Serial.println("done\n");

  delay(500);  // Wait 5 seconds for next scan
}