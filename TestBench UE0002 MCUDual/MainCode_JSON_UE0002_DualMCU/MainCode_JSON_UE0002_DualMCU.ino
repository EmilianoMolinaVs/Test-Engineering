/* 
===== CÓDIGO DE INTEGRACIÓN CONTROL UE0002 DualMCU ====
Puente para control de relevadores de alimentación de cargador de baterías con super capacitor y demanda de carga variable
Es un código unicamente de control de potencia, la comunicación se da mediante el arnés 

*/

// --- BIBLIOTECAS ---
#include <Wire.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include <Arduino.h>
#include <Adafruit_INA219.h>

// ==== Declaración de pines
#define RX2 4         // GPIO04 como RXD
#define TX2 5         // GPIO05 como TXD
#define RUN_BUTTON 2  // Botón de Arranque
#define RELAYCom 20   // Relevador de puerto USB C
#define I2C_SDA 6     // SDA Sensor de Corriente
#define I2C_SCL 7     // SCL  Sensor de Corriente
#define RELAYA 8      // Relevador de Fuente de Alimentación
#define RELAYB 9      // Relevador de Fuente de Alimentación

// ==== Inicialización de objetos
HardwareSerial USB(1);             // Objeto para UART2 en PULSAR como PagWeb
TwoWire I2CBus = TwoWire(0);       // Comunicación I2C para Sensor de Corriente
Adafruit_INA219 ina219_in(0x40);   // Dirección de Sensor de Entrada
Adafruit_INA219 ina219_out(0x41);  // Dirección de Sensor de Salida

// ==== Variables de inicialización
String JSON_entrada;  // Variable que recibe al JSON en crudo de PagWeb
StaticJsonDocument<256> receiveJSON;

String JSON_lectura;  // Variable que envía el JSON de datos
StaticJsonDocument<256> sendJSON;


// ==== Función Medición de Corriente con INA219 ====
// Se emplea el sensor de entrada del TestBench
float medirCorriente() {

  const float shuntOffset_mV = 0.0;  // Offset en vacío para lectura inicial
  const float R_SHUNT = 0.05;        // Resistencia Shunt = 50 mΩ

  float shunt_mV = ina219_in.getShuntVoltage_mV();
  float bus_V = ina219_in.getBusVoltage_V();

  shunt_mV -= shuntOffset_mV;
  float shunt_V = shunt_mV / 1000.0;
  float current_A = shunt_V / R_SHUNT;  // Corriente de interés
  float load_V = bus_V + shunt_V;
  float power_W = load_V * current_A;

  float current_mA = current_A * 1000;

  return current_mA;
}


void setup() {

  Serial.begin(115200);                     // Serial de enlace entre TestBench y PagWeb
  USB.begin(115200, SERIAL_8N1, RX2, TX2);  // Bus de comunicación con el UART de la DualMCU

  // Iniciar comunicación I2C
  I2CBus.begin(I2C_SDA, I2C_SCL);
  if (!ina219_in.begin(&I2CBus)) {
    Serial.println("INA219 no encontrado");
  }

  pinMode(RELAYA, OUTPUT);     // Relay de Fuente de Alimentación
  pinMode(RELAYB, OUTPUT);     // Relay de Fuente de Alimentación
  pinMode(RELAYCom, OUTPUT);   // Relay de CH340 para Alimentación por USBC
  pinMode(RUN_BUTTON, INPUT);  // Botonera de Arranque

  digitalWrite(RELAYCom, LOW);  // Estado inicial: Cargador alimentando DualMCU
  digitalWrite(RELAYA, LOW);
  digitalWrite(RELAYB, LOW);
}

void loop() {

  if (digitalRead(RUN_BUTTON) == HIGH) {
    delay(100);

    if (digitalRead(RUN_BUTTON) == LOW) {
      sendJSON.clear();
      sendJSON["Run"] = "OK";           // Envio de corriente JSON para corto
      serializeJson(sendJSON, Serial);  // Envío de datos por JSON a la PagWeb
      Serial.println();
    }
  }

  if (Serial.available()) {  // Instrucciones JSON desde PagWeb

    JSON_entrada = Serial.readStringUntil('\n');                              // Leer hasta newline (JSON en crudo)
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);  // Deserializa el JSON y guarda la información en datosJSON

    if (!error) {

      String Function = receiveJSON["Function"];

      int opc = 0;
      // ==== Operaciones de validación de ESP32 ====
      if (Function == "ping") opc = 1;             // {"Function":"ping"}
      else if (Function == "mac") opc = 2;         // {"Function":"mac"}
      else if (Function == "bme") opc = 3;         // {"Function":"bme"}
      else if (Function == "test_esp32") opc = 4;  // {"Function":"test_esp32"}
      else if (Function == "vn_sensor") opc = 5;   // {"Function":"vn_sensor"}
      // ==== Operaciones de validación de RP2040 ====
      else if (Function == "passthrough") opc = 6;  // Comando passthrough: {"Function":"passthrough"}
      else if (Function == "hmotor") opc = 7;       // Comando haptic motor: {"Function":"hmotor"}
      else if (Function == "analog_rp") opc = 8;    // Comando analógicos RP: {"Function":"analog_rp"}
      else if (Function == "test_rp") opc = 9;      // Comando Test Rp2040: {"Function":"test_rp"}
      // ==== Operaciones de control con TestBench ====
      else if (Function == "VCC_ON") opc = 10;     // Comando TestBench: {"Function":"VCC_ON"}
      else if (Function == "VCC_OFF") opc = 11;    // Comando TestBench: {"Function":"VCC_OFF"}
      else if (Function == "meas_curr") opc = 12;  // Comando TestBench: {"Function":"meas_curr"}


      switch (opc) {
        case 1:
          {
            sendJSON.clear();
            sendJSON["Function"] = "ping";
            serializeJson(sendJSON, USB);
            USB.println();
            break;
          }

        case 2:
          {
            sendJSON.clear();
            sendJSON["Function"] = "mac";
            serializeJson(sendJSON, USB);
            USB.println();
            break;
          }

        case 3:
          {
            sendJSON.clear();
            sendJSON["Function"] = "bme";
            serializeJson(sendJSON, USB);
            USB.println();
            break;
          }

        case 4:
          {
            sendJSON.clear();
            sendJSON["Function"] = "test_esp32";
            serializeJson(sendJSON, USB);
            USB.println();
            break;
          }

        case 5:
          {
            sendJSON.clear();
            sendJSON["Function"] = "vn_sensor";
            serializeJson(sendJSON, USB);
            USB.println();
            break;
          }

        case 6:
          {
            sendJSON.clear();
            sendJSON["Function"] = "passthrough";
            serializeJson(sendJSON, USB);
            USB.println();
            break;
          }

        case 7:
          {
            sendJSON.clear();
            sendJSON["Function"] = "hmotor";
            serializeJson(sendJSON, USB);
            USB.println();
            break;
          }

        case 8:
          {
            sendJSON.clear();
            sendJSON["Function"] = "analog_rp";
            serializeJson(sendJSON, USB);
            USB.println();
            break;
          }

        case 9:
          {
            sendJSON.clear();
            sendJSON["Function"] = "test_rp";
            serializeJson(sendJSON, USB);
            USB.println();
            break;
          }

        case 10:
          {
            digitalWrite(RELAYCom, HIGH);
            delay(50);
            break;
          }

        case 11:
          {
            digitalWrite(RELAYCom, LOW);
            delay(50);
            break;
          }


        case 12:
          {
            delay(50);
            sendJSON.clear();
            float meas = 0;
            int muestras = 10;
            // Bucle de medición de valores
            for (int i = 0; i < muestras; i++) {
              float corriente = medirCorriente() + 80;
              meas += corriente;
              //Serial.println("Medición: " + String(corriente));
              delay(20);
            }
            float avg_current = meas / muestras;

            sendJSON["current"] = avg_current;
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }
      }
    }
  }

  if (USB.available()) {

    String incoming = USB.readStringUntil('\n');  // leer línea completa

    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, incoming);

    if (!error) {
      // ✅ Es JSON válido → lo reenvías limpio
      serializeJson(doc, Serial);
      Serial.println();
    }

    // ❌ Si no es JSON, lo ignoras (no imprime nada)
  }
}