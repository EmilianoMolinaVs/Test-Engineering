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

#define TX2 16  // Serial PagWeb
#define RX2 17

#define ONSX 32    // Activación de Q1 para alimentación de SX1308
#define WAKEUP 33  // WakeUp SW2
#define DIVIn 34   // Divisor de tensión TP4056 In+ In-
#define DIVOut 35  // Divisor de tensión TP4056 Out+ Out-


// ==== Creación de objetos ====
HardwareSerial PagWeb(1);

// Adafruit_PCD8544 display = Adafruit_PCD8544(3, 4, 5, 6, 7);
Adafruit_PCD8544 display = Adafruit_PCD8544(SCL_PIN, MOSI_PIN, DAT_PIN, SCE_PIN, RST_PIN);

// Variables y banderas
String stateButton = "";      // Estado de botón de WakeUp
bool DIVIn_estable = false;   // Estabilidad de GPIO34 DIVIn
bool DIVOut_estable = false;  // Estabilidad de GPIO35 DIVOut

// Variables de JSON
String JSON_entrada;  // Variable que recibe al JSON en crudo de PagWeb
StaticJsonDocument<200> receiveJSON;

String JSON_salida;  // Variable que envía el JSON de datos
StaticJsonDocument<200> sendJSON;


void setup() {

  // === Declaración e inicialización de pines - gpios
  pinMode(WAKEUP, INPUT);
  pinMode(DIVIn, INPUT);
  pinMode(DIVOut, INPUT);

  pinMode(VCC, OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(RGB_R, OUTPUT);  // Activos a BAJA
  pinMode(RGB_G, OUTPUT);
  pinMode(RGB_B, OUTPUT);
  pinMode(ONSX, OUTPUT);

  digitalWrite(ONSX, HIGH);  // Base de transistor para SX1308
  digitalWrite(VCC, HIGH);   // VCC de OLED

  // Inicialización de Serial
  Serial.begin(115200);
  Serial.println("Serial inicializado");

  PagWeb.begin(115200, SERIAL_8N1, RX2, TX2);
  Serial.println("Serial PagWeb inicializado");

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

  if (PagWeb.available()) {

    JSON_entrada = PagWeb.readStringUntil('\n');                              // Leer hasta newline (JSON en crudo)
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
            serializeJson(sendJSON, PagWeb);  // Envío de datos por JSON a la PagWeb
            PagWeb.println();
            break;
          }

        case 2:
          {
            sendJSON.clear();  // Limpia cualquier dato previo
            display.clearDisplay();

            // ==== Validación de I34 Divisor V en +In -In
            delay(200);
            DIVIn_estable = leerAnalogicoEstable(DIVIn);
            if (DIVIn_estable) {
              writeScreen(2, "Analog I34: ", "OK");
              sendJSON["analog34"] = "OK";
            } else {
              writeScreen(2, "Analog I34: ", "Fail");
              sendJSON["analog34"] = "Fail";
            }

            // ==== Validación de I35 Divisor V en +Out -Out
            // Va a leer correctamente 5V hasta que se suelde el SX1308
            delay(200);
            DIVOut_estable = leerAnalogicoEstable(DIVOut);
            if (DIVOut_estable) {
              writeScreen(3, "Analog I35: ", "OK");
              sendJSON["analog35"] = "OK";
            } else {
              writeScreen(3, "Analog I35: ", "Fail");
              sendJSON["analog35"] = "Fail";
            }

            serializeJson(sendJSON, PagWeb);  // Envío de datos por JSON a la PagWeb
            PagWeb.println();
            break;
          }
      }
    }
  } else if (digitalRead(WAKEUP) == LOW) {

    // ==== Validación de SW2 Wake Up
    delay(200);
    if (digitalRead(WAKEUP) == HIGH)
      Serial.println("Botón presionado SW2 WakeUp");
    for (int i = 0; i < 15; i++) {
      demoBUTTON();
      delay(20);
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

void demoBUTTON() {
  delay(100);
  digitalWrite(RGB_R, LOW);
  digitalWrite(RGB_G, HIGH);
  digitalWrite(RGB_B, HIGH);

  delay(100);
  digitalWrite(RGB_R, HIGH);
  digitalWrite(RGB_G, LOW);
  digitalWrite(RGB_B, HIGH);

  delay(100);
  digitalWrite(RGB_R, LOW);
  digitalWrite(RGB_G, LOW);
  digitalWrite(RGB_B, HIGH);
}

void writeScreen(uint8_t line, String lbl, String msg) {
  int y = line * 8;  // 8 px por línea

  display.setCursor(0, y);
  display.print(lbl);

  display.setCursor(70, y);
  display.print(msg);

  display.display();
}


bool leerAnalogicoEstable(uint8_t GPIO) {
  int lecturas[10];
  int minVal = 4095;
  int maxVal = 0;
  int NUM_LECTURAS = 10;
  int UMBRAL = 10;

  for (int i = 0; i < NUM_LECTURAS; i++) {
    Serial.println("Lectura: " + String(analogRead(GPIO)));
    lecturas[i] = analogRead(GPIO);

    if (lecturas[i] < minVal) minVal = lecturas[i];
    if (lecturas[i] > maxVal) maxVal = lecturas[i];

    delay(10);  // pequeño retardo entre lecturas
  }

  if ((maxVal - minVal) <= UMBRAL) {
    return true;
  } else {
    return false;
  }
}
