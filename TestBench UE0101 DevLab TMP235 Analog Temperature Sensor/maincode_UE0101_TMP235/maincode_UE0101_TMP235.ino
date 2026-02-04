/*
==== CÓDIGO DE INTEGRACIÓN SHIELD PULSAR C6 UE0101 DevLab: TMP235 Analog Temperature Sensor ====
*/

#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

// ---- Pines I2C de OLED ----
#define SDA_PIN 22  // SDA Pines de lectura para sensor ICP
#define SCL_PIN 23  // SCL Pines de lectura para sensor ICP

// ---- Pines de lectura para sensor analógico [6]----
#define SENS_1 0
#define SENS_2 1
#define SENS_3 8
#define SENS_4 4
#define SENS_5 5
#define SENS_6 6

// ---- Pin de Switcheo entre sensores ----
#define SWITCH 16

// ---- Configuración de la pantala OLED ----
#define OLED_RESET -1                                                      // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_WIDTH 128                                                   // OLED display width, in pixels
#define SCREEN_HEIGHT 64                                                   // OLED display height, in pixels
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);  // Objeto de la OLED

// ---- Banderas ----
bool flagSW = false;

void setup() {
  Serial.begin(115200);

  // ---- Declaración de pines ----
  pinMode(SWITCH, INPUT);

  // ---- Inicialización de I2C de OLED ----
  Wire.begin(SDA_PIN, SCL_PIN);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED no encontrada");
    while (true)
      ;
  }

  // ---- Configuración del ADC ----
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
}

void loop() {

  // ---- HEADER ---
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(35, 0);
  display.println("TEMPERATURA");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  display.display();


  // ---- Valores medidos ----
  float t = readTempC(SENS_6);

  display.setCursor(10, 20);
  display.println("Sensor 1: " + String(t, 2) + " C");
  display.display();


  Serial.print("Temp: ");
  Serial.print(t, 2);
  Serial.println(" C");

  delay(1000);
}




// ---- Funciones Auxiliares -----
float readTempC(uint8_t PIN) {
  const int N = 20;
  long sum_mv = 0;

  for (int i = 0; i < N; i++) {
    sum_mv += analogReadMilliVolts(PIN);
    delay(5);
  }

  float v_mv = sum_mv / (float)N;

  // Tu curva: 0°C = 500mV, 10mV/°C
  float tempC = (v_mv - 500.0f) / 10.0f;
  return tempC;
}