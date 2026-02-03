#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

// ================== LCD PINS ==================
#define LCD_CS   D10
#define LCD_DC   D7
#define LCD_RST  A6

#define Buzzer D5

Adafruit_ST7735 lcd = Adafruit_ST7735(LCD_CS, LCD_DC, LCD_RST);

// ================== TOUCH SENSORS ==================
const uint8_t sensorPins[9] = { A0, A1, A2, A3, A4, A5, D3, D4, D6 };

const char* sensorNames[9] = { "S1","S2","S3","S4","S5","S6","S7","S8","S9" };

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

uint16_t lastMask = 0;

// ================== BEEP SUAVE ==================
void softBeep() {
  tone(Buzzer, 2200, 40);
}

// ================== SETUP ==================
void setup() {

  for (uint8_t i = 0; i < 9; i++) {
    pinMode(sensorPins[i], INPUT_PULLDOWN);
  }

  pinMode(Buzzer, OUTPUT);
  digitalWrite(Buzzer, LOW);

  // ✅ Para 0.96" 80x160 IPS normalmente es esto:
  lcd.initR(INITR_MINI160x80);

  lcd.setRotation(3);

  // ✅ Si ves el fondo al revés (negro->blanco), esta línea lo corrige en muchos módulos IPS:
  lcd.invertDisplay(true);
  // Si con true se ve raro (colores mal), prueba:
  // lcd.invertDisplay(false);

  // Fondo negro real
  lcd.fillScreen(ST77XX_BLACK);

  // ---- HEADER ----
  lcd.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
  lcd.setTextSize(1);
  lcd.setCursor(15, 5);
  lcd.println("TOUCH TESTBENCH");

  lcd.drawFastHLine(0, 18, 160, ST77XX_CYAN);

  lcd.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  lcd.setCursor(5, 25);
  lcd.println("Sensores activos:");
}

// ================== LOOP ==================
void loop() {

  uint16_t mask = 0;
  uint8_t count = 0;
  int lastSingle = -1;

  for (uint8_t i = 0; i < 9; i++) {
    if (digitalRead(sensorPins[i]) == HIGH) {
      mask |= (1 << i);
      count++;
      lastSingle = i;
    }
  }

  if (mask != lastMask) {

    if (mask != 0) softBeep();

    lcd.fillRect(0, 45, 160, 80, ST77XX_BLACK);

    // ================== NINGUNO ==================
    if (mask == 0) {
      lcd.setTextColor(ST77XX_RED, ST77XX_BLACK);
      lcd.setTextSize(2);
      lcd.setCursor(35, 50);
      lcd.println("NINGUNO");
    }
    // ================== SOLO UNO ==================
    else if (count == 1 && lastSingle >= 0) {

      lcd.setTextColor(sensorColors[lastSingle], ST77XX_BLACK);
      lcd.setTextSize(4);
      lcd.setCursor(30, 45);
      lcd.print(lastSingle + 1);

      lcd.setTextSize(1);
      lcd.setCursor(70, 50);
      lcd.print(sensorNames[lastSingle]);
    }
    // ================== VARIOS ==================
    else {
      lcd.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
      lcd.setTextSize(1);
      lcd.setCursor(5, 45);
      lcd.println("Multiples:");

      int x = 5;
      int y = 55;

      for (uint8_t i = 0; i < 9; i++) {
        if (mask & (1 << i)) {
          lcd.setTextColor(sensorColors[i], ST77XX_BLACK);
          lcd.setCursor(x, y);
          lcd.print(sensorNames[i]);
          x += 35;
          if (x > 150) { x = 5; y += 12; }
        }
      }
    }

    lastMask = mask;
  }
}
