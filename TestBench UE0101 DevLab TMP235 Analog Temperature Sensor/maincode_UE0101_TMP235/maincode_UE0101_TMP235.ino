/*
==== CÓDIGO DE INTEGRACIÓN SHIELD PULSAR C6 UE0101 DevLab: TMP235 Analog Temperature Sensor ====
*/

#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <Arduino.h>

// ---- Pines I2C de OLED ----
#define SDA_PIN 22  // SDA Pines de lectura para sensor ICP
#define SCL_PIN 23  // SCL Pines de lectura para sensor ICP

// ---- Pines de lectura para sensor analógico [6]----
#define SENS_1 0
#define SENS_2 1
#define SENS_3 2
#define SENS_4 4
#define SENS_5 5
#define SENS_6 6

// ---- Pines de switcheo y manejo de relevadores || Neopixel ----
#define SWITCH 15
#define RELAY 14
#define NEOPIX 8

// ---- Configuración de la pantala OLED ----
#define OLED_RESET -1                                                      // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_WIDTH 128                                                   // OLED display width, in pixels
#define SCREEN_HEIGHT 64                                                   // OLED display height, in pixels
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);  // Objeto de la OLED

Adafruit_NeoPixel np = Adafruit_NeoPixel(3, NEOPIX, NEO_GRB + NEO_KHZ800);  // Objeto NeoPixel

// ---- Banderas y Variables ----
bool flagSW = false;
int animX = 0;

// ==== Declaración de Variables JSON ====
StaticJsonDocument<128> sendJSON;

void setup() {

  Serial.begin(115200);

  // ---- Declaración de pines ----
  pinMode(SWITCH, INPUT_PULLUP);
  pinMode(RELAY, OUTPUT);

  digitalWrite(RELAY, HIGH);

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

  // ---- Inicialización del Neopixel ----
  np.begin();
  np.setBrightness(20);
  np.setPixelColor(0, np.Color(0, 255, 0));
  np.show();
}

void loop() {

  sendJSON.clear();

  // ---- HEADER ---
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(25, 0);
  display.println("TEMPERATURA");
  display.drawLine(0, 12, animX, 12, SSD1306_WHITE);  // Línea animada
  animX += 4;                                         // velocidad
  if (animX > 127) animX = 0;


  // ---- Valores de Temperatura y muestreo ----
  float t1 = readTempC(SENS_1);
  float t2 = readTempC(SENS_2);
  float t3 = readTempC(SENS_3);
  float t4 = readTempC(SENS_4);
  float t5 = readTempC(SENS_5);
  float t6 = readTempC(SENS_6);

  display.setCursor(5, 20);
  display.print("S1: ");
  display.print(t1, 0);
  display.println(" C");

  display.setCursor(5, 35);
  display.print("S2: ");
  display.print(t2, 0);
  display.println(" C");

  display.setCursor(5, 50);
  display.print("S3: ");
  display.print(t3, 0);
  display.println(" C");

  display.setCursor(70, 20);
  display.print("S4: ");
  display.print(t4, 0);
  display.println(" C");

  display.setCursor(70, 35);
  display.print("S5: ");
  display.print(t5, 0);
  display.println(" C");

  display.setCursor(70, 50);
  display.print("S6: ");
  display.print(t6, 0);
  display.println(" C");
  sendJSON["s6"] = t6;

  sendJSON["s1"] = String(t1, 2);
  sendJSON["s2"] = String(t2, 2);
  sendJSON["s3"] = String(t3, 2);

  sendJSON["s4"] = String(t4, 2);
  sendJSON["s5"] = String(t5, 2);
  sendJSON["s6"] = String(t6, 2);


  // ---- Botón de Switch ---
  if (digitalRead(SWITCH) == LOW) {
    delay(30);  // debounce
    if (digitalRead(SWITCH) == LOW) {
      flagSW = !flagSW;
      while (digitalRead(SWITCH) == LOW)
        ;  // espera a soltar
    }
  }


  display.setCursor(100, 0);
  if (flagSW == true) {
    digitalWrite(RELAY, LOW);
    display.println("ON");
    sendJSON["relay"] = "1";
  } else {
    digitalWrite(RELAY, HIGH);
    display.println("OFF");
    sendJSON["relay"] = "2";
  }

  display.display();

  if (t1 > 30 || t2 > 30 || t3 > 30 || t4 > 30 || t5 > 30 || t6 > 30) {
    np.setPixelColor(0, np.Color(255, 0, 0));
    np.show();
  } else {
    np.setPixelColor(0, np.Color(0, 255, 0));
    np.show();
  }

  serializeJson(sendJSON, Serial);
  Serial.println();
  delay(100);
}




// ---- Funciones Auxiliares -----
float readTempC(uint8_t PIN) {
  const int N = 5;
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