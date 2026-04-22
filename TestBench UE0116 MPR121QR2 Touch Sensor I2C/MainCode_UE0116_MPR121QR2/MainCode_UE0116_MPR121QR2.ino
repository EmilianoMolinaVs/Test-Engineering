#include <Wire.h>
#include <HardwareSerial.h>
#include <Arduino.h>
#include <ArduinoJson.h>

// ==== CONFIGURACIÓN DE PINES ====
#define SDA_PIN 6
#define SCL_PIN 7
#define RX2 15  // GPIO15 como RX para PagWeb
#define TX2 19  // GPIO19 como TX para PagWeb

#define ELECT4_PIN 20  // GPIO20 para lectura de Electrodo 4 en Módulo
#define ELECT5_PIN 21  // GPIO21 para lectura de Electrodo 5 en Módulo
#define ELECT6_PIN 18  // GPIO18 para lectura de Electrodo 6 en Módulo
#define ELECT7_PIN 2   // GPIO2 para lectura de Electrodo 7 en Módulo
#define ELECT8_PIN 14  // GPIO14 para lectura de Electrodo 8 en Módulo
#define ELECT9_PIN 0   // GPIO0 para lectura de Electrodo 9 en Módulo
#define ELECTA_PIN 1   // GPIO1 para lectura de Electrodo A en Módulo
#define ELECTB_PIN 3   // GPIO3 para lectura de Electrodo B en Módulo

static const uint8_t electrPorts[] = { ELECT4_PIN, ELECT5_PIN, ELECT6_PIN, ELECT7_PIN,
                                       ELECT8_PIN, ELECT9_PIN, ELECTA_PIN, ELECTB_PIN };

// ==== Inicialización de objetos
HardwareSerial PagWeb(1);  // Objeto para UART2 en PULSAR como PagWeb

String JSON_entrada;                  ///< Buffer para recibir JSON desde PagWeb
StaticJsonDocument<256> receiveJSON;  ///< Documento JSON para parsear datos recibidos

String JSON_lectura;               ///< Buffer para transmitir JSON de respuesta
StaticJsonDocument<256> sendJSON;  ///< Documento JSON para armar respuestas

// ==== Registros del MPR121QR2 ====
static const uint8_t MPR_ADDR = 0x5A;
static const uint8_t REG_ELECTRODE_CONFIG = 0x5E;
static const uint8_t REG_GPIO_CONTROL_0 = 0x73;
static const uint8_t REG_GPIO_CONTROL_1 = 0x74;
static const uint8_t REG_GPIO_DIRECTION = 0x76;
static const uint8_t REG_GPIO_ENABLE = 0x77;
static const uint8_t REG_GPIO_SET = 0x78;
static const uint8_t REG_GPIO_CLEAR = 0x79;
static const uint8_t REG_GPIO_DATA = 0x75;

static const uint8_t GPIO_MASK_ALL = 0xFF;  // ELE4..ELE11
static const uint8_t SWEEP_ORDER[8] = { 4, 5, 6, 7, 8, 9, 10, 11 };

// ==== Funciones de Propósito General ====
void writeReg(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(MPR_ADDR);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
  delayMicroseconds(100);
}

uint8_t readReg(uint8_t reg) {
  Wire.beginTransmission(MPR_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);

  Wire.requestFrom(MPR_ADDR, (uint8_t)1);
  if (Wire.available() < 1) {
    return 0;
  }
  return Wire.read();
}

void setOutputsRaw(uint8_t rawMask) {
  writeReg(REG_GPIO_CLEAR, GPIO_MASK_ALL);
  writeReg(REG_GPIO_SET, rawMask);
}

void initGpioOnly() {
  // Stop touch engine so all electrode channels remain free for GPIO.
  writeReg(REG_ELECTRODE_CONFIG, 0x00);
  delay(10);

  writeReg(REG_GPIO_CONTROL_0, 0x00);           // GPIO mode
  writeReg(REG_GPIO_CONTROL_1, 0x00);           // no pull-up
  writeReg(REG_GPIO_DIRECTION, GPIO_MASK_ALL);  // 1 = output
  writeReg(REG_GPIO_ENABLE, GPIO_MASK_ALL);     // enable ELE4..ELE11

  setOutputsRaw(0x00);
}

void serialDebug(String str) {
  str.replace("\"", "\\\"");  // Escapa comillas
  Serial.println("{\"debug\": \"" + str + "\"}");
}

void pagwebDebug(String str) {
  str.replace("\"", "\\\"");  // Escapa comillas
  PagWeb.println("{\"debug\": \"" + str + "\"}");
}

bool gpioStatus(uint8_t port) {
  delay(50);
  int sum = 0;

  for (int i = 0; i < 5; i++) {
    if (digitalRead(port) == HIGH) sum++;
    //Serial.println(sum);
  }

  if (sum == 5) return true;
  else return false;
}

void setup() {

  // ==== Inicialización del Monitor Serie ====
  Serial.begin(115200);
  delay(100);
  serialDebug("Serial initialized...");

  PagWeb.begin(115200, SERIAL_8N1, RX2, TX2);  // Iniciar UART2 en los pines seleccionados
  pagwebDebug("UART PagWeb Initialized...");

  // ==== Inicialización I2C y Registros MPR121QR2 ====
  Wire.begin(SDA_PIN, SCL_PIN);
  initGpioOnly();

  // ==== Declaración de GPIOS ====
  pinMode(ELECT4_PIN, INPUT);
  pinMode(ELECT5_PIN, INPUT);
  pinMode(ELECT6_PIN, INPUT);
  pinMode(ELECT7_PIN, INPUT);
  pinMode(ELECT8_PIN, INPUT);
  pinMode(ELECT9_PIN, INPUT);
  pinMode(ELECTA_PIN, INPUT);
  pinMode(ELECTB_PIN, INPUT);
}

void loop() {

  if (PagWeb.available()) {

    JSON_entrada = PagWeb.readStringUntil('\n');                              // Leer hasta newline (JSON en crudo)
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);  // Deserializa el JSON y guarda la información en datosJSON

    if (!error) {

      String Function = receiveJSON["Function"];  // Function es la variable de interés del JSON

      int opc = 0;
      if (Function == "ping") opc = 1;              // {"Function":"ping"}
      else if (Function == "init") opc = 2;         // {"Function":"init"}
      else if (Function == "digitalScan") opc = 3;  // {"Function":"digitalScan"}
      else if (Function == "B") opc = 4;            // {"Function":"init"}
      else if (Function == "restart") opc = 5;      // {"Function":"restart"}

      switch (opc) {
        case 1:
          {
            sendJSON.clear();
            sendJSON["ping"] = "pong";
            serializeJson(sendJSON, PagWeb);
            PagWeb.println();
            break;
          }

        case 2:
          {
            Wire.begin(SDA_PIN, SCL_PIN);
            initGpioOnly();
            pagwebDebug("GPIOS initilized...");
            break;
          }

        case 3:
          {
            sendJSON.clear();
            int total_values = 0;
            for (uint8_t i = 0; i < 8; ++i) {
              uint8_t electrode = SWEEP_ORDER[i];
              uint8_t mask = (uint8_t)(1u << (electrode - 4));

              setOutputsRaw(mask);  // Se fija la salida digital en el MPR121QR2
              delay(100);
              if (gpioStatus(electrPorts[i])) {
                sendJSON["port" + String(i)] = "OK";
                total_values++;
              } else {
                sendJSON["port" + String(i)] = "FAIL";
              }

              uint8_t data = readReg(REG_GPIO_DATA);
              Serial.print("ON ELE");
              Serial.print((int)electrode);
              Serial.print(" | GPIO_DATA=0x");
              Serial.println(data, HEX);
              delay(100);
            }

            if (total_values == 8) sendJSON["Result"] = "OK";
            serializeJson(sendJSON, PagWeb);
            PagWeb.println();
            break;
          }

        case 5:
          ESP.restart();
          break;

        default: break;
      }
    }
  }
}
