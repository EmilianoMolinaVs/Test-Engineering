#include <Adafruit_ST7735.h>  // Librería para ST7735
#include <SPI.h>
#include "Adafruit_GFX.h"

#define CS_PIN 18
#define DC_PIN 21
#define RST_PIN 5
#define SCL_PIN 6
#define SDA_PIN 7

Adafruit_ST7735 tft = Adafruit_ST7735(CS_PIN, DC_PIN, SDA_PIN, SCL_PIN, RST_PIN);

void setup() {
  Serial.begin(9600);
  Serial.print(F("Holi Test de prueba para ST7735 TFT"));
  tft.initR(INITR_MINI160x80);
  Serial.println(F("Tiempo        (microsegundos)"));
  tft.setRotation(1);
  Serial.print(F("Texto "));
  Serial.println(testText());
  delay(2000);
  Serial.print(F("Líneas   "));
  Serial.println(testLines(ST77XX_CYAN));
  delay(2000);
  Serial.print(F("Rectángulos (contorno) "));
  Serial.println(testRects(ST77XX_GREEN));
  delay(2000);
  tft.fillScreen(ST77XX_BLACK);
  Serial.print(F("Círculos     (contorno) "));
  Serial.println(testCircles(10, ST77XX_RED));
  delay(2000);
  Serial.print(F("Triángulos  (contorno) "));
  Serial.println(testTriangles());
  delay(2000);
  Serial.print(F("Triángulos  (solidos) "));
  Serial.println(testFilledTriangles());
  delay(500);
  Serial.print(F("Rectángulos con aristas redondeados (contorno) "));
  Serial.println(testRoundRects());
  delay(2000);
  Serial.print(F("Rectángulos con aristas redondeados (solido) "));
  Serial.println(testFilledRoundRects());
  delay(2000);
  Serial.print(F("Pantalla de colores "));
  Serial.println(testFillScreen());
  delay(2000);
  Serial.println(F("Hecho!"));
}

void loop(void) {
  for (uint8_t rotation = 0; rotation < 4; rotation++) {
    tft.setRotation(rotation);
    testText();
    delay(3000);
  }
}

unsigned long testFillScreen() {
  unsigned long start = micros();
  tft.fillScreen(ST77XX_RED);
  delay(2000);
  tft.fillScreen(ST77XX_GREEN);
  delay(2000);
  tft.fillScreen(ST77XX_BLUE);
  delay(2000);
  tft.fillScreen(ST77XX_WHITE);
  delay(2000);
  return micros() - start;
}

unsigned long testText() {
  tft.fillScreen(ST77XX_BLACK);
  unsigned long start = micros();
  tft.setCursor(0, 0);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.println("Esta vivo!");
  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(2);
  tft.println("UNIT");
  tft.setTextColor(ST77XX_RED);
  tft.setTextSize(3);
  tft.println("ELECTRONICS");
  tft.println();
  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(5);
  tft.println("123");
  return micros() - start;
}

unsigned long testLines(uint16_t color) {
  unsigned long start, t;
  int x1, y1, x2, y2,
    w = tft.width(),
    h = tft.height();

  tft.fillScreen(ST77XX_BLACK);

  x1 = y1 = 0;
  y2 = h - 1;
  start = micros();
  for (x2 = 0; x2 < w; x2 += 6) tft.drawLine(x1, y1, x2, y2, color);
  x2 = w - 1;
  for (y2 = 0; y2 < h; y2 += 6) tft.drawLine(x1, y1, x2, y2, color);
  t = micros() - start;  // fillScreen doesn't count against timing


  return micros() - start;
}

unsigned long testFastLines(uint16_t color1, uint16_t color2) {
  unsigned long start;
  int x, y, w = tft.width(), h = tft.height();

  tft.fillScreen(ST77XX_BLACK);
  start = micros();
  for (y = 0; y < h; y += 5) tft.drawFastHLine(0, y, w, color1);
  for (x = 0; x < w; x += 5) tft.drawFastVLine(x, 0, h, color2);

  return micros() - start;
}

unsigned long testRects(uint16_t color) {
  unsigned long start;
  int n, i, i2,
    cx = tft.width() / 2,
    cy = tft.height() / 2;

  tft.fillScreen(ST77XX_BLACK);
  n = min(tft.width(), tft.height());
  start = micros();
  for (i = 2; i < n; i += 6) {
    i2 = i / 2;
    tft.drawRect(cx - i2, cy - i2, i, i, color);
  }
  return micros() - start;
}

unsigned long testFilledRects(uint16_t color1, uint16_t color2) {
  unsigned long start, t = 0;
  int n, i, i2, cx = tft.width() / 2 - 1, cy = tft.height() / 2 - 1;
  tft.fillScreen(ST77XX_BLACK);
  n = min(tft.width(), tft.height());
  for (i = n; i > 0; i -= 6) {
    i2 = i / 2;
    start = micros();
    tft.fillRect(cx - i2, cy - i2, i, i, color1);
    t += micros() - start;

    tft.drawRect(cx - i2, cy - i2, i, i, color2);
  }

  return t;
}

unsigned long testFilledCircles(uint8_t radius, uint16_t color) {
  unsigned long start;
  int x, y, w = tft.width(), h = tft.height(), r2 = radius * 2;

  tft.fillScreen(ST77XX_BLACK);
  start = micros();
  for (x = radius; x < w; x += r2) {
    for (y = radius; y < h; y += r2) {
      tft.fillCircle(x, y, radius, color);
    }
  }

  return micros() - start;
}

unsigned long testCircles(uint8_t radius, uint16_t color) {
  unsigned long start;
  int x, y, r2 = radius * 2,
            w = tft.width() + radius,
            h = tft.height() + radius;

  start = micros();
  for (x = 0; x < w; x += r2) {
    for (y = 0; y < h; y += r2) {
      tft.drawCircle(x, y, radius, color);
    }
  }

  return micros() - start;
}

unsigned long testTriangles() {
  unsigned long start;
  int n, i, cx = tft.width() / 2 - 1,
            cy = tft.height() / 2 - 1;

  tft.fillScreen(ST77XX_BLACK);
  n = min(cx, cy);
  start = micros();
  for (i = 0; i < n; i += 5) {
    tft.drawTriangle(
      cx, cy - i,
      cx - i, cy + i,
      cx + i, cy + i,
      tft.color565(200, 20, i));
  }
  return micros() - start;
}
unsigned long testFilledTriangles() {
  unsigned long start, t = 0;
  int i, cx = tft.width() / 2 - 1,
         cy = tft.height() / 2 - 1;
  tft.fillScreen(ST77XX_BLACK);
  start = micros();
  for (i = min(cx, cy); i > 10; i -= 5) {
    start = micros();
    tft.fillTriangle(cx, cy - i, cx - i, cy + i, cx + i, cy + i,
                     tft.color565(0, i, i));
    t += micros() - start;
    tft.drawTriangle(cx, cy - i, cx - i, cy + i, cx + i, cy + i,
                     tft.color565(i, i, 0));
  }

  return t;
}

unsigned long testRoundRects() {
  unsigned long start;
  int w, i, i2,
    cx = tft.width() / 2,
    cy = tft.height() / 2;

  tft.fillScreen(ST77XX_BLACK);
  w = min(tft.width(), tft.height());
  start = micros();
  for (i = 0; i < w; i += 8) {
    i2 = i / 2 - 2;
    tft.drawRoundRect(cx - i2, cy - i2, i, i, i / 8, tft.color565(i, 100, 100));
  }
  return micros() - start;
}
unsigned long testFilledRoundRects() {
  unsigned long start;
  int i, i2, cx = tft.width() / 2 + 10,
             cy = tft.height() / 2 + 10;
  tft.fillScreen(ST77XX_BLACK);
  start = micros();
  for (i = min(tft.width(), tft.height()) - 20; i > 25; i -= 6) {
    i2 = i / 2;
    tft.fillRoundRect(cx - i2, cy - i2, i - 20, i - 20, i / 8, tft.color565(100, i / 2, 100));
  }

  return micros() - start;
}