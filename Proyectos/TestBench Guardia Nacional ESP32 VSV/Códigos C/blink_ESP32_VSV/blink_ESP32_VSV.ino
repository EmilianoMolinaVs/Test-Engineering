/* ==== Código de Integración ESP32 VSV ====

*/

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include <Wire.h>

// Conexión SPI:
// pin 7 --> LCD reset (RST)
// pin 6 --> LCD chip select (SCE)
// pin 5 --> Data/Command select (D/C)
// pin 4 --> Serial data input (DN/ MOSI)
// pin 3 --> Serial clock out (SCLK)

// Acomodo de GPIOs para SPI OLED
#define VCC 14
#define RST_PIN 18
#define SCL_PIN 22
#define DAT_PIN 19
#define MOSI_PIN 21
#define SCE_PIN 4

#define LED 23  // gpio provisional

#define RGB_R 13  // GPIOs de LED RGB
#define RGB_G 15
#define RGB_B 2

#define TX2 16
#define Rx2 17

#define XD1 32     // Activación de Q1 para salida en IN+
#define WAKEUP 33  // WakeUp SW2
#define XD3 34     // Divisor de tensión TP4056 In+ In-
#define XD4 35     // Divisor de tensión TP4056 Out+ Out-


// ==== Creación de objetos ====

// Adafruit_PCD8544 display = Adafruit_PCD8544(3, 4, 5, 6, 7);
Adafruit_PCD8544 display = Adafruit_PCD8544(SCL_PIN, MOSI_PIN, DAT_PIN, SCE_PIN, RST_PIN);

// Variables y banderas
String stateButton;  // Estado de botón de WakeUp

// Variables de JSON
String JSON_entrada;  // Variable que recibe al JSON en crudo de PagWeb
StaticJsonDocument<200> receiveJSON;

String JSON_salida;  // Variable que envía el JSON de datos
StaticJsonDocument<200> sendJSON;


void setup() {

  // === Declaración e inicialización de pines - gpios
  pinMode(WAKEUP, INPUT);

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

  if (Serial.available()) {

    JSON_entrada = Serial.readStringUntil('\n');                              // Leer hasta newline (JSON en crudo)
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);  // Deserializa el JSON y guarda la información en datosJSON

    if (!error) {
      String Function = receiveJSON["Function"];  // Function es la variable de interés del JSON
      int opc = 0;                                // Variable de switcheo

      if (Function == "ping") opc = 1;          // {"Function":"ping"}
      else if (Function == "testAll") opc = 2;  // {"Function":"testAll"}

      switch (opc) {
        case 1:
          {
            sendJSON.clear();  // Limpia cualquier dato previo
            sendJSON["ping"] = "pong";
            serializeJson(sendJSON, Serial);  // Envío de datos por JSON a la PagWeb
            Serial.println();
            break;
          }

        case 2:
          {
            Serial.println("Presiona el SW2 WakeUp");
            for (int i = 0; i < 100; i++) {
              if (digitalRead(WAKEUP) == LOW) {
                for (int j = 0; j < 50; j++) {
                  if (digitalRead(WAKEUP) == HIGH) {
                    stateButton = "OK";
                    writeScreen("Boton SW2: ", stateButton);
                    return;
                  }
                  delay(500);
                }
              }
              delay(500);
            }

            break;
          }
      }
    }
  } else {
    demoLED();
  }
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

void writeScreen(String lbl, String msg) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(lbl);
  display.setCursor(60, 0);
  display.println(msg);
  display.display();
}
