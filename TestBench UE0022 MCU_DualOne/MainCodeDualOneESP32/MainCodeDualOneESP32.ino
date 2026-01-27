/* 
===== CÓDIGO UNIT DUAL ONE JSON Integración ESP32 ====
*/

#include <Wire.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include "ue_i2c_icp_10111_sen.h"

#define RX2 16
#define TX2 17

#define SDA_PIN 21  // Pines de lectura para sensor ICP
#define SCL_PIN 22

#define LED_R 25
#define LED_G 26
#define LED_B 27


// ==== Declaración de objetos
HardwareSerial Bridge(1);
ICP101xx sensor;

// ==== Declaración de variables
String JSON_entrada;
StaticJsonDocument<200> receiveJSON;

String JSON_salida;  // Variable que envía el JSON de datos
StaticJsonDocument<200> sendJSON;

void setup() {
  Serial.begin(115200);
  Serial.println("UART0 listo para comunicación...");

  Bridge.begin(115200, SERIAL_8N1, RX2, TX2);

  Wire.begin(SDA_PIN, SCL_PIN);
  if (!sensor.begin(&Wire)) {
    Serial.println("ERROR: Could not initialize sensor!");
    Serial.println("Check I2C wiring and connections.");
  }

  pinMode(LED_R, OUTPUT);  // Activos a BAJA
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
}

void loop() {

  // Condicional para enviar JSON a RP2040 target
  if (Serial.available()) {  // JSON de salida hacia RP2040

    JSON_entrada = Serial.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);

    if (!error) {
      String Function = receiveJSON["Function"];

      int opc = 0;
      // Claves para Test de RP2040
      if (Function == "testRP2040") opc = 1;       // {"Function":"testRP2040"}
      else if (Function == "testBuzzer") opc = 2;  // {"Function":"testBuzzer"}
      else if (Function == "testLEDs") opc = 3;    // {"Function":"testLEDs"}


      // Claves para Test de ESP32
      else if (Function == "testESP32") opc = 9;

      switch (opc) {
        case 1:  // Case de testAll de RP2040
          {
            sendJSON.clear();
            sendJSON["Function"] = "testAll";
            serializeJson(sendJSON, Bridge);
            Bridge.println();
            break;
          }

        case 2:  // Clave de test de buzzer en RP2040
          {
            sendJSON.clear();
            sendJSON["Function"] = "buzzer";
            serializeJson(sendJSON, Bridge);
            Bridge.println();
            break;
          }

        case 3:  // Clave de test de LEDs en RP2040
          {
            sendJSON.clear();
            sendJSON["Function"] = "leds";
            serializeJson(sendJSON, Bridge);
            Bridge.println();
            break;
          }




        case 9:
          {
            String state = sensorICP();
            Serial.println("Sensor ICP: " + state);
            break;
          }
      }
    }
  }

  // Condicional para recibir resultados de Test de RP2040
  if (Bridge.available()) {  // JSON de entrada desde RP2040
    JSON_entrada = Bridge.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);
    Serial.println(JSON_entrada);
  }

  // Condicional de Demo
  if (!Serial.available() && !Bridge.available()) {
    demoLED();
  }
}

/*
{"Function":"meas"}
{"Function":"buzzer"}
*/

void demoLED() {
  digitalWrite(LED_R, LOW);
  digitalWrite(LED_G, HIGH);
  digitalWrite(LED_B, HIGH);

  delay(500);
  digitalWrite(LED_R, HIGH);
  digitalWrite(LED_G, LOW);
  digitalWrite(LED_B, HIGH);

  delay(500);
  digitalWrite(LED_R, HIGH);
  digitalWrite(LED_G, HIGH);
  digitalWrite(LED_B, LOW);

  delay(500);
}

String sensorICP() {
  sensor.measure(sensor.NORMAL);

  float avgTemp = 0;
  float avgPress = 0;

  for (int i = 0; i < 10; i++) {
    float temperature = sensor.getTemperatureC();
    float pressure = sensor.getPressurePa();

    avgTemp += temperature;
    avgPress += pressure;
    delay(100);
  }

  avgTemp = avgTemp / 10;
  Serial.println(avgTemp);

  if (avgTemp < 40 && avgTemp > 10) {
    return "OK";
  } else {
    return "FAIL";
  }
}