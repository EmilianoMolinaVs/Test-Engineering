#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>

// Conexión SPI:
// pin 7 --> LCD reset (RST)
// pin 6 --> LCD chip select (SCE)
// pin 5 --> Data/Command select (D/C)
// pin 4 --> Serial data input (DN/ MOSI)
// pin 3 --> Serial clock out (SCLK)

#define RST_PIN 14
#define SCL_PIN 6 //4
#define DAT_PIN 2 //0
#define MOSI_PIN 7 // 5
#define SCE_PIN 18 //11

// Adafruit_PCD8544 display = Adafruit_PCD8544(3, 4, 5, 6, 7);
Adafruit_PCD8544 display = Adafruit_PCD8544(SCL_PIN, MOSI_PIN, DAT_PIN, SCE_PIN, RST_PIN);

String nombre = "";  // Variable para guardar el nombre ingresado

void setup() {
  Serial.begin(115200);
  SPI.begin();

  display.begin();
  display.setContrast(60);  // Ajusta contraste (0–127)
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0, 0);
  display.println("Hola!");
  display.println("Soy la LCD Nokia 5110");
  display.display();

  delay(2000);

  Serial.println("Hola! Escribe tu nombre en el monitor serie y presiona Enter:");
  display.println("Escribe tu nombre y presiona Enter:");
  display.display();
}

void loop() {
  // Si llega texto por el monitor serie
  if (Serial.available() > 0) {
    delay(50);
    nombre = Serial.readString();  // Lee hasta que des Enter
    nombre.trim();                 // Quita espacios o saltos extra

    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Hola,");
    display.println(nombre);
    display.display();

    Serial.print("Se mostro en pantalla: ");
    Serial.println(nombre);
  }
}