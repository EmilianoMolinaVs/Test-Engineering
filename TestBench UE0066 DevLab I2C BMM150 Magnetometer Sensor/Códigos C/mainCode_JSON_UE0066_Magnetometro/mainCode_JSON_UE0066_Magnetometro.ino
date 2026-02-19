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

DFRobot_BMM150_I2C *bmm150_i2c = nullptr;
DFRobot_BMM150_SPI *bmm150_spi = nullptr;


// Variables de JSON
String JSON_entrada;  // Variable que recibe al JSON en crudo de PagWeb
StaticJsonDocument<200> receiveJSON;

String JSON_salida;  // Variable que envía el JSON de datos
StaticJsonDocument<200> sendJSON;

bool bmm150_initialized_i2c = false;
bool bmm150_initialized_spi = false;

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
      else if (Function == "spi_init") opc = 3;  // {"Function":"spi_init"}
      else if (Function == "mag_read") opc = 4;  // {"Function":"mag_read"}


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
            bmm150_initialized_i2c = false;

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
            if (bmm150_i2c != nullptr) {
              delete bmm150_i2c;
            }

            bmm150_i2c = new DFRobot_BMM150_I2C(&Wire, addr_scan);

            if (bmm150_i2c->begin()) {
              sendJSON["status"] = "FAIL";
              sendJSON["msg"] = "Init failed";
              serializeJson(sendJSON, PagWeb);
              PagWeb.println();
              break;
            }

            bmm150_i2c->setOperationMode(BMM150_POWERMODE_NORMAL);
            bmm150_i2c->setPresetMode(BMM150_PRESETMODE_HIGHACCURACY);
            bmm150_i2c->setRate(BMM150_DATA_RATE_10HZ);
            bmm150_i2c->setMeasurementXYZ();

            bmm150_initialized_i2c = true;

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
            bmm150_initialized_spi = false;

            pinMode(CSS_PIN, OUTPUT);
            digitalWrite(PSS_PIN, LOW);  // PS HIGH -> I2C | PS LOW -> SPI
            digitalWrite(CSS_PIN, LOW);

            if (bmm150_spi != nullptr) {
              delete bmm150_spi;
            }

            SPI.begin(SCL_PIN, SDO_PIN, SDA_PIN, CSS_PIN);

            bmm150_spi = new DFRobot_BMM150_SPI(CSS_PIN, &SPI);

            if (bmm150_spi->begin()) {
              sendJSON["status"] = "FAIL";
              sendJSON["msg"] = "SPI init failed";
              serializeJson(sendJSON, PagWeb);
              PagWeb.println();
              return;
            }

            bmm150_initialized_spi = true;

            bmm150_spi->setOperationMode(BMM150_POWERMODE_NORMAL);
            bmm150_spi->setPresetMode(BMM150_PRESETMODE_HIGHACCURACY);
            bmm150_spi->setRate(BMM150_DATA_RATE_30HZ);
            bmm150_spi->setMeasurementXYZ();

            Serial.println(bmm150_spi->selfTest(BMM150_SELF_TEST_NORMAL));
            Serial.print("rate is ");
            Serial.println(bmm150_spi->getRate());

            bmm150_spi->softReset();

            bmm150_spi->setOperationMode(BMM150_POWERMODE_NORMAL);
            bmm150_spi->setPresetMode(BMM150_PRESETMODE_HIGHACCURACY);
            bmm150_spi->setRate(BMM150_DATA_RATE_30HZ);
            bmm150_spi->setMeasurementXYZ();


            break;
          }

        case 4:
          {
            sendJSON.clear();

            if ((bmm150_initialized_i2c && bmm150_i2c == nullptr)
                || (bmm150_initialized_spi && bmm150_spi == nullptr)
                || (!bmm150_initialized_i2c && !bmm150_initialized_spi)) {

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
  sBmm150MagData_t magData;

  if (bmm150_initialized_i2c) {
    magData = bmm150_i2c->getGeomagneticData();
  } else if (bmm150_initialized_spi) {
    magData = bmm150_spi->getGeomagneticData();
  }

  sendJSON.clear();
  sendJSON["x_uT"] = magData.x;
  sendJSON["y_uT"] = magData.y;
  sendJSON["z_uT"] = magData.z;
  //sendJSON["compass_deg"] = bmm150->getCompassDegree();

  serializeJson(sendJSON, PagWeb);
  PagWeb.println();
}
