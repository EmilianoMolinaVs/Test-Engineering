// Firmware Dedicado: HDMI + IMU (BMI270) - Movimiento de Minion
// Arduino Nano RP2350

#include <Wire.h>
#include <udvi_hstx.h>
#include "SparkFun_BMI270_Arduino_Library.h"
#include "minion_bitmap.h" // Archivo con el array de la imagen del minion

// ============ CONFIGURACIÓN HDMI ============
DVHSTXPinout pinConfig = { 14, 18, 16, 12 };
DVHSTX16 display(pinConfig, DVHSTX_RESOLUTION_320x240);

// ============ CONFIGURACIÓN BMI270 ============
BMI270 imu;
uint8_t i2cAddress = BMI2_I2C_PRIM_ADDR + 1;  // 0x69

// ============ VARIABLES DEL MINION ============
// Posición inicial (centro de la pantalla)
float minionX = 160.0;
float minionY = 120.0;

// Variables para borrar el frame anterior y evitar parpadeos
int oldX = 160;
int oldY = 120;

// Velocidad actual del minion
float velX = 10;
float velY = 10;

// Tamaño de la imagen (debe coincidir con tu array exportado)
const int MINION_WIDTH = 64;  
const int MINION_HEIGHT = 64;

// Tiempos para control de FPS
unsigned long lastFrameTime = 0;
const int targetFPS = 60;
const int frameDelay = 1000 / targetFPS;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n========================================");
  Serial.println("Iniciando HDMI + IMU (Minion Interactable)");
  Serial.println("========================================\n");

  // ============ PASO 1: Inicializar HDMI ============
  Serial.print("1. Inicializando Display HDMI... ");
  if (!display.begin()) {
    Serial.println("ERROR!");
    while (true) {
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      delay(100);
    }
  }
  Serial.println("OK (320x240)");
  display.fillScreen(0x0000); // Fondo negro

  // ============ PASO 2: Inicializar I2C y BMI270 ============
  Serial.print("2. Inicializando I2C (SDA=8, SCL=9)... ");
  Wire.setSDA(8);
  Wire.setSCL(9);
  Wire.begin();
  delay(100);
  Serial.println("OK");

  Serial.print("3. Inicializando BMI270... ");
  if (imu.beginI2C(i2cAddress) != BMI2_OK) {
    Serial.println("ERROR: IMU NO DETECTADO");
    display.setCursor(10, 10);
    display.setTextColor(0xF800);
    display.print("ERROR: IMU NO DETECTADO");
    while(1); // Detener si no hay IMU
  }
  Serial.println("OK");

  // Ajustar posición inicial para que el Minion nazca en el centro exacto
  minionX = (display.width() - MINION_WIDTH) / 2.0;
  minionY = (display.height() - MINION_HEIGHT) / 2.0;
}

void loop() {
  if (millis() - lastFrameTime >= frameDelay) {
    lastFrameTime = millis();

    // 1. Leer los datos del acelerómetro
    imu.getSensorData();

    // 2. Aplicar "físicas" (inclinación -> aceleración)
    // Dependiendo de la orientación física de tu placa, es posible que 
    // debas invertir los signos (+ por -) o cambiar accelX por accelY.
    float accelX = imu.data.accelX; 
    float accelY = imu.data.accelY; 

    velX += accelX * 0.35; // Sensibilidad X
    velY += accelY * 0.35; // Sensibilidad Y

    // Aplicar fricción (para que no se mueva infinitamente y se detenga)
    velX *= 0.92;
    velY *= 0.92;

    // Actualizar posición
    minionX += velX;
    minionY += velY;

    // 3. Colisiones con los bordes (Rebote)
    if (minionX < 0) { 
      minionX = 0; 
      velX = -velX * 1.6; // Rebote perdiendo energía
    }
    else if (minionX > display.width() - MINION_WIDTH) { 
      minionX = display.width() - MINION_WIDTH; 
      velX = -velX * 0.6; 
    }

    if (minionY < 0) { 
      minionY = 0; 
      velY = -velY * 0.6; 
    }
    else if (minionY > display.height() - MINION_HEIGHT) { 
      minionY = display.height() - MINION_HEIGHT; 
      velY = -velY * 0.6; 
    }

    // 4. Dibujar en el HDMI
    int currentX = (int)minionX;
    int currentY = (int)minionY;

    // Si el Minion se movió, borramos SOLO su posición anterior (Evita parpadeos)
    if (currentX != oldX || currentY != oldY) {
      display.fillRect(oldX, oldY, MINION_WIDTH, MINION_HEIGHT, 0x0000); 
    }

    // Dibujar la imagen del minion en la nueva posición
    // (Asegúrate de que la función de tu librería coincida, usualmente drawRGBBitmap)
    display.drawRGBBitmap(currentX, currentY, epd_bitmap_minion, MINION_WIDTH, MINION_HEIGHT);

    // Guardar posición para el siguiente frame
    oldX = currentX;
    oldY = currentY;
  }
}