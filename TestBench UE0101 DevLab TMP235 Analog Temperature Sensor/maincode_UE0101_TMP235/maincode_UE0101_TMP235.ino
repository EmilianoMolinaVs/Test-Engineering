/*
==== CÓDIGO DE INTEGRACIÓN SHIELD PULSAR C6 UE0101 DevLab: TMP235 Analog Temperature Sensor ====
*/
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

#define CS_PIN 18
#define DC_PIN 21
#define RST_PIN 5

Adafruit_ST7735 lcd = Adafruit_ST7735(CS_PIN, DC_PIN, LCD_RST);

// Colores por sensor
const uint16_t sensorColors[9] = {
  ST77XX_RED,
  ST77XX_GREEN,
  ST77XX_BLUE,
  ST77XX_YELLOW,
  ST77XX_CYAN,
  ST77XX_MAGENTA,
  0xFD20,        // Naranja
  ST77XX_WHITE,  // Blanco
  0x07E0         // Verde claro
};

void setup() {
  Serial.begin(115200);

  // ESTA es la tabla correcta para la mayoría de 0.96"
  tft.initR(INITR_MINI160x80);
  tft.setRotation(3);
  tft.fillScreen(ST77XX_WHITE);

  tft.setTextSize(1);
  tft.setTextWrap(false);

  escribirTexto("Sistema listo", 10, 10, ST77XX_GREEN);
}

void loop() {
  delay(2000);
  escribirTexto("ERROR", 10, 40, ST77XX_RED);

  delay(2000);
  escribirTexto("OK", 10, 40, ST77XX_GREEN);
}

void escribirTexto(const char* texto, int x, int y, uint16_t color) {
  tft.fillRect(x, y, 260, 140, ST77XX_BLACK);
  tft.setCursor(x, y);
  tft.setTextColor(color, ST77XX_BLACK);
  tft.print(texto);
}
