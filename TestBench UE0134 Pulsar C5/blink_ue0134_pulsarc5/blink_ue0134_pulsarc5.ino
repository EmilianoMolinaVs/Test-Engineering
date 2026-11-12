/*
Firmware test base v1.0 Pulsar C5 

Funcionalidades: 

- Control de blink y neopixel 
- Lectura de direcciones I2C y manejo de sensores nativos 
  . MAX17048 Addr 0x36      | Charger Battery Sensor
  . MMC5603 Addr 0x30       | Magnetometer Sensor 
  . SHT40 Addr 0x44 No Addr | Humidity and Temperature Sensor
  . LPS22DF Addr 0x5C       | Pressure and Temperature Sensor
  . VCNL4040 Addr 0x60      | Light Sensor 

{"I2C_Devices":["0x18","0x30","0x34","0x36","0x5C","0x60"],"Result":"OK"}


      {"Function":"ping"}
      {"Function":"mac"}
      {"Function":"i2c_scan"}
      {"Function":"mgn"}
      {"Function":"bat"}
      {"Function":"press"}
      {"Function":"prox"}
      {"Function":"sd"}


*/

// ==== BIBLIOTECAS ====
#include <Wire.h>
#include <WiFi.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_MMC56x3.h>
#include "SparkFun_VCNL4040_Arduino_Library.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"

// ==== DECLARACIÓN DE GPIOS ====
#define NEOP_PIN 0  // >> GPIO Entrada de Neopixel
#define LED_PIN 27  // >> GPIO LED BUILT IN
#define SDA_PIN 2   // >> PIN SDA Bus I2C
#define SCL_PIN 3   // >> PIN SCL bus I2C

// ¡NUEVO!: Pines seguros para el segundo bus I2C
#define SDA2_PIN 1  // >> PIN SDA Segundo Bus I2C
#define SCL2_PIN 6  // >> PIN SCL Segundo Bus I2C

// Pines SD Card
#define MISO_PIN 26  // >> PIN MISO en SPI bus para SD CARD
#define MOSI_PIN 7   // >> PIN MOSI en SPI bus para SD CARD
#define SCK_PIN 27   // >> PIN SCK en SPI bus para SD CARD
#define CS_PIN 23    // >> PIN CS en SPI bus para SD CARD

// ==== MACROS Y VARIABLES GLOBALES ====
#define NUMPIXELS 10  // Número de Neopixeles
int intensity = 20;   // Intensidad de brillo en Neopixel
int delay_neop = 50;  // Tiempo de actualización en Neopixel

String JSON_entrada;  // Variable que recibe JSON del frontend
String JSON_salida;   // Variable que envía el JSON de datos

#define MAX17048_ADDR 0x36  // Dirección de esclavo MAX17048
#define REG_VCELL 0x02      //
#define REG_SOC 0x04        //
#define REG_MODE 0x06       // Registro para comandos especiales

// ==== MACROS LPS22DF ====
#define LPS22DF_ADDR 0x5C  // Dirección en tu placa Pulsar C5
#define LPS_WHO_AM_I 0x0F
#define LPS_CTRL_REG1 0x10
#define LPS_PRESS_OUT 0x28
float temperature, pressure;

// ==== CREACIÓN DE OBJETOS ====
StaticJsonDocument<254> receiveJSON;
StaticJsonDocument<254> sendJSON;
Adafruit_NeoPixel pixels(NUMPIXELS, NEOP_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_MMC5603 mmc = Adafruit_MMC5603(12345);
VCNL4040 proximitySensor;

TwoWire Wire2 = TwoWire(1);  // Instancia para el segundo bus I2C (índice 1)
SPIClass sdSPI(HSPI);        // Instancia separada para el bus SPI (HSPI / FSPI según core de C5)

// ==== FUNCIONALIDADES ====
void demo() {
  // Variables estáticas para mantener su valor entre cada ejecución de la función
  static unsigned long previousMillis = 0;
  static int colorState = 0;
  static int pixelIndex = 0;

  unsigned long currentMillis = millis();

  // Verificamos si ya pasó el tiempo (delay_neop = 250ms)
  if (currentMillis - previousMillis >= delay_neop) {
    previousMillis = currentMillis;  // Actualizamos la marca de tiempo

    switch (colorState) {
      case 0:  // Estado 0: Rojo
        digitalWrite(LED_PIN, HIGH);
        pixels.setPixelColor(pixelIndex, pixels.Color(intensity, 0, 0));
        break;

      case 1:  // Estado 1: Verde
        digitalWrite(LED_PIN, LOW);
        pixels.setPixelColor(pixelIndex, pixels.Color(0, intensity, 0));
        break;

      case 2:  // Estado 2: Azul
        digitalWrite(LED_PIN, HIGH);
        pixels.setPixelColor(pixelIndex, pixels.Color(0, 0, intensity));
        break;

      case 3:  // Estado 3: Cian/Custom
        digitalWrite(LED_PIN, LOW);
        pixels.setPixelColor(pixelIndex, pixels.Color(0, 130, intensity));
        break;

      case 4:  // Estado 4: Limpieza y reinicio del ciclo
        pixels.clear();
        pixels.show();

        // Reiniciamos los contadores
        colorState = 0;
        pixelIndex = 0;
        return;  // Salimos para arrancar el ciclo rojo en la siguiente iteración
    }

    // Mostramos el color cargado
    pixels.show();

    // Avanzamos al siguiente pixel
    pixelIndex++;

    // Si ya recorrimos todos los pixeles, pasamos al siguiente color
    if (pixelIndex >= NUMPIXELS) {
      pixelIndex = 0;
      colorState++;
    }
  }
}

void messageIntro() {
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

  // ==== Inicializaciones ====
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire2.begin(SDA2_PIN, SCL2_PIN);
  sdSPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);

  pixels.clear();
  pixels.setBrightness(150);

  // ==== Inicialización de GPIOs ====
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // ==== Mensaje de Bienvenida ====
  // messageIntro();
}

void loop() {

  if (Serial.available()) {

    JSON_entrada = Serial.readStringUntil('\n');
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

      else if (Function == "restart") opc = 15;  // {"Function":"restart"}

      switch (opc) {
        case 1:
          {
            sendJSON.clear();
            sendJSON["ping"] = "pong";
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }

        case 2:
          {
            sendJSON.clear();
            WiFi.mode(WIFI_MODE_STA);
            sendJSON["mac"] = WiFi.macAddress();
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }

        case 3:  // ==== ESCÁNER I2C DUAL ====
          {
            sendJSON.clear();
            JsonArray devices_bus1 = sendJSON.createNestedArray("I2C_Bus1_LP");
            JsonArray devices_bus2 = sendJSON.createNestedArray("I2C_Bus2");

            // Escaneo Bus 1 (LP en GPIO 2, 3)
            for (uint8_t addr = 1; addr < 127; addr++) {
              Wire.beginTransmission(addr);
              if (Wire.endTransmission() == 0) {
                char hexAddr[5];
                sprintf(hexAddr, "0x%02X", addr);
                devices_bus1.add(hexAddr);
              }
            }

            // Escaneo Bus 2 (GPIO 1, 4)
            for (uint8_t addr = 1; addr < 127; addr++) {
              Wire2.beginTransmission(addr);
              if (Wire2.endTransmission() == 0) {
                char hexAddr[5];
                sprintf(hexAddr, "0x%02X", addr);
                devices_bus2.add(hexAddr);
              }
            }

            sendJSON["Result"] = "OK";
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }

        case 4:  // ==== MMC5603 ====
          {
            sendJSON.clear();
            if (!mmc.begin(MMC56X3_DEFAULT_ADDRESS, &Wire)) {
              sendJSON.clear();
              sendJSON["Sensor"] = "MMC5603";
              sendJSON["Result"] = "Error";
              sendJSON["Msg"] = "Not found";
              serializeJson(sendJSON, Serial);
              Serial.println();
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
              serializeJson(sendJSON, Serial);
              Serial.println();
              delay(200);
            }
            break;
          }

        case 5:  // ==== MAX17048 ====
          {
            sendJSON.clear();
            // Candado: Verificamos si hay ACK en la dirección I2C
            Wire.beginTransmission(MAX17048_ADDR);
            if (Wire.endTransmission() != 0) {
              sendJSON.clear();
              sendJSON["Sensor"] = "MAX17048";
              sendJSON["Result"] = "Error";
              sendJSON["Msg"] = "Not found";
              serializeJson(sendJSON, Serial);
              Serial.println();
              break;
            }

            quickStart();
            delay(500);  // Esperar estabilización
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
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }

        case 6:  // ==== LPS22DF ====
          {
            sendJSON.clear();
            // Candado WHO_AM_I
            Wire.beginTransmission(LPS22DF_ADDR);
            Wire.write(LPS_WHO_AM_I);
            Wire.endTransmission(false);
            Wire.requestFrom((uint16_t)LPS22DF_ADDR, (uint8_t)1);

            if (!Wire.available()) {
              sendJSON.clear();
              sendJSON["Sensor"] = "LPS22DF";
              sendJSON["Result"] = "Error";
              sendJSON["Msg"] = "Not found";
              serializeJson(sendJSON, Serial);
              Serial.println();
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

              serializeJson(sendJSON, Serial);
              Serial.println();
              delay(200);
            }
            break;
          }

        case 7:  // ==== VCNL4040 ====
          {
            sendJSON.clear();
            if (!proximitySensor.begin()) {
              sendJSON.clear();
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
              serializeJson(sendJSON, Serial);
              Serial.println();
              delay(200);
            }
            break;
          }

        case 8:  // ==== SD CARD ====
          {
            sendJSON.clear();
            sendJSON["Sensor"] = "SD_CARD";

            // ¡NUEVO! Inicializar con la instancia sdSPI
            if (!SD.begin(CS_PIN, sdSPI)) {
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

                // ¡CORREGIDO! Multiplicación para tamaño en MB
                uint64_t cardSizeMB = SD.cardSize() / (1024 * 1024);
                sendJSON["Size_MB"] = cardSizeMB;
              }
            }

            serializeJson(sendJSON, Serial);
            Serial.println();
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
          serializeJson(sendJSON, Serial);
          Serial.println();
          break;
      }
    }
  } else {
    demo();
  }
}


// ______________________

// Función para reiniciar el algoritmo del MAX17048
void quickStart() {
  Wire.beginTransmission(MAX17048_ADDR);
  Wire.write(REG_MODE);
  Wire.write(0x40);  // MSB del comando 0x4000
  Wire.write(0x00);  // LSB del comando 0x4000
  Wire.endTransmission();
}

// Función para leer registros de 16 bits
uint16_t leerRegistro16(uint8_t reg) {
  Wire.beginTransmission(MAX17048_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);  // Restart para mantener el bus

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

void initLPS22DF() {
  // Verificar comunicación
  Wire.beginTransmission(LPS22DF_ADDR);
  Wire.write(LPS_WHO_AM_I);
  Wire.endTransmission(false);
  Wire.requestFrom((uint16_t)LPS22DF_ADDR, (uint8_t)1);

  if (Wire.available()) {
    uint8_t id = Wire.read();
    Serial.printf("LPS22DF Detectado. WHO_AM_I: 0x%02X\n", id);
  } else {
    Serial.println("Error: LPS22DF no detectado en 0x5C.");
  }

  // Encender muestreo continuo (ODR = 25Hz)
  Wire.beginTransmission(LPS22DF_ADDR);
  Wire.write(LPS_CTRL_REG1);
  Wire.write(0x40);
  Wire.endTransmission();
}