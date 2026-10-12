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
float minionX = 0;
float minionY = 0;
int oldX = 0;
int oldY = 0;

// Velocidad inicial en 0 para que nazca estático
float velX = 0;
float velY = 0;

// Tamaño de la imagen
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
  if (!display.begin()) {
    Serial.println("ERROR HDMI!");
    while (true) {
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      delay(100);
    }
  }
  display.fillScreen(0x0000);

  // ============ PASO 2: Inicializar I2C y BMI270 ============
  Wire.setSDA(8);
  Wire.setSCL(9);
  Wire.begin();
  delay(100);

  if (imu.beginI2C(i2cAddress) != BMI2_OK) {
    display.setCursor(10, 10);
    display.setTextColor(0xF800);
    display.print("ERROR: IMU NO DETECTADO");
    while(1); 
  }

  // Centrar al Minion exactamente según la resolución real
  minionX = (display.width() - MINION_WIDTH) / 2.0;
  minionY = (display.height() - MINION_HEIGHT) / 2.0;
  
  // SINCRONIZAR las variables antiguas para evitar el salto del primer fotograma
  oldX = (int)minionX;
  oldY = (int)minionY;
}

void loop() {
  if (millis() - lastFrameTime >= frameDelay) {
    lastFrameTime = millis();

    // 1. Leer acelerómetro
    imu.getSensorData();
    float accelX = imu.data.accelX; 
    float accelY = imu.data.accelY; 

    // 2. Físicas Fluidas
    // Sensibilidad: 0.20 responde rápido pero sin ser incontrolable
    velX += accelX * 1.50; 
    velY += accelY * 1.50; 

    // Fricción: 0.94 da una sensación de "deslizamiento sobre hielo"
    velX *= 0.6;
    velY *= 0.6;

    minionX += velX;
    minionY += velY;

    // 3. Colisiones unificadas (Rebote realista)
    const float rebote = 1.2; 

    if (minionX < 0) { 
      minionX = 0; 
      velX = -velX * rebote; 
    }
    else if (minionX > display.width() - MINION_WIDTH) { 
      minionX = display.width() - MINION_WIDTH; 
      velX = -velX * rebote; 
    }

    if (minionY < 0) { 
      minionY = 0; 
      velY = -velY * rebote; 
    }
    else if (minionY > display.height() - MINION_HEIGHT) { 
      minionY = display.height() - MINION_HEIGHT; 
      velY = -velY * rebote; 
    }

    // 4. Dibujar
    int currentX = (int)minionX;
    int currentY = (int)minionY;

    if (currentX != oldX || currentY != oldY) {
      display.fillRect(oldX, oldY, MINION_WIDTH, MINION_HEIGHT, 0x0000); 
    }

    display.drawRGBBitmap(currentX, currentY, epd_bitmap_minion, MINION_WIDTH, MINION_HEIGHT);

    oldX = currentX;
    oldY = currentY;
  }
}