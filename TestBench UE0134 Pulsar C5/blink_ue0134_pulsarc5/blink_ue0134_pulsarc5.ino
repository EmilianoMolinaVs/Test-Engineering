/*
Firmware test v1.0 Pulsar C5 

Funcionalidades: 

- Control de blink y neopixel 
- Lectura de direcciones I2C y manejo de sensores nativos 
  . MAX17048 Addr 0x36      | Charger Battery Sensor
  . MMC5603 Addr 0x30       | Magnetometer Sensor 
  . SHT40 Addr 0x44         | Humidity and Temperature Sensor
  . LPS22DF Addr 0x5C       | Pressure and Temperature Sensor
  . VCNL4040 Addr 0x60      | Light Sensor 

{"I2C_Devices":["0x30","0x36","0x44","0x5C","0x60"],"Result":"OK"}

      {"Function":"ping"}
      {"Function":"mac"}
      {"Function":"i2c_scan"}
      {"Function":"mgn"}
      {"Function":"bat"}
      {"Function":"press"}
      {"Function":"prox"}
      {"Function":"sd"}
      {"Function":"wifi"}
      {"Function":"gpios"}
      {"Function":"restart"}
-
*/

// ==== BIBLIOTECAS ====
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_MMC56x3.h>
#include "SparkFun_VCNL4040_Arduino_Library.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"

// ==== DECLARACIÓN DE GPIOS ====
#define RX2_PIN 12   // >> GPIO RX de Serial 0
#define TX2_PIN 11   // >> GPIO TX de Serial 0
#define NEOP_PIN 0   // >> GPIO Entrada de Neopixel
#define LED_PIN 27   // >> GPIO LED BUILT IN (¡Comparte pin con SCK_PIN!)
#define SDA_PIN 2    // >> PIN SDA Bus I2C 1 (LP)
#define SCL_PIN 3    // >> PIN SCL Bus I2C 1 (LP)
#define SDA2_PIN 1   // >> PIN SDA Segundo Bus I2C
#define SCL2_PIN 6   // >> PIN SCL Segundo Bus I2C
#define MISO_PIN 26  // >> PIN MISO en SPI bus para SD CARD
#define MOSI_PIN 7   // >> PIN MOSI en SPI bus para SD CARD
#define SCK_PIN 27   // >> PIN SCK en SPI bus para SD CARD
#define CS_PIN 23    // >> PIN CS en SPI bus para SD CARD
#define PIN_9 9      // >> PIN 9 GPIO Digital | Pareja [9 - 24]
#define PIN_24 24    // >> PIN 24 GPIO Digital
#define PIN_4 4      // >> PIN 4 GPIO Digital | Pareja [4 - 5]
#define PIN_5 5      // >> PIN 5 GPIO Digital


// ==== MACROS Y VARIABLES GLOBALES ====
#define NUMPIXELS 10  // Número de Neopixeles
int intensity = 10;   // Intensidad de brillo en Neopixel
int delay_neop = 20;  // Tiempo de actualización en Neopixel

String JSON_entrada;  // Variable que recibe JSON del frontend
String JSON_salida;   // Variable que envía el JSON de datos

#define MAX17048_ADDR 0x36  // Dirección de esclavo MAX17048
#define REG_VCELL 0x02
#define REG_SOC 0x04
#define REG_MODE 0x06  // Registro para comandos especiales

// ==== MACROS LPS22DF ====
#define LPS22DF_ADDR 0x5C  // Dirección en tu placa Pulsar C5
#define LPS_WHO_AM_I 0x0F
#define LPS_CTRL_REG1 0x10
#define LPS_PRESS_OUT 0x28
float temperature, pressure;

// ==== CREACIÓN DE OBJETOS ====
StaticJsonDocument<128> receiveJSON;
StaticJsonDocument<128> sendJSON;
Adafruit_NeoPixel pixels(NUMPIXELS, NEOP_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_MMC5603 mmc = Adafruit_MMC5603(12345);
VCNL4040 proximitySensor;

// ==== FUNCIONALIDADES ====
void demo() {
  static unsigned long previousMillis = 0;
  static int colorState = 0;
  static int pixelIndex = 0;

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= delay_neop) {
    previousMillis = currentMillis;

    // Nota: Se omite digitalWrite(LED_PIN) aquí para no destruir la asignación SPI SCK en GPIO 27
    switch (colorState) {
      case 0:  // Estado 0: Rojo
        pixels.setPixelColor(pixelIndex, pixels.Color(intensity, 0, 0));
        break;

      case 1:  // Estado 1: Verde
        pixels.setPixelColor(pixelIndex, pixels.Color(0, intensity, 0));
        break;

      case 2:  // Estado 2: Azul
        pixels.setPixelColor(pixelIndex, pixels.Color(0, 0, intensity));
        break;

      case 3:  // Estado 3: Cian/Custom
        pixels.setPixelColor(pixelIndex, pixels.Color(0, intensity, intensity));
        break;

      case 4:  // Estado 4: Limpieza y reinicio del ciclo
        pixels.clear();
        pixels.show();
        colorState = 0;
        pixelIndex = 0;
        return;
    }

    pixels.show();
    pixelIndex++;

    if (pixelIndex >= NUMPIXELS) {
      pixelIndex = 0;
      colorState++;
    }
  }
}

void mensajeBienvenida() {
  delay(1000);  // Pequeña pausa para que el usuario alcance a leerlo
  Serial.println("========================================================");
  Serial.println("*                                                      *");
  Serial.println("*    🚀 ¡HOLA! BIENVENIDO A TU PULSAR C5 🚀      *");
  Serial.println("*                                                      *");
  Serial.println("========================================================");
  Serial.println("*  Tu nueva placa de desarrollo esta lista para usar.  *");
  Serial.println("*  Esperamos que la disfrutes al máximo! :D            *");
  Serial.println("========================================================");
  delay(500);  // Pequeña pausa para que el usuario alcance a leerlo
  Serial.print("\n>> Iniciando proceso de hackeo de computadora");
  for (int i = 0; i < 10; i++) {
    Serial.print(".");
    delay(100);
  }
  Serial.print("\n>> Computadora hackeada");
  for (int i = 0; i < 3; i++) {
    Serial.print(".");
    delay(300);
  }
  Serial.print("\n>> Borrando archivos de sistema");
  for (int i = 0; i < 10; i++) {
    Serial.print(".");
    delay(100);
  }
  delay(1000);
  Serial.println("\n Bromita! XDD");
  Serial.println("¡Disfruta de tu nueva PULSAR C5 de 🚀 UNIT Electronics 🚀 :D!");
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial0.begin(115200, SERIAL_8N1, RX2_PIN, TX2_PIN);

  // Inicializamos el Bus 1 por defecto en GPIOs 2 y 3
  Wire.begin(SDA_PIN, SCL_PIN);

  // Configuramos CS con Pull-Up antes de iniciar SPI para evitar estados flotantes en la SD
  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);

  pixels.clear();
  pixels.setBrightness(150);

  mensajeBienvenida();
}

void loop() {

  Stream *activePort = nullptr;


  if (Serial.available() || Serial0.available()) {

    if (Serial.available()) {
      activePort = &Serial;
    } else if (Serial0.available()) {
      activePort = &Serial0;
    }

    JSON_entrada = activePort->readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);

    if (error) {
      Serial.println("Halo murió en Reach...");
    } else {
      int opc = 0;
      String Function = receiveJSON["Function"];

      if (Function == "ping") opc = 1;           // {"Function":"ping"}
      else if (Function == "mac") opc = 2;       // {"Function":"mac"}
      else if (Function == "i2c_scan") opc = 3;  // {"Function":"i2c_scan"}
      else if (Function == "mgn") opc = 4;       // {"Function":"mgn"}
      else if (Function == "bat") opc = 5;       // {"Function":"bat"}
      else if (Function == "press") opc = 6;     // {"Function":"press"}
      else if (Function == "prox") opc = 7;      // {"Function":"prox"}
      else if (Function == "sd") opc = 8;        // {"Function":"sd"}
      else if (Function == "wifi") opc = 9;      // {"Function":"wifi"}
      else if (Function == "gpios") opc = 10;    // {"Function":"gpios"}
      else if (Function == "restart") opc = 15;  // {"Function":"restart"}

      switch (opc) {
        case 1:
          {
            sendJSON.clear();
            sendJSON["ping"] = "pong";
            serializeJson(sendJSON, *activePort);
            activePort->println();
            break;
          }

        case 2:
          {
            sendJSON.clear();
            WiFi.mode(WIFI_MODE_STA);
            sendJSON["mac"] = WiFi.macAddress();
            serializeJson(sendJSON, *activePort);
            activePort->println();
            break;
          }

        case 3:  // ==== ESCÁNER I2C DUAL CON VALIDACIÓN ESTRICTA ====
          {
            sendJSON.clear();
            JsonArray devices_bus1 = sendJSON.createNestedArray("I2C_Devices");
            JsonArray devices_bus2 = sendJSON.createNestedArray("I2C_Bus2");

            // Lista obligatoria de sensores en Bus 1: 0x30, 0x36, 0x44, 0x5C, 0x60
            const uint8_t reqAddrs[] = { 0x30, 0x36, 0x44, 0x5C, 0x60 };
            const uint8_t totalReq = sizeof(reqAddrs) / sizeof(reqAddrs[0]);
            uint8_t matchedCount = 0;

            // 1. Escaneo Bus 1 (GPIO 2 y 3)
            Wire.end();
            Wire.begin(SDA_PIN, SCL_PIN);
            for (uint8_t addr = 1; addr < 127; addr++) {
              Wire.beginTransmission(addr);
              if (Wire.endTransmission() == 0) {
                char hexAddr[5];
                sprintf(hexAddr, "0x%02X", addr);
                devices_bus1.add(hexAddr);

                // Verificamos si la dirección encontrada está en la lista requerida
                for (uint8_t k = 0; k < totalReq; k++) {
                  if (addr == reqAddrs[k]) {
                    matchedCount++;
                    break;
                  }
                }
              }
            }

            // 2. Escaneo Bus 2 (GPIO 1 y 6) usando el controlador HP_I2C0 por matriz GPIO
            Wire.end();
            pinMode(SDA2_PIN, INPUT_PULLUP);
            pinMode(SCL2_PIN, INPUT_PULLUP);
            Wire.begin(SDA2_PIN, SCL2_PIN);

            for (uint8_t addr = 1; addr < 127; addr++) {
              Wire.beginTransmission(addr);
              if (Wire.endTransmission() == 0) {
                char hexAddr[5];
                sprintf(hexAddr, "0x%02X", addr);
                devices_bus2.add(hexAddr);
              }
            }

            // Restauramos el Bus 1 por defecto para el resto de los cases
            Wire.end();
            Wire.begin(SDA_PIN, SCL_PIN);

            // Solo enviamos "OK" si se encontraron exactamente los 5 sensores del Bus 1
            if (matchedCount == totalReq) {
              sendJSON["Result"] = "OK";
            } else {
              sendJSON["Result"] = "Error";
              sendJSON["Missing_Count"] = totalReq - matchedCount;
            }

            serializeJson(sendJSON, *activePort);
            activePort->println();
            break;
          }

        case 4:  // ==== MMC5603 ====
          {
            sendJSON.clear();
            if (!mmc.begin(MMC56X3_DEFAULT_ADDRESS, &Wire)) {
              sendJSON["Sensor"] = "MMC5603";
              sendJSON["Result"] = "Error";
              sendJSON["Msg"] = "Not found";
              serializeJson(sendJSON, *activePort);
              activePort->println();
              break;
            }

            mmc.setDataRate(100);
            mmc.setContinuousMode(true);

            for (int i = 0; i < 30; i++) {
              sensors_event_t event;
              mmc.getEvent(&event);

              sendJSON.clear();
              sendJSON["Sensor"] = "MMC5603";
              sendJSON["Result"] = "OK";
              sendJSON["X_uT"] = event.magnetic.x;
              sendJSON["Y_uT"] = event.magnetic.y;
              sendJSON["Z_uT"] = event.magnetic.z;
              serializeJson(sendJSON, *activePort);
              activePort->println();
              delay(200);
            }
            break;
          }

        case 5:  // ==== MAX17048 ====
          {
            sendJSON.clear();
            Wire.beginTransmission(MAX17048_ADDR);
            if (Wire.endTransmission() != 0) {
              sendJSON["Sensor"] = "MAX17048";
              sendJSON["Result"] = "Error";
              sendJSON["Msg"] = "Not found";
              serializeJson(sendJSON, *activePort);
              activePort->println();
              break;
            }

            quickStart();
            delay(500);
            float voltaje = leerVoltaje();
            float porcentaje = leerSOC();

            if (porcentaje > 100.0) {
              quickStart();
              delay(500);
              voltaje = leerVoltaje();
              porcentaje = leerSOC();
            }

            sendJSON.clear();
            sendJSON["Sensor"] = "MAX17048";
            sendJSON["Result"] = "OK";
            sendJSON["V"] = voltaje;
            sendJSON["SOC"] = porcentaje;
            serializeJson(sendJSON, *activePort);
            activePort->println();
            break;
          }

        case 6:  // ==== LPS22DF ====
          {
            sendJSON.clear();
            Wire.beginTransmission(LPS22DF_ADDR);
            Wire.write(LPS_WHO_AM_I);
            Wire.endTransmission(false);
            Wire.requestFrom((uint16_t)LPS22DF_ADDR, (uint8_t)1);

            if (!Wire.available()) {
              sendJSON["Sensor"] = "LPS22DF";
              sendJSON["Result"] = "Error";
              sendJSON["Msg"] = "Not found";
              serializeJson(sendJSON, *activePort);
              activePort->println();
              break;
            }

            uint8_t id = Wire.read();  // 0xB4

            Wire.beginTransmission(LPS22DF_ADDR);
            Wire.write(LPS_CTRL_REG1);
            Wire.write(0x40);
            Wire.endTransmission();

            for (int i = 0; i < 30; i++) {
              Wire.beginTransmission(LPS22DF_ADDR);
              Wire.write(LPS_PRESS_OUT);
              Wire.endTransmission(false);
              Wire.requestFrom((uint16_t)LPS22DF_ADDR, (uint8_t)5);

              sendJSON.clear();
              sendJSON["Sensor"] = "LPS22DF";

              if (Wire.available() == 5) {
                uint8_t px = Wire.read();
                uint8_t pl = Wire.read();
                uint8_t ph = Wire.read();
                uint8_t tl = Wire.read();
                uint8_t th = Wire.read();

                int32_t press_raw = (int32_t)(ph << 16 | pl << 8 | px);
                if (press_raw & 0x00800000) press_raw |= 0xFF000000;
                int16_t temp_raw = (int16_t)(th << 8 | tl);

                sendJSON["Result"] = "OK";
                sendJSON["ID"] = id;
                sendJSON["Press_hPa"] = press_raw / 4096.0f;
                sendJSON["Temp_C"] = temp_raw / 100.0f;
              } else {
                sendJSON["Result"] = "Error";
                sendJSON["Msg"] = "Read error";
              }

              serializeJson(sendJSON, *activePort);
              activePort->println();
              delay(200);
            }
            break;
          }

        case 7:  // ==== VCNL4040 ====
          {
            sendJSON.clear();
            if (!proximitySensor.begin()) {
              sendJSON["Sensor"] = "VCNL4040";
              sendJSON["Result"] = "Error";
              sendJSON["Msg"] = "Not found";
              serializeJson(sendJSON, Serial);
              Serial.println();
              break;
            }

            for (int i = 0; i < 30; i++) {
              unsigned int proxValue = proximitySensor.getProximity();

              sendJSON.clear();
              sendJSON["Sensor"] = "VCNL4040";
              sendJSON["Result"] = "OK";
              sendJSON["Prox"] = proxValue;
              serializeJson(sendJSON, *activePort);
              activePort->println();
              delay(200);
            }
            break;
          }

        case 8:  // ==== SD CARD ====
          {
            sendJSON.clear();
            sendJSON["Sensor"] = "SD_CARD";

            // Reiniciamos el bus SPI global justo antes de montar la SD
            // para asegurar que GPIO 27 (SCK) esté asignado a SPI y usamos 4 MHz para mayor estabilidad
            SPI.end();
            SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);

            if (!SD.begin(CS_PIN, SPI, 4000000)) {
              sendJSON["Result"] = "Error";
              sendJSON["Msg"] = "Init failed / No SD";
            } else {
              uint8_t cardType = SD.cardType();

              if (cardType == CARD_NONE) {
                sendJSON["Result"] = "Error";
                sendJSON["Msg"] = "No SD card attached";
              } else {
                sendJSON["Result"] = "OK";

                if (cardType == CARD_MMC) sendJSON["Type"] = "MMC";
                else if (cardType == CARD_SD) sendJSON["Type"] = "SDSC";
                else if (cardType == CARD_SDHC) sendJSON["Type"] = "SDHC";
                else sendJSON["Type"] = "UNKNOWN";

                uint64_t cardSizeMB = SD.cardSize() / (1024ULL * 1024ULL);
                sendJSON["Size_MB"] = cardSizeMB;
              }
              SD.end();  // Liberamos el montaje para permitir re-ejecutar el test en caliente
            }

            serializeJson(sendJSON, *activePort);
            activePort->println();
            break;
          }

        case 9:  // ==== WiFi Connection ====
          {
            sendJSON.clear();
            const char *ssid = "IngPruebas-Master";
            const char *password = "cachirula";
            const char *serverUrl = "http://192.168.4.1/ping";

            WiFi.mode(WIFI_STA);
            WiFi.setSleep(false);  // Evita microcortes que provocan el error -11
            WiFi.begin(ssid, password);

            for (int i = 0; i < 30; i++) {
              if (WiFi.status() == WL_CONNECTED) break;
              delay(500);
            }

            if (WiFi.status() != WL_CONNECTED) {
              sendJSON["Result"] = "FAIL";
              sendJSON["error"] = "WiFi connection timeout";
              sendJSON["wifi_status"] = WiFi.status();
            } else {
              sendJSON["ip"] = WiFi.localIP().toString();
              sendJSON["rssi"] = WiFi.RSSI();

              HTTPClient http;
              http.setReuse(false);   // Fuerza conexión limpia en cada ciclo
              http.setTimeout(5000);  // Tiempo de espera explícito (5 segundos)
              http.begin(serverUrl);
              http.addHeader("Content-Type", "application/json");

              JsonDocument docReq;
              docReq["dispositivo"] = "Esclavo_B";
              docReq["mensaje"] = "Ping para validar conexion";

              String jsonRequest;
              serializeJson(docReq, jsonRequest);

              int httpResponseCode = http.POST(jsonRequest);
              sendJSON["http_code"] = httpResponseCode;

              if (httpResponseCode == 200) {
                String payload = http.getString();

                JsonDocument docRes;
                DeserializationError error = deserializeJson(docRes, payload);

                if (!error) {
                  String respuestaMaster = docRes["respuesta"] | "sin respuesta";
                  unsigned long uptimeMaster = docRes["master_uptime_ms"] | 0;

                  sendJSON["Result"] = "OK";
                  sendJSON["response"] = respuestaMaster;
                  // sendJSON["uptime"] = uptimeMaster;
                } else {
                  sendJSON["Result"] = "FAIL";
                  sendJSON["error"] = "Error al parsear JSON del accessPoint";
                  sendJSON["parse_error"] = error.c_str();
                }
              } else {
                sendJSON["Result"] = "FAIL";
                sendJSON["error"] = (httpResponseCode > 0)
                                      ? "HTTP status inesperado"
                                      : "Error en peticion HTTP";
              }

              http.end();
            }

            serializeJson(sendJSON, *activePort);
            activePort->println();
            break;
          }
        case 10:  // ==== GPIOs Status ====
          {
            sendJSON.clear();
            sendJSON["Test"] = "GPIOs";

            bool stateA = testGpios(PIN_24, PIN_9);
            bool stateB = testGpios(PIN_4, PIN_5);

            if (stateA && stateB) {
              sendJSON["Result"] = "OK";
            } else {
              sendJSON["gpios"] = false;
              sendJSON["D4_D6"] = stateA;
              sendJSON["D16_D17"] = stateB;
            }

            serializeJson(sendJSON, *activePort);
            activePort->println();
            break;
          }

        case 15:  // ==== RESTART ====
          {
            ESP.restart();
            break;
          }

        default:
          sendJSON.clear();
          sendJSON["error"] = "InvalidInput";
          sendJSON["msg"] = "invalid option";
          serializeJson(sendJSON, *activePort);
          activePort->println();
          break;
      }
    }
  } else {
    demo();
  }
}

// ______________________

void quickStart() {
  Wire.beginTransmission(MAX17048_ADDR);
  Wire.write(REG_MODE);
  Wire.write(0x40);
  Wire.write(0x00);
  Wire.endTransmission();
}

uint16_t leerRegistro16(uint8_t reg) {
  Wire.beginTransmission(MAX17048_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);

  Wire.requestFrom((uint8_t)MAX17048_ADDR, (size_t)2);

  if (Wire.available() == 2) {
    uint16_t msb = Wire.read();
    uint16_t lsb = Wire.read();
    return (msb << 8) | lsb;
  }
  return 0;
}

float leerVoltaje() {
  uint16_t rawVcell = leerRegistro16(REG_VCELL);
  return rawVcell * 0.000078125;
}

float leerSOC() {
  uint16_t rawSoc = leerRegistro16(REG_SOC);
  return rawSoc / 256.0;
}


bool testGpios(uint8_t gpioA, uint8_t gpioB) {
  bool resultAB = testSequence(gpioA, gpioB);
  if (!resultAB)
    return false;
  delay(10);  // Pausa entre pruebas
  bool resultBA = testSequence(gpioB, gpioA);
  if (!resultBA)
    return false;
  return true;  // Ambas pruebas correctas
}

bool testSequence(uint8_t gpioOut, uint8_t gpioIn) {

  pinMode(gpioOut, OUTPUT);
  pinMode(gpioIn, INPUT);  // Configuración con resistencia
  uint8_t testPattern[] = { 1, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1, 1 };

  for (int i = 0; i < sizeof(testPattern); i++) {
    digitalWrite(gpioOut, testPattern[i]);  // Envía bit
    delay(10);                              // Espera de estabilización
    int readValue = digitalRead(gpioIn);    // Lee bit
    if (readValue != testPattern[i]) {
      return false;
    }
  }

  return true;  // Todos los bits coincidieron correctamente
}