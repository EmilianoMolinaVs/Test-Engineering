// Firmware Dedicado: HDMI + IMU (Giroscopio) - Laberinto Final (Bandera Carreras y Victoria)
// Arduino Nano RP2350

#include <Wire.h>
#include <udvi_hstx.h>
#include "SparkFun_BMI270_Arduino_Library.h"
#include "minion_bitmap.h"

// ============ CONFIGURACIÓN HDMI ============
DVHSTXPinout pinConfig = { 14, 18, 16, 12 };
DVHSTX16 display(pinConfig, DVHSTX_RESOLUTION_320x240);

// ============ CONFIGURACIÓN BMI270 ============
BMI270 imu;
uint8_t i2cAddress = BMI2_I2C_PRIM_ADDR + 1;

// ============ VARIABLES DEL MINION Y JUEGO ============
float minionX = 5;
float minionY = 5;
int oldX = 5;
int oldY = 5;

float velX = 0;
float velY = 0;

// Ángulos virtuales calculados desde el giroscopio
float virtualAngleX = 0;
float virtualAngleY = 0;

// Sistema de vidas (Bananas)
int bananas = 10;
unsigned long lastDamageTime = 0;
const unsigned long DAMAGE_COOLDOWN = 1000; // 1 segundo de inmunidad al chocar

const int MINION_WIDTH = 64;  
const int MINION_HEIGHT = 64;

unsigned long lastFrameTime = 0;
const int targetFPS = 60;
const int frameDelay = 1000 / targetFPS;

// ============ DEFINICIÓN DEL LABERINTO ============
struct Rect {
  int x, y, w, h;
};

// Laberinto en zig-zag
const int NUM_WALLS = 3;
Rect walls[NUM_WALLS] = {
  {80, 0, 10, 160},    // Pared 1: Baja desde el techo
  {160, 80, 10, 160},  // Pared 2: Sube desde el suelo
  {240, 0, 10, 160}    // Pared 3: Baja desde el techo
};

// Zona de Meta
Rect goal = {270, 10, 40, 40};

// ============ FUNCIONES DE DIBUJO (TEXTURAS, META Y VICTORIA) ============

// Función para dibujar una pared azul con patrón pixelado
void drawWall(int x, int y, int w, int h) {
  display.fillRect(x, y, w, h, 0x000A); // Fondo Azul Oscuro
  
  // Patrón de "píxeles" (Tablero de ajedrez)
  for (int i = 0; i < w; i += 4) {
    for (int j = 0; j < h; j += 4) {
      if (((i / 4) + (j / 4)) % 2 == 0) {
        int drawW = (w - i < 4) ? (w - i) : 4;
        int drawH = (h - j < 4) ? (h - j) : 4;
        display.fillRect(x + i, y + j, drawW, drawH, 0x001F); 
      }
    }
  }
}

// Función para dibujar bandera de carreras (a cuadros)
void drawGoalFlag(int x, int y, int w, int h) {
  display.fillRect(x, y, w, h, 0x0000); // Limpiar fondo
  display.fillRect(x + 5, y, 4, h, 0x8410); // Mástil (Gris)
  
  // Tela de la bandera (Cuadros blancos y negros)
  int flagW = w - 15;
  int flagH = h / 2;
  int startX = x + 9;
  int startY = y + 4;
  int sqSize = 5; // Tamaño de cada cuadro
  
  for (int i = 0; i < flagW; i += sqSize) {
    for (int j = 0; j < flagH; j += sqSize) {
      uint16_t color = (((i / sqSize) + (j / sqSize)) % 2 == 0) ? 0xFFFF : 0x0000;
      int drawW = (flagW - i < sqSize) ? (flagW - i) : sqSize;
      int drawH = (flagH - j < sqSize) ? (flagH - j) : sqSize;
      display.fillRect(startX + i, startY + j, drawW, drawH, color);
    }
  }
  // Borde blanco de la bandera para que resalte
  display.drawRect(startX, startY, flagW, flagH, 0xFFFF);
}

void drawMaze() {
  for (int i = 0; i < NUM_WALLS; i++) {
    drawWall(walls[i].x, walls[i].y, walls[i].w, walls[i].h); 
  }
  drawGoalFlag(goal.x, goal.y, goal.w, goal.h);
}

// Pantalla de celebración
void playVictoryScreen() {
  display.fillScreen(0x0000);
  unsigned long startMillis = millis();

  // Bucle de celebración de 3 segundos
  while (millis() - startMillis < 3000) {
    // 1. Generar un fuego artificial retro aleatorio
    int fx = random(20, 300);
    int fy = random(20, 220);
    uint16_t fColor = random(0x1111, 0xFFFF); // Colores aleatorios brillantes

    // Dibujar explosión retro (cruz y diagonales)
    display.drawLine(fx - 15, fy, fx + 15, fy, fColor);
    display.drawLine(fx, fy - 15, fx, fy + 15, fColor);
    display.drawLine(fx - 10, fy - 10, fx + 10, fy + 10, fColor);
    display.drawLine(fx - 10, fy + 10, fx + 10, fy - 10, fColor);

    // 2. Texto WINNER intermitente en el centro
    display.setTextSize(5);
    if ((millis() / 150) % 2 == 0) {
      display.setTextColor(0xFFE0, 0x0000); // Amarillo con fondo negro
    } else {
      display.setTextColor(0xF800, 0x0000); // Rojo con fondo negro
    }
    display.setCursor(60, 100);
    display.print("WINNER!");

    delay(80); // Velocidad de la animación

    // 3. Borrar la explosión anterior para el efecto de parpadeo
    display.drawLine(fx - 15, fy, fx + 15, fy, 0x0000);
    display.drawLine(fx, fy - 15, fx, fy + 15, 0x0000);
    display.drawLine(fx - 10, fy - 10, fx + 10, fy + 10, 0x0000);
    display.drawLine(fx - 10, fy + 10, fx + 10, fy - 10, 0x0000);
  }

  // Al terminar, resetear todo al estado inicial
  minionX = 5; minionY = 5;
  velX = 0; velY = 0;
  virtualAngleX = 0; virtualAngleY = 0;
  bananas = 10; 
  
  display.fillScreen(0x0000);
  drawMaze();
  oldX = 5; oldY = 5;
  lastFrameTime = millis();
}

// Retorna true si hay intersección entre dos rectángulos
bool checkCollision(float x1, float y1, int w1, int h1, float x2, float y2, int w2, int h2) {
  return (x1 < x2 + w2 && x1 + w1 > x2 && y1 < y2 + h2 && y1 + h1 > y2);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  if (!display.begin()) {
    while (true) { digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); delay(100); }
  }
  display.fillScreen(0x0000);
  drawMaze();

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

  minionX = 5;
  minionY = 5;
  oldX = (int)minionX;
  oldY = (int)minionY;
  lastFrameTime = millis();
}

void loop() {
  unsigned long currentMillis = millis();
  
  if (currentMillis - lastFrameTime >= frameDelay) {
    float dt = (currentMillis - lastFrameTime) / 1000.0;
    lastFrameTime = currentMillis;

    imu.getSensorData();
    
    // Cinemática del Giroscopio
    virtualAngleX += imu.data.gyroY * dt; 
    virtualAngleY += imu.data.gyroX * dt; 

    virtualAngleX *= 0.96;
    virtualAngleY *= 0.96;

    velX += virtualAngleX * 0.1; 
    velY += virtualAngleY * 0.1; 

    velX *= 0.85; 
    velY *= 0.85;

    float nextX = minionX + velX;
    float nextY = minionY + velY;
    
    const float rebote = 0.8; 
    bool colisionX = false;
    bool colisionY = false;
    bool hitWall = false;

    // Colisiones con bordes
    if (nextX < 0) { nextX = 0; colisionX = true; hitWall = true; } 
    else if (nextX > display.width() - MINION_WIDTH) { nextX = display.width() - MINION_WIDTH; colisionX = true; hitWall = true; }

    if (nextY < 0) { nextY = 0; colisionY = true; hitWall = true; } 
    else if (nextY > display.height() - MINION_HEIGHT) { nextY = display.height() - MINION_HEIGHT; colisionY = true; hitWall = true; }

    // Colisiones con las paredes
    for (int i = 0; i < NUM_WALLS; i++) {
      if (checkCollision(nextX, minionY, MINION_WIDTH, MINION_HEIGHT, walls[i].x, walls[i].y, walls[i].w, walls[i].h)) {
        colisionX = true; hitWall = true;
        if (velX > 0) nextX = walls[i].x - MINION_WIDTH;
        else nextX = walls[i].x + walls[i].w;            
      }

      if (checkCollision(nextX, nextY, MINION_WIDTH, MINION_HEIGHT, walls[i].x, walls[i].y, walls[i].w, walls[i].h)) {
        colisionY = true; hitWall = true;
        if (velY > 0) nextY = walls[i].y - MINION_HEIGHT; 
        else nextY = walls[i].y + walls[i].h;             
      }
    }

    if (colisionX) velX = -velX * rebote;
    if (colisionY) velY = -velY * rebote;

    // Sistema de Daño (Pérdida de Bananas)
    if (hitWall && (currentMillis - lastDamageTime > DAMAGE_COOLDOWN)) {
      bananas--;
      lastDamageTime = currentMillis;
    }

    // Condición de Derrota (0 Bananas)
    if (bananas <= 0) {
      minionX = 5; minionY = 5;
      velX = 0; velY = 0;
      virtualAngleX = 0; virtualAngleY = 0;
      bananas = 10; 
      
      display.fillScreen(0x0000);
      drawMaze();
      oldX = 5; oldY = 5;
      return; 
    }

    minionX = nextX;
    minionY = nextY;

    // Verificar si llegó a la Meta (Victoria)
    if (checkCollision(minionX, minionY, MINION_WIDTH, MINION_HEIGHT, goal.x, goal.y, goal.w, goal.h)) {
      playVictoryScreen(); // Llama a la animación y reinicia el laberinto internamente
      return; 
    }

    // Dibujado Diferencial
    int currentX = (int)minionX;
    int currentY = (int)minionY;

    if (currentX != oldX || currentY != oldY) {
      display.fillRect(oldX, oldY, MINION_WIDTH, MINION_HEIGHT, 0x0000); 
      
      for (int i = 0; i < NUM_WALLS; i++) {
        if (checkCollision(oldX, oldY, MINION_WIDTH, MINION_HEIGHT, walls[i].x, walls[i].y, walls[i].w, walls[i].h)) {
          drawWall(walls[i].x, walls[i].y, walls[i].w, walls[i].h);
        }
      }
      if (checkCollision(oldX, oldY, MINION_WIDTH, MINION_HEIGHT, goal.x, goal.y, goal.w, goal.h)) {
          drawGoalFlag(goal.x, goal.y, goal.w, goal.h);
      }
    }

    display.drawRGBBitmap(currentX, currentY, epd_bitmap_minion, MINION_WIDTH, MINION_HEIGHT);

    oldX = currentX;
    oldY = currentY;

    // Dibujar UI
    display.setTextSize(1);
    display.setTextColor(0xFFE0, 0x0000); 
    display.setCursor(240, 220); 
    display.print("Bananas: ");
    display.print(bananas);
    if (bananas < 10) display.print(" "); 
  }
}