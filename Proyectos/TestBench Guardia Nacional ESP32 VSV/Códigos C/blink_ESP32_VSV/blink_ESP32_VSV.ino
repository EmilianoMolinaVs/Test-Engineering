/* ==== Código de Integración 


*/


#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>


// Conexión SPI:
// pin 7 --> LCD reset (RST)
// pin 6 --> LCD chip select (SCE)
// pin 5 --> Data/Command select (D/C)
// pin 4 --> Serial data input (DN/ MOSI)
// pin 3 --> Serial clock out (SCLK)

// Acomodo de Gpios para VSV GN
#define VCC 14
#define RST_PIN 18
#define SCL_PIN 22
#define DAT_PIN 19
#define MOSI_PIN 21
#define SCE_PIN 4


#define LED 23  // gpio provisional

#define RGB_R 13
#define RGB_G 15
#define RGB_B 2

// 18 sck rst
// 19 miso dat
// 21 sda dc
// 22 scl sck
// 23 miso
// 4 gpio bl


// ==== Creación de objetos ====
// Adafruit_PCD8544 display = Adafruit_PCD8544(3, 4, 5, 6, 7);
Adafruit_PCD8544 display = Adafruit_PCD8544(SCL_PIN, MOSI_PIN, DAT_PIN, SCE_PIN, RST_PIN);

void setup() {

  // Declaración de pines - gpios
  pinMode(VCC, OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(RGB_R, OUTPUT);  // Activos a BAJA
  pinMode(RGB_G, OUTPUT);
  pinMode(RGB_B, OUTPUT);

  digitalWrite(VCC, HIGH);

  // Inicialización de Serial
  Serial.begin(115200);
  Serial.println("Serial inicializado");

  // Inicialización de SPI
  SPI.begin();

  display.begin();
  display.setContrast(60);  // Ajusta contraste (0–127)
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0, 0);
  display.println("Hola Mundo!");
  display.println("Prueba VSV :D");
  display.display();
}

void loop() {
  demoLED();
}

void demoLED() {
  delay(500);
  digitalWrite(RGB_R, LOW);
  digitalWrite(RGB_G, HIGH);
  digitalWrite(RGB_B, HIGH);

  delay(500);
  digitalWrite(RGB_R, HIGH);
  digitalWrite(RGB_G, LOW);
  digitalWrite(RGB_B, HIGH);

  delay(500);
  digitalWrite(RGB_R, LOW);
  digitalWrite(RGB_G, LOW);
  digitalWrite(RGB_B, HIGH);
}
