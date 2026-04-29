#include <Wire.h>

// --- Mapa de Registros del VEML3328 ---
#define VEML3328_ADDR 0x10
#define VEML3328_REG_CONF 0x00   // Registro para configurar y encender
#define VEML3328_REG_CLEAR 0x04  // Canal de luz blanca/clara
#define VEML3328_REG_RED 0x05    // Canal Rojo
#define VEML3328_REG_GREEN 0x06  // Canal Verde
#define VEML3328_REG_BLUE 0x07   // Canal Azul
#define VEML3328_REG_IR 0x08     // Canal Infrarrojo
#define VEML3328_REG_ID 0x0C     // Registro de Identificador

// --- Pines físicos de tu Pulsar C6 ---
#define SDA_PIN 22
#define SCL_PIN 23

// Función para escribir 16 bits en un registro
void writeRegister(uint8_t reg, uint16_t value) {
  Wire.beginTransmission(VEML3328_ADDR);
  Wire.write(reg);                  // Apunta al registro deseado
  Wire.write(value & 0xFF);         // Envía el byte bajo (LSB)
  Wire.write((value >> 8) & 0xFF);  // Envía el byte alto (MSB)
  Wire.endTransmission();
}

// Función para leer 16 bits de un registro
uint16_t readRegister(uint8_t reg) {
  Wire.beginTransmission(VEML3328_ADDR);
  Wire.write(reg);
  Wire.endTransmission();  

  Wire.requestFrom((uint8_t)VEML3328_ADDR, (uint8_t)2);

  if (Wire.available() == 2) {
    uint8_t lsb = Wire.read();
    uint8_t msb = Wire.read();
    return (msb << 8) | lsb;
  }
  return 0;  // Si falla la lectura, devolverá 0
}

void setup() {
  Serial.begin(115200);

  // Inicializamos el bus I2C
  Wire.begin(SDA_PIN, SCL_PIN);
  delay(1000);

  Serial.println("Iniciando VEML3328 (Modo Directo)...");

  // Encendemos el sensor (SD0 = 0)
  writeRegister(VEML3328_REG_CONF, 0x0000);

  // Guardamos el ID en una variable de 16 bits
  uint16_t id_completo = readRegister(VEML3328_REG_ID);

  Serial.print("ID Completo devuelto por el sensor: 0x");
  Serial.println(id_completo, HEX);  // Debería imprimir 828

  // Aplicamos una máscara (& 0xFF) para ignorar el byte alto y ver solo el ID real
  Serial.print("Device ID (Byte bajo): 0x");
  Serial.println(id_completo & 0xFF, HEX);  // Debería imprimir 28

  // Le damos un pequeño tiempo al sensor para estabilizarse
  delay(150);

  Serial.println("Sensor configurado y listo.");
  Serial.println("---------------------------------");
}

void loop() {
  // Tomamos la lectura de cada canal
  uint16_t clear = readRegister(VEML3328_REG_CLEAR);
  uint16_t red = readRegister(VEML3328_REG_RED);
  uint16_t green = readRegister(VEML3328_REG_GREEN);
  uint16_t blue = readRegister(VEML3328_REG_BLUE);
  uint16_t ir = readRegister(VEML3328_REG_IR);

  // Imprimimos el resultado de forma bonita
  Serial.printf("C: %5u | R: %5u | G: %5u | B: %5u | IR: %5u\n", clear, red, green, blue, ir);

  // Esperamos 1 segundo antes de la próxima lectura
  delay(1000);
}