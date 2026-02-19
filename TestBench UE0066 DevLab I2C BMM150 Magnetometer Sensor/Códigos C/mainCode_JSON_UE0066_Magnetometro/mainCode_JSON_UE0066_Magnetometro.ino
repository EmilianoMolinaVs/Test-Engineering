/* ==== MainCode UE0066 BMM150 Magnetometro ==== 

Manejo  de protocolos I2C 4-Addr y SPI vinculado a PagWeb

*/


#include <Wire.h>
#include <SPI.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HardwareSerial.h>
#include "DFRobot_BMM150.h"


// ==== Declaración de pines ====

#define RUN_BUTTON 4  // Botón de Arranque

#define RX2 D4  // GPIO15 como RX UART Pagweb
#define TX2 D5  // GPIO19 como TX

#define SDA_PIN 6   // I2C -> Datos   | SPI -> CLK
#define SCL_PIN 7   // I2C -> CLK     | SPI -> MOSI
#define SDO_PIN 2   // I2C -> Address | SPI -> MISO
#define CSS_PIN 18  // I2C -> Address | SPI -> Chip Select
#define PSS_PIN 3   // Selector de protocolo


// ==== Inicialización de objetos ====
HardwareSerial PagWeb(1);  // Objeto para UART2 en PULSAR como PagWeb
DFRobot_BMM150_I2C *bmm150 = nullptr;


// Variables de JSON
String JSON_entrada;  // Variable que recibe al JSON en crudo de PagWeb
StaticJsonDocument<200> receiveJSON;

String JSON_salida;  // Variable que envía el JSON de datos
StaticJsonDocument<200> sendJSON;

bool bmm150_initialized = false;
uint8_t bmm150_addr = 0;

void setup() {

  // ==== Inicialización de UART ====
  Serial.begin(115200);
  delay(100);
  PagWeb.begin(115200, SERIAL_8N1, RX2, TX2);
  Serial.println("Serial inicializado...");

  // ==== Declaración de Entradas | Salidas
  pinMode(RUN_BUTTON, INPUT);
  pinMode(PSS_PIN, OUTPUT);
  pinMode(SDO_PIN, OUTPUT);
  pinMode(CSS_PIN, OUTPUT);

  digitalWrite(PSS_PIN, HIGH);  // PS HIGH -> I2C | PS LOW -> SPI
  digitalWrite(SDO_PIN, HIGH);
  digitalWrite(CSS_PIN, HIGH);

  /*
  SDO | CSS | Address
  0       0     0x10
  0       1     0x12 
  1       0     0x11
  1       1     0x13
  */
}


void loop() {

  if (digitalRead(RUN_BUTTON) == HIGH) {
    delay(200);
    sendJSON.clear();  // Limpia cualquier dato previo

    if (digitalRead(RUN_BUTTON) == LOW) {
      Serial.println("Arranque por botonera");
      sendJSON["Run"] = "OK";           // Envio de corriente JSON para corto
      serializeJson(sendJSON, PagWeb);  // Envío de datos por JSON a la PagWeb
      PagWeb.println();
    }
  }


  if (PagWeb.available()) {

    JSON_entrada = PagWeb.readStringUntil('\n');                              // Leer hasta newline (JSON en crudo)
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);  // Deserializa el JSON y guarda la información en datosJSON

    if (!error) {
      String Function = receiveJSON["Function"];
      String Address = receiveJSON["Address"];

      int opc = 0;

      if (Function == "ping") opc = 1;           // {"Function":"ping"}
      else if (Function == "i2c_init") opc = 2;  // {"Function":"i2c_init", "Address": "0x10"}
      else if (Function == "i2c_read") opc = 3;  // {"Function":"i2c_read"}


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
            sendJSON.clear();

            uint8_t addr = (uint8_t)strtol(Address.c_str(), NULL, 16);

            switch (addr) {
              case 0x10:
                digitalWrite(SDO_PIN, LOW);
                digitalWrite(CSS_PIN, LOW);
                break;

              case 0x11:
                digitalWrite(SDO_PIN, HIGH);
                digitalWrite(CSS_PIN, LOW);
                break;

              case 0x12:
                digitalWrite(SDO_PIN, LOW);
                digitalWrite(CSS_PIN, HIGH);
                break;

              case 0x13:
                digitalWrite(SDO_PIN, HIGH);
                digitalWrite(CSS_PIN, HIGH);
                break;


              default:
                sendJSON["status"] = "FAIL";
                sendJSON["msg"] = "Invalid address";
                serializeJson(sendJSON, PagWeb);
                PagWeb.println();
                return;
            }

            Wire.begin(SDA_PIN, SCL_PIN);

            uint8_t addr_scan = detectBMM150Address();

            if (addr_scan == 0) {
              sendJSON.clear();
              sendJSON["error"] = "BMM150 not detected";
              serializeJson(sendJSON, PagWeb);
              PagWeb.println();
              return;
            }


            // Liberar si ya existía
            if (bmm150 != nullptr) {
              delete bmm150;
            }

            bmm150 = new DFRobot_BMM150_I2C(&Wire, addr_scan);

            if (bmm150->begin()) {
              sendJSON["status"] = "FAIL";
              sendJSON["msg"] = "Init failed";
              serializeJson(sendJSON, PagWeb);
              PagWeb.println();
              break;
            }

            bmm150->setOperationMode(BMM150_POWERMODE_NORMAL);
            bmm150->setPresetMode(BMM150_PRESETMODE_HIGHACCURACY);
            bmm150->setRate(BMM150_DATA_RATE_10HZ);
            bmm150->setMeasurementXYZ();

            bmm150_initialized = true;

            char addrStr[6];
            sprintf(addrStr, "0x%02X", addr_scan);

            sendJSON["status"] = "OK";
            sendJSON["addr"] = addrStr;
            serializeJson(sendJSON, PagWeb);
            PagWeb.println();
            break;
          }



        case 3:
          {
            sendJSON.clear();

            if (!bmm150_initialized || bmm150 == nullptr) {
              sendJSON["status"] = "FAIL";
              sendJSON["msg"] = "Sensor not initialized";
              serializeJson(sendJSON, PagWeb);
              PagWeb.println();
              break;
            }

            for (int i = 0; i < 10; i++) {
              readMagnetometer();
              delay(50);
            }

            break;
          }



        default:
          Serial.println("Opción no válida");
          break;
      }
    }
  }
}


uint8_t detectBMM150Address() {
  uint8_t addresses[] = { 0x10, 0x11, 0x12, 0x13 };

  for (uint8_t i = 0; i < 4; i++) {
    Wire.beginTransmission(addresses[i]);
    if (Wire.endTransmission() == 0) {
      return addresses[i];
    }
  }
  return 0;
}


void readMagnetometer() {
  sBmm150MagData_t magData = bmm150->getGeomagneticData();

  sendJSON.clear();
  sendJSON["x_uT"] = magData.x;
  sendJSON["y_uT"] = magData.y;
  sendJSON["z_uT"] = magData.z;
  sendJSON["compass_deg"] = bmm150->getCompassDegree();

  serializeJson(sendJSON, PagWeb);
  PagWeb.println();
}
