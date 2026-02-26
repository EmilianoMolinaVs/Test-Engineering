/* ==== Código de Integración ESP32 VSV ==== */

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include <Wire.h>

// ==== GPIOs ====
#define VCC 4
#define RST_PIN 23
#define SCL_PIN 19
#define DAT_PIN 22
#define MOSI_PIN 21
#define SCE_PIN 14
#define LED 18

#define RGB_R 13
#define RGB_G 15
#define RGB_B 2

#define TX2 16
#define RX2 17

#define ONSX 32
#define WAKEUP 33
#define DIVIn 34
#define DIVOut 35

// ==== Objetos ====
HardwareSerial PagWeb(1);
Adafruit_PCD8544 display = Adafruit_PCD8544(
  SCL_PIN, MOSI_PIN, DAT_PIN, SCE_PIN, RST_PIN);

// ==== Variables ====
bool DIVIn_estable = false;
bool DIVOut_estable = false;

int contador = 0;

// ---- botón ----
bool lastWakeupState = HIGH;
bool buttonMode = false;
unsigned long buttonTime = 0;

// ---- JSON ----
String JSON_entrada;
StaticJsonDocument<200> receiveJSON;

String JSON_salida;
StaticJsonDocument<200> sendJSON;

// =========================================================
void setup() {

  pinMode(WAKEUP, INPUT);
  pinMode(DIVIn, INPUT);
  pinMode(DIVOut, INPUT);

  pinMode(VCC, OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(RGB_R, OUTPUT);
  pinMode(RGB_G, OUTPUT);
  pinMode(RGB_B, OUTPUT);
  pinMode(ONSX, OUTPUT);

  digitalWrite(ONSX, HIGH);
  digitalWrite(VCC, HIGH);
  digitalWrite(LED, HIGH);

  Serial.begin(115200);
  PagWeb.begin(115200, SERIAL_8N1, RX2, TX2);

  SPI.begin();

  display.begin();
  display.setContrast(60);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(BLACK);

  display.setCursor(0, 0);
  display.println("Hola Mundooo!");
  display.println("Prueba VSV :D");
  display.display();
}

// =========================================================
void loop() {

  /* ======= PARTE JSON (SIN CAMBIOS) ======= */
  if (PagWeb.available()) {

    JSON_entrada = PagWeb.readStringUntil('\n');                              // Leer hasta newline (JSON en crudo)
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);  // Deserializa el JSON y guarda la información en datosJSON

    if (!error) {
      String Function = receiveJSON["Function"];  // Function es la variable de interés del JSON
      int opc = 0;                                // Variable de switcheo

      if (Function == "ping") opc = 1;            // {"Function":"ping"}
      else if (Function == "mac") opc = 2;        // {"Function":"mac"}
      else if (Function == "RGB_Red") opc = 3;    // {"Function":"RGB_Red"}
      else if (Function == "RGB_Blue") opc = 4;   // {"Function":"RGB_Blue"}
      else if (Function == "RGB_Green") opc = 5;  // {"Function":"RGB_Green"}
      else if (Function == "testAll") opc = 6;    // {"Function":"testAll"}


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

            // Obtener MAC (64 bits)
            uint64_t chipid = ESP.getEfuseMac();

            // Formatear MAC a string tipo XX:XX:XX:XX:XX:XX
            char macStr[18];
            sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
                    (uint8_t)(chipid >> 40),
                    (uint8_t)(chipid >> 32),
                    (uint8_t)(chipid >> 24),
                    (uint8_t)(chipid >> 16),
                    (uint8_t)(chipid >> 8),
                    (uint8_t)(chipid));

            sendJSON["Function"] = "mac";
            sendJSON["MAC"] = macStr;

            serializeJson(sendJSON, PagWeb);
            PagWeb.println();

            break;
          }

        case 3:  // Rojo
          {
            digitalWrite(RGB_R, LOW);
            digitalWrite(RGB_G, HIGH);
            digitalWrite(RGB_B, HIGH);
            delay(8000);
            break;
          }

        case 4:  // Azul
          {
            digitalWrite(RGB_R, LOW);
            digitalWrite(RGB_G, LOW);
            digitalWrite(RGB_B, HIGH);
            delay(8000);
            break;
          }

        case 5:  // Verde
          {
            digitalWrite(RGB_R, HIGH);
            digitalWrite(RGB_G, LOW);
            digitalWrite(RGB_B, HIGH);
            delay(8000);
            break;
          }

        case 6:
          {
            sendJSON.clear();  // Limpia cualquier dato previo
            sendJSON["Result"] = "OK";
            sendJSON["uart"] = "OK";
            display.clearDisplay();

            // ==== Validación de I34 Divisor V en +In -In
            delay(200);
            DIVIn_estable = leerAnalogicoEstable(DIVIn, 200);
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
            DIVOut_estable = leerAnalogicoEstable(DIVOut, 200);
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
  }

  /* ======= BOTÓN (FLANCO) ======= */
  bool currentState = digitalRead(WAKEUP);

  if (lastWakeupState == HIGH && currentState == LOW) {
    Serial.println("Botón presionado SW2 WakeUp");

    contador++;  // 🔥 incrementa contador
    buttonMode = true;
    buttonTime = millis();

    // ---- Mostrar contador en pantalla ----
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Conteo SW2:");
    display.setTextSize(2);
    display.setCursor(0, 16);
    display.println(contador);
    display.setTextSize(1);
    display.display();
  }

  lastWakeupState = currentState;

  /* ======= CONTROL DE LED ======= */
  if (buttonMode) {
    demoBUTTON();

    if (millis() - buttonTime > 1000) {
      buttonMode = false;
    }
  } else {
    demoLED();
  }
}

// =========================================================
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
  digitalWrite(RGB_R, HIGH);
  digitalWrite(RGB_G, HIGH);
  digitalWrite(RGB_B, HIGH);
}

// =========================================================
bool leerAnalogicoEstable(uint8_t GPIO, int umbral) {
  long suma = 0;
  int val;

  delay(50);
  int referencia = analogRead(GPIO);

  for (int i = 0; i < 10; i++) {
    val = analogRead(GPIO);
    Serial.println("Lectura: " + String(val));
    suma += val;
    delay(50);
  }

  int promedio = suma / 10;

  Serial.println("Promedio: " + String(promedio));
  Serial.println("");

  return abs(promedio - referencia) <= umbral;
}



void writeScreen(uint8_t line, String lbl, String msg) {
  int y = line * 8;  // 8 px por línea

  display.setCursor(0, y);
  display.print(lbl);

  display.setCursor(70, y);
  display.print(msg);

  display.display();
}