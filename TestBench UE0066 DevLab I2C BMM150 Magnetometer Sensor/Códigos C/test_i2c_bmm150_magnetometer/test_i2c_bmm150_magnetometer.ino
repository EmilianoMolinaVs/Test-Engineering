#include <Wire.h>
#include "DFRobot_BMM150.h"

// Dirección I2C por defecto del BMM150 (depende del cableado de CS y SDO)
//DFRobot_BMM150_I2C bmm150(&Wire, I2C_ADDRESS_4);
DFRobot_BMM150_I2C *bmm150 = nullptr;

uint8_t detectBMM150Address() {
  uint8_t addresses[] = {0x10, 0x11, 0x12, 0x13};

  for (uint8_t i = 0; i < 4; i++) {
    Wire.beginTransmission(addresses[i]);
    if (Wire.endTransmission() == 0) {
      return addresses[i]; // dirección encontrada
    }
  }
  return 0; // no encontrado
}


void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(6, 7);

  uint8_t addr = detectBMM150Address();

  if (addr == 0) {
    Serial.println("❌ BMM150 no detectado en ninguna direccion I2C");
    while (1);
  }

  Serial.print("✅ BMM150 detectado en 0x");
  Serial.println(addr, HEX);

  // Crear el objeto con la dirección detectada
  bmm150 = new DFRobot_BMM150_I2C(&Wire, addr);

  while (bmm150->begin()) {
    Serial.println("BMM150 init failed, retrying...");
    delay(1000);
  }

  Serial.println("BMM150 init success!");

  bmm150->setOperationMode(BMM150_POWERMODE_NORMAL);
  bmm150->setPresetMode(BMM150_PRESETMODE_HIGHACCURACY);
  bmm150->setRate(BMM150_DATA_RATE_10HZ);
  bmm150->setMeasurementXYZ();
}


void loop() {
  sBmm150MagData_t magData = bmm150->getGeomagneticData();

  Serial.print("X: "); Serial.print(magData.x); Serial.print(" uT | ");
  Serial.print("Y: "); Serial.print(magData.y); Serial.print(" uT | ");
  Serial.print("Z: "); Serial.print(magData.z); Serial.println(" uT");

  Serial.print("Compass: ");
  Serial.print(bmm150->getCompassDegree());
  Serial.println(" deg");
  Serial.println("-------------------------");

  delay(500);
}
