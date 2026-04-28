#include <Wire.h>
#include <HardwareSerial.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Adafruit_TSL2591.h>


#define SDA_PIN 6
#define SCL_PIN 7
#define RELAYUSB 20  // GPIO020 RELAY USBC

String JSON_entrada;                   ///< Buffer para recibir JSON desde PagWeb
StaticJsonDocument<1024> receiveJSON;  ///< Documento JSON para parsear datos recibidos

String JSON_lectura;                ///< Buffer para transmitir JSON de respuesta
StaticJsonDocument<1024> sendJSON;  ///< Documento JSON para armar respuestas


//Creacion de objeto de clase Adafruit_TSL2591 que hace referencia al sensor
tsl2591Gain_t ganancias[] = {
  TSL2591_GAIN_LOW,   /// low gain (1x)
  TSL2591_GAIN_MED,   /// medium gain (25x)
  TSL2591_GAIN_HIGH,  /// medium gain (428x)
  TSL2591_GAIN_MAX
};

tsl2591IntegrationTime_t timings[] = {
  TSL2591_INTEGRATIONTIME_100MS,  // 100 millis
  TSL2591_INTEGRATIONTIME_200MS,  // 200 millis
  TSL2591_INTEGRATIONTIME_300MS,  // 300 millis
  TSL2591_INTEGRATIONTIME_400MS,  // 400 millis
  TSL2591_INTEGRATIONTIME_500MS,  // 500 millis
  TSL2591_INTEGRATIONTIME_600MS
};

const char* etiquetasGanancia[] = {
  "LOW",
  "MED",
  "HIGH",
  "MAX"
};

const char* etiquetasTiming[] = {
  "100",
  "200",
  "300",
  "400",
  "500",
  "600"
};

Adafruit_TSL2591 tsl = Adafruit_TSL2591(1);

void initDevice() {
  Serial.println("╔══════════════════════════════════════╗");
  Serial.println("║  Iniciando ID del TSL2591            ║");
  Serial.println("╚══════════════════════════════════════╝");

  //Inicializamos el dispositivo para verificar si hay transmision
  bool found = tsl.begin();
  if (found) {
    Serial.println("Dispositivo encontrado en la dirección 0x29");
  } else {
    Serial.println("Dispositivo no encontrado");
  }
}

void scanIICDevices() {
  Serial.println("Escaneando bus I2C...");
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("✅ Dispositivo encontrado en: 0x");
      Serial.println(addr, HEX);
    }
  }
}

void sweepMeditions() {
  for (int i = 0; i < 4; i++) {
    tsl.setGain(ganancias[i]);
    for (int j = 0; j < 6; j++) {
      Serial.print(etiquetasGanancia[i]);
      Serial.print(";");
      tsl.setTiming(timings[j]);
      delay((j + 1) * 100 + 20);
      Serial.print(etiquetasTiming[j]);
      Serial.print(";");
      printResults();
    }
  }
}

void advancedRead(void) {
  // More advanced data read example. Read 32 bits with top 16 bits IR, bottom 16 bits full spectrum
  // That way you can do whatever math and comparisons you want!
  uint32_t lum = tsl.getFullLuminosity();
  uint16_t ir, full;
  ir = lum >> 16;
  full = lum & 0xFFFF;
  Serial.print(F("[ "));
  Serial.print(millis());
  Serial.print(F(" ms ] "));
  Serial.print(F("IR: "));
  Serial.print(ir);
  Serial.print(F("  "));
  Serial.print(F("Full: "));
  Serial.print(full);
  Serial.print(F("  "));
  Serial.print(F("Visible: "));
  Serial.print(full - ir);
  Serial.print(F("  "));
  Serial.print(F("Lux: "));
  Serial.println(tsl.calculateLux(full, ir), 6);
}


void printResults() {
  uint32_t lum = tsl.getFullLuminosity();
  uint16_t ir, full;
  ir = lum >> 16;
  full = lum & 0xFFFF;

  //ms;Ganancia;ATIME;CH0;CH1;Visible;Lux
  Serial.print(millis());
  Serial.print(";");
  //Serial.print(tsl.getGain());
  //Serial.print(";");
  //Serial.print(tsl.getTiming());
  //Serial.print(";");
  Serial.print(ir);
  Serial.print(";");
  Serial.print(full);
  Serial.print(";");
  Serial.print(full - ir);
  Serial.print(";");
  Serial.print(tsl.calculateLux(full, ir));
  Serial.println("");
}

void serialDebug(const String& message) {
  String escaped = message;
  escaped.replace("\"", "\\\"");
  Serial.println("{\"debug\": \"" + escaped + "\"}");
}

void setup() {

  Serial.begin(115200);
  delay(100);
  serialDebug("Serial initialized...");

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000);
  initDevice();

  pinMode(RELAYUSB, OUTPUT);
}

void loop() {

  if (Serial.available()) {
    JSON_entrada = Serial.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);

    String Function = receiveJSON["Function"];

    int opc = 0;
    if (Function == "ping") opc = 1;       // {"Function" : "ping"}
    else if (Function == "read") opc = 2;  // {"Function" : "read"}

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
          int i = 1;  // Ganancia
          int j = 0;  // Tiempos de Integración
          tsl.setGain(ganancias[i]);
          tsl.setTiming(timings[j]);

          uint32_t lum = tsl.getFullLuminosity();
          uint16_t ir, full;
          ir = lum >> 16;
          full = lum & 0xFFFF;

          // ==== Encabezado ====
          Serial.println("---- GAIN: " + String(ganancias[i]) + " ---- " + "TIME: " + String(timings[j]) + " ---- ");
          // ==== Datos ====
          Serial.print("IR: " + String(ir) + " ");
          Serial.print("FULL: " + String(full) + " ");
          Serial.print("VIS: " + String(full - ir) + " ");
          Serial.println("LUX: " + String(tsl.calculateLux(full, ir)));
          Serial.println(" ");
          delay(500);
          break;
        }

      default: break;
    }
  }
}
