/*
Firmware test base v1.0 Pulsar C5 

Funcionalidades: 

- Control de blink y neopixel 
- Lectura de direcciones I2C y manejo de sensores nativos 
  . MAX17048 Addr 0x36      | Charger Battery Sensor
  . MMC5603 Addr 0x30       | Magnetometer Sensor 
  . SHT40 Addr 0x44 No Addr | Humidity and Temperature Sensor
  . LPS22DF Addr 0x5C       | Pressure and Temperature Sensor
  . VCNL4040 Addr 0x60

  34 60 18
*/

// ==== BIBLIOTECAS ====
#include <Wire.h>
#include <WiFi.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_MMC56x3.h>

// ==== DECLARACIÓN DE GPIOS ====
#define NEOP_PIN 0
#define LED_BUILTIN 27
#define SDA_PIN 2
#define SCL_PIN 3

// ==== MACROS Y VARIABLES GLOBALES ====
#define NUMPIXELS 1    // Número de Neopixeles
int intensity = 30;    // Intensidad de brillo en Neopixel
int delay_neop = 250;  // Tiempo de actualización en Neopixel

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
        digitalWrite(LED_BUILTIN, HIGH);
        pixels.setPixelColor(pixelIndex, pixels.Color(intensity, 0, 0));
        break;

      case 1:  // Estado 1: Verde
        digitalWrite(LED_BUILTIN, LOW);
        pixels.setPixelColor(pixelIndex, pixels.Color(0, intensity, 0));
        break;

      case 2:  // Estado 2: Azul
        digitalWrite(LED_BUILTIN, HIGH);
        pixels.setPixelColor(pixelIndex, pixels.Color(0, 0, intensity));
        break;

      case 3:  // Estado 3: Cian/Custom
        digitalWrite(LED_BUILTIN, LOW);
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

  pixels.clear();
  pixels.setBrightness(150);

  // ==== Inicialización de GPIOs ====
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

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
            String mac = WiFi.macAddress();
            sendJSON["mac"] = mac;
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }


        case 3:
          {
            for (uint8_t addr = 1; addr < 127; addr++) {
              Wire.beginTransmission(addr);
              if (Wire.endTransmission() == 0) {
                Serial.print("Dispositivo en 0x");
                Serial.println(addr, HEX);
              }
              delay(10);
            }
            break;
          }

        case 4:
          {
            if (!mmc.begin(MMC56X3_DEFAULT_ADDRESS, &Wire)) {  // I2C mode
              /* There was a problem detecting the MMC5603 ... check your connections */
              Serial.println("Ooops, no MMC5603 detected ... Check your wiring!");
              break;
            } else {
              Serial.println("MMC5603 initialized...");
            }

            /* Display some basic information on this sensor */
            mmc.printSensorDetails();
            mmc.setDataRate(100);  // in Hz, from 1-255 or 1000
            mmc.setContinuousMode(true);

            for (int i = 0; i < 30; i++) {

              sensors_event_t event;
              mmc.getEvent(&event);

              // Display the results (magnetic vector values are in micro-Tesla (uT))
              Serial.print("X: ");
              Serial.print(event.magnetic.x);
              Serial.print("  ");
              Serial.print("Y: ");
              Serial.print(event.magnetic.y);
              Serial.print("  ");
              Serial.print("Z: ");
              Serial.print(event.magnetic.z);
              Serial.print("  ");
              Serial.println("uT");
              delay(200);
            }

            break;
          }

        case 5:
          {
            quickStart();
            delay(1000);
            float voltaje = leerVoltaje();
            float porcentaje = leerSOC();

            // Si el algoritmo se pierde por un pico de voltaje y da más del 100%
            if (porcentaje > 100.0) {
              Serial.println("-> SOC irreal detectado (>100%). Forzando Quick-Start...");
              quickStart();
              delay(500);  // Esperamos medio segundo para que actualice los registros

              // Volvemos a tomar las lecturas ya corregidas
              voltaje = leerVoltaje();
              porcentaje = leerSOC();
            }

            Serial.print("Voltaje: ");
            Serial.print(voltaje, 1);  // 1 decimal de precisión
            Serial.print(" V  |  SOC: ");
            Serial.print(porcentaje, 1);
            Serial.println(" %");

            break;
          }

        case 6:
          {
            Serial.println("Iniciando lecturas LPS22DF (I2C Nativo)...");

            for (int i = 0; i < 30; i++) {
              Wire.beginTransmission(LPS22DF_ADDR);
              Wire.write(LPS_PRESS_OUT);
              Wire.endTransmission(false);
              Wire.requestFrom((uint16_t)LPS22DF_ADDR, (uint8_t)5);

              if (Wire.available() == 5) {
                uint8_t px = Wire.read();  // XL
                uint8_t pl = Wire.read();  // L
                uint8_t ph = Wire.read();  // H
                uint8_t tl = Wire.read();  // Temp L
                uint8_t th = Wire.read();  // Temp H

                // Ensamblar presión (24 bits)
                int32_t press_raw = (int32_t)(ph << 16 | pl << 8 | px);
                // Extensión de signo manual para números negativos
                if (press_raw & 0x00800000) {
                  press_raw |= 0xFF000000;
                }

                // Ensamblar temperatura (16 bits)
                int16_t temp_raw = (int16_t)(th << 8 | tl);

                // Conversión física
                pressure = press_raw / 4096.0f;
                temperature = temp_raw / 100.0f;

                Serial.print("Pressure[hPa]:");
                Serial.print(pressure, 2);
                Serial.print(", Temperature[C]:");
                Serial.println(temperature, 2);
              } else {
                Serial.println("Error de lectura I2C en LPS22DF.");
              }
              delay(200);
            }
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
