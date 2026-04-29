#include <Arduino.h>
#include <Wire.h>
#include "veml3328.h"

// Pines PulsarC6 (Confirma si son estos en tu hardware, usualmente SDA:6, SCL:7)
#define I2C_SDA 6
#define I2C_SCL 7

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n--- VEML3328 Debugging ---");

  Wire.begin(I2C_SDA, I2C_SCL);
  // Bajamos un poco la velocidad por estabilidad si hay cables largos
  Wire.setClock(100000);

  // 1. Inicializar sensor
  if (Veml3328.begin(&Wire) != 0) {
    Serial.println("ERROR: No se pudo iniciar el sensor (Falla en el Wake).");
  }

  // 2. Leer Device ID (Debe ser 0x28 para el VEML3328)
  uint16_t id = Veml3328.deviceID();
  Serial.print("Device ID detectado: 0x");
  Serial.println(id, HEX);

  if (id != 0x28) {
    Serial.println("ADVERTENCIA: El ID no coincide con un VEML3328 original (esperado 0x28).");
  }

  // 3. Configuración Forzada para evitar Ceros
  Serial.println("Configurando canales...");

  Veml3328.wake();      // Despierta el chip completo
  Veml3328.rbWakeup();  // Asegura que canales Rojo y Azul estén encendidos

  // Aumentamos la ganancia y el tiempo para que no de 0 en interiores
  Veml3328.setGain(gain_x2);
  Veml3328.setIntTime(time_400);  // 400ms de integración (más sensibilidad)

  Serial.println("Sistema listo.\n");
}

void loop() {
  // Verificamos si el sensor sigue respondiendo leyendo el ID de nuevo
  // Si esto da 0 o FFFF, el bus I2C se colgó
  uint16_t checkID = Veml3328.deviceID();

  if (checkID == 0 || checkID == 0xFFFF) {
    Serial.println("¡ERROR! Conexión I2C perdida. Reintentando bus...");
    Wire.begin(I2C_SDA, I2C_SCL);
    delay(1000);
    return;
  }

  uint16_t r = Veml3328.getRed();
  uint16_t g = Veml3328.getGreen();
  uint16_t b = Veml3328.getBlue();
  uint16_t ir = Veml3328.getIR();
  uint16_t c = Veml3328.getClear();

  Serial.printf("ID:0x%02X | R:%d G:%d B:%d | IR:%d Clear:%d\n", checkID, r, g, b, ir, c);

  delay(100);
}