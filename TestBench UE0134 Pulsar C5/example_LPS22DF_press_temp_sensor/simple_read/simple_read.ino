#include <Wire.h>

// Asignación de hardware para tu diseño con ESP32-C5
#define SDA_PIN 2
#define SCL_PIN 3

// La dirección I2C es 0x5D por defecto (SA0 a VDD_IO). Si enviaste SA0 a GND, usa 0x5C.
#define LPS22DF_ADDR  0x5C

// Mapa de registros esenciales
#define WHO_AM_I      0x0F // Valor esperado: 0xB4
#define CTRL_REG1     0x10 // Control de encendido y tasa de datos (ODR)
#define PRESS_OUT_XL  0x28 // Inicio de datos de presión (24 bits)

void setup() {
  Serial.begin(115200);
  
  // Inicializa el bus I2C en los pines requeridos
  Wire.begin(SDA_PIN, SCL_PIN);
  
  // 1. Verificar comunicación con el sensor
  Wire.beginTransmission(LPS22DF_ADDR);
  Wire.write(WHO_AM_I);
  Wire.endTransmission(false); // Repeated start
  Wire.requestFrom(LPS22DF_ADDR, 1);
  
  if (Wire.available()) {
    uint8_t id = Wire.read();
    Serial.printf("WHO_AM_I: 0x%02X\n", id); // Debería imprimir 0xB4
  } else {
    Serial.println("Error: Sensor no encontrado.");
  }
  
  // 2. Configurar el sensor (Encendido continuo)
  // ODR = 25Hz (0b0100 << 4) = 0x40. Promediado AVG = 4 (bits inferiores por defecto)
  Wire.beginTransmission(LPS22DF_ADDR);
  Wire.write(CTRL_REG1);
  Wire.write(0x40);
  Wire.endTransmission();
}

void loop() {
  // Lectura en ráfaga de 5 bytes (3 bytes de presión + 2 bytes de temperatura)
  Wire.beginTransmission(LPS22DF_ADDR);
  Wire.write(PRESS_OUT_XL);
  Wire.endTransmission(false);
  Wire.requestFrom(LPS22DF_ADDR, 5);
  
  if (Wire.available() == 5) {
    uint8_t px = Wire.read(); // PRESS_OUT_XL
    uint8_t pl = Wire.read(); // PRESS_OUT_L
    uint8_t ph = Wire.read(); // PRESS_OUT_H
    uint8_t tl = Wire.read(); // TEMP_OUT_L
    uint8_t th = Wire.read(); // TEMP_OUT_H
    
    // Ensamblar la presión (24 bits)
    int32_t press_raw = (int32_t)(ph << 16 | pl << 8 | px);
    
    // Extensión de signo manual de 24 a 32 bits por el complemento a 2
    if (press_raw & 0x00800000) { 
      press_raw |= 0xFF000000;
    }
    
    // Ensamblar la temperatura (16 bits)
    int16_t temp_raw = (int16_t)(th << 8 | tl);
    
    // Aplicar sensibilidad física de la hoja de datos (4096 LSB/hPa, 100 LSB/°C)
    float presion_hPa = press_raw / 4096.0f;
    float temperatura_C = temp_raw / 100.0f;
    
    Serial.printf("Presion: %.2f hPa | Temp: %.2f °C\n", presion_hPa, temperatura_C);
  }
  
  delay(1000); 
}