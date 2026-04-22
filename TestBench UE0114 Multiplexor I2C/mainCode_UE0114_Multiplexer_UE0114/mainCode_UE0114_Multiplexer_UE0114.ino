/* 
MainCode UE0114 TestBench DevLab: I2C TCA9548A Multiplexer Module

*/

#include <Wire.h>
#include <HardwareSerial.h>
#include <Arduino.h>
#include <ArduinoJson.h>


// ==== CONFIGURACIÓN DE PINES ====
#define SDA_PIN 6
#define SCL_PIN 7
#define A0_PIN 15  // --> GPIO15 para control de pin A0 Selector de Dirección I2C
#define A1_PIN 19  // --> GPIO19 para control de pin A1 Selector de Dirección I2C
#define A2_PIN 20  // --> GPIO20 para control de pin A2 Selector de Dirección I2C


// ==== VARIABLES DE CONFIGURACIÓN ====
uint8_t ADD[8] = { 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77 };  // Direcciones del Multiplexor


String JSON_entrada;                   ///< Buffer para recibir JSON desde PagWeb
StaticJsonDocument<1024> receiveJSON;  ///< Documento JSON para parsear datos recibidos

String JSON_lectura;                ///< Buffer para transmitir JSON de respuesta
StaticJsonDocument<1024> sendJSON;  ///< Documento JSON para armar respuestas


void printDebug(String str) {
  str.replace("\"", "\\\"");  // Escapa comillas
  Serial.println("{\"debug\": \"" + str + "\"}");
}

// ==== FUNCIONES DE CONTROL DEL MUX ====

bool tcaselect(uint8_t port) {
  if (port > 7) return false;

  Wire.beginTransmission(ADD[0]);
  Wire.write(1 << port);
  delay(10);
  return (Wire.endTransmission() == 0);
}

// Nueva función: Cierra todos los canales para evitar arrastrar cortos
void tcaDisable() {
  Wire.beginTransmission(ADD[0]);
  Wire.write(0);
  Wire.endTransmission();
}

// ==== TRUCO DE INGENIERÍA: Recuperación de I2C ====
// Genera pulsos de reloj manuales para forzar a cualquier esclavo colgado a soltar la línea SDA
void recoverI2CBus() {
  pinMode(SDA_PIN, INPUT);
  pinMode(SCL_PIN, INPUT);
  delay(50);

  if (digitalRead(SDA_PIN) == LOW) {
    printDebug("¡BUS I2C COLGADO! Intentando recuperacion por bit-bang...");
    pinMode(SCL_PIN, OUTPUT);
    for (int i = 0; i < 16; i++) {
      digitalWrite(SCL_PIN, LOW);
      delay(20);
      digitalWrite(SCL_PIN, HIGH);
      delay(20);
    }
  }

  pinMode(SDA_PIN, INPUT);
  pinMode(SCL_PIN, INPUT);
  Wire.begin(SDA_PIN, SCL_PIN);  // Reiniciamos el periférico de la ESP32
}

void setup() {

  // ==== Inicialización de comunicación serie ====
  Serial.begin(115200);
  delay(100);
  printDebug("Serial initialized...");

  // ==== Inicialización de comunicación I2C ====
  Wire.begin(SDA_PIN, SCL_PIN);
  printDebug("I2C Multiplexer...");
  Wire.setTimeOut(50);  // Si en 50ms no hay respuesta, libera el bus (ESP32)

  // ==== Declaración de GPIOS ====
  // ---- Salidas ----
  pinMode(A0_PIN, OUTPUT);
  pinMode(A1_PIN, OUTPUT);
  pinMode(A2_PIN, OUTPUT);

  digitalWrite(A0_PIN, LOW);
  digitalWrite(A1_PIN, LOW);
  digitalWrite(A2_PIN, LOW);
}

void loop() {

  if (Serial.available()) {

    JSON_entrada = Serial.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);

    if (!error) {

      String Function = receiveJSON["Function"];

      int opc = 0;
      if (Function == "ping") opc = 1;           // {"Function":"ping"}
      else if (Function == "scanAdd") opc = 2;   // {"Function":"scanAdd"}
      else if (Function == "scanPort") opc = 3;  // {"Function":"scanPort"}

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
            JsonArray foundMuxes = sendJSON.createNestedArray("mux_addr");

            printDebug("Scanning dynamic MUX addresses...");

            // Iteramos sobre las 8 combinaciones posibles de hardware
            for (int step = 0; step < 8; step++) {

              // 1. Configuramos los pines lógicos
              digitalWrite(A0_PIN, (step >> 0) & 1);
              digitalWrite(A1_PIN, (step >> 1) & 1);
              digitalWrite(A2_PIN, (step >> 2) & 1);

              // 10ms de delay para que los niveles lógicos de 3.3V y el TCA se asienten
              delay(10);

              // 2. Verificamos SI responde a la dirección ESPERADA para esta combinación
              Wire.beginTransmission(ADD[step]);
              byte error = Wire.endTransmission();

              if (error == 0) {
                // Formateo usando String para que ArduinoJson reserve la memoria de forma segura
                String hexStr = "0x" + String(ADD[step], HEX);  // Esto es válido pq la tabla de verdad de Addr está ordenada 000 0x70 -> 111 0x77
                foundMuxes.add(hexStr);
              }
            }

            // Validamos que hayamos encontrado al menos una respuesta
            if (foundMuxes.size() == 8) {
              sendJSON["Result"] = "OK";
            } else {
              sendJSON["Result"] = "FAIL";
            }

            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }

        case 3:
          {
            sendJSON.clear();
            int totalTargets = 0;
            printDebug("Starting scan ports...");

            // 1. Inicialización de dirección 0x70 para MUX
            digitalWrite(A0_PIN, LOW);
            digitalWrite(A1_PIN, LOW);
            digitalWrite(A2_PIN, LOW);
            delay(50);

            printDebug("Inicio de bucle de lectura de puertos...");

            for (byte p = 0; p < 8; p++) {
              printDebug("-> Intentando abrir MUX Port: " + String(p));

              bool dir = false;
              String portName = "port_" + String(p);
              JsonArray portDevices = sendJSON.createNestedArray(portName);

              if (!tcaselect(p)) {
                printDebug("ERROR: MUX no respondio al abrir Port " + String(p));
                recoverI2CBus();
              } else {
                printDebug("Port " + String(p) + " abierto. Escaneando direcciones...");
                delay(10);  // Pequeño respiro para que el FET del multiplexor se asiente físicamente

                for (byte address = 1; address < 127; ++address) {
                  if (address == ADD[0]) continue;

                  Wire.beginTransmission(address);
                  byte error = Wire.endTransmission();

                  if (error == 0) {
                    dir = true;
                    Serial.println("I2C device found at address 0x" + String(address, HEX));
                    String hexStr = "0x" + String(address, HEX);
                    portDevices.add(hexStr);
                    totalTargets++;
                    break;
                  }

                  // Cambia esto para imprimir TODOS los errores, no solo el 2,
                  // por si el ESP32 arroja un error de timeout (5) o error desconocido (4).
                  if (error != 0) {
                    //printDebug("error: " + String(error) + " iter: " + String(address, HEX));
                  }

                  // CRÍTICO: Pequeño respiro para estabilizar la capacitancia del bus y vaciar el buffer UART
                  delay(1);
                }
              }

              // CRÍTICO: Aislar el puerto inmediatamente después del barrido
              tcaDisable();
              printDebug("Port " + String(p) + " escaneo finalizado y cerrado.");
              delay(5);
            }

            sendJSON["Total_Targets"] = totalTargets;
            sendJSON["status"] = "scan_completed";

            printDebug("Armando JSON final...");
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }

        default: break;
      }
    }
  }
}
