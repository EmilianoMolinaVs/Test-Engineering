/* 
===== CÓDIGO UNIT DUAL ONE JSON Integración ESP32 ====
*/

#include <Wire.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include "ue_i2c_icp_10111_sen.h"

#define RELAY 4     // GPIO de activación de relay para Carga Variable
#define RX2 16      // RX Serial 2 para puente con RP2040
#define TX2 17      // TX Serial 2 para puente con RP2040
#define PIN_19 19   // Salida de trama para GPIO23
#define PIN_23 23   // Entrada de trama desde GPIO19
#define PIN_5 5     // Salida de trama para GPIO18
#define PIN_18 18   // Entradad de trama desde GPIO5
#define SDA_PIN 21  // SDA Pines de lectura para sensor ICP
#define SCL_PIN 22  // SCL Pines de lectura para sensor ICP
#define LED_R 25    // Rojo RGB en DualOne
#define LED_G 26    // Verd RGB en DualOne
#define LED_B 27    // Azul RGB en DualOne

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

  // ==== Declaración de entradas y salidas ====
  pinMode(LED_R, OUTPUT);   // LED RGB Rojo Activo a BAJA
  pinMode(LED_G, OUTPUT);   // LED RGB Rojo Activo a BAJA
  pinMode(LED_B, OUTPUT);   // LED RGB Rojo Activo a BAJA
  pinMode(RELAY, OUTPUT);   // Pin de Relay GPIO4 para control de Carga Variable
  pinMode(PIN_19, OUTPUT);  // Salida de trama para GPIO23
  pinMode(PIN_5, OUTPUT);   // Salida de trama para GPIO18

  pinMode(PIN_23, INPUT_PULLUP);  // Entrada de trama desde GPIO19
  pinMode(PIN_18, INPUT);  // Entrada de trama desde GPIO5

  // ==== Declaración de Estados Iniciales ====
  digitalWrite(RELAY, HIGH);
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
      else if (Function == "ping") opc = 4;        // {"Function":"ping"}

      // Claves para Test de ESP32
      else if (Function == "testESP32") opc = 7;  // {"Function":"testESP32"}
      else if (Function == "relayOn") opc = 8;    // {"Function":"relayOn"}
      else if (Function == "relayOff") opc = 9;   // {"Function":"relayOff"}



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

        case 4:  // Clave de ping para validación de UART con RP2040
          {
            sendJSON["Function"] = "ping";
            serializeJson(sendJSON, Bridge);
            Bridge.println();
            break;
          }


        case 7:  // Clave de validación de GPIOs de ESP32 Main
          {
            sendJSON.clear();

            // ==== Validación de GPIOs 21 y 22 por I2C para Sensor ICP
            String stateICP = sensorICP();
            sendJSON["I21"] = stateICP;
            sendJSON["I22"] = stateICP;

            // ==== Validación de GPIOs 19 y 23
            String stateA = sequenceDIG(PIN_23, PIN_19);
            sendJSON["I19"] = stateA;
            sendJSON["I23"] = stateA;
            delay(1000);

            // ==== Validación de GPIOs 18 y 5
            String stateB = sequenceDIG(PIN_18, PIN_5);
            sendJSON["I18"] = stateB;
            sendJSON["I5"] = stateB;
            delay(150);

            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }

        case 8:  // Clave para accionamiento de relay para Carga Variable
          {
            digitalWrite(RELAY, LOW);
            break;
          }

        case 9:  // Clave para apagado de relay de Carga Variable
          {
            digitalWrite(RELAY, HIGH);
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

// Función de sensador con ICP101
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
  //Serial.println(avgTemp);

  if (avgTemp < 40 && avgTemp > 10) {
    return "OK";
  } else {
    return "FAIL";
  }
}

// Función de escritura y lectura de secuencia en pines digitales
String sequenceDIG(uint8_t GpioIn, uint8_t GpioOut) {
  bool debug = false;
  byte testSequence = 0b10101100;

  //Serial.printf("\n--- Test %d -> %d ---\n", GpioOut, GpioIn);

  for (int i = 7; i >= 0; i--) {
    int bitToSend = (testSequence >> i) & 1;

    digitalWrite(GpioOut, bitToSend);
    delay(10);  // Pequeño delay para estabilidad

    int bitRead = digitalRead(GpioIn);

    if (debug) {
      Serial.printf("Bit %d: Envié %d -> Leí %d", i, bitToSend, bitRead);
    };



    if (bitRead != bitToSend) {
      return "FAIL";
    }
  }
  return "OK";
}