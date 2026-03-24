/*
╔╔════════════════════════════════════════════════════════════════╗
║║    CÓDIGO DE INTEGRACIÓN Y CONTROL - UE0002 DualMCU          ║║
╚╚════════════════════════════════════════════════════════════════╝

📋 DESCRIPCIÓN GENERAL:
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  Módulo puente para el control integrado de la tarjeta UE0002 DualMCU
  que incluye:
  
  ✓ Control de relevadores de alimentación para:
    - Cargador de baterías con super capacitor
    - Demanda de carga variable
  
  ✓ Comunicación bidireccional:
    - UART con ESP32 de la DualMCU (pines D0/D1)
    - Puente comunicación con RP2040 via DipSwitch
  
  ✓ Funcionalidad de sensores:
    - I2C para sensor de corriente (INA219)
    - Comunicación con sensores BME/BMI
    - Control de motor háptico
  
  ✓ Interfaz con Multi TestBench:
    - Botonera de arranque (GPIO02)
    - Relés de alimentación y puerto USB-C


📌 INSTRUCCIONES DE INSTALACIÓN:
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  1. Cargar en la tarjeta Pulsar C6 integrada en el TestBench
  2. Seleccionar el puerto COM de la Pulsar C6 en la interfaz de pruebas
  3. La comunicación se establece automáticamente en el setup()

*/


// ╔════════════════════════════════════════════════════════════════╗
// ║                    LIBRERÍAS NECESARIAS                       ║
// ╚════════════════════════════════════════════════════════════════╝

#include <Wire.h>             // Comunicación I2C
#include <HardwareSerial.h>   // UART por hardware
#include <ArduinoJson.h>      // Procesamiento de JSON
#include <Arduino.h>          // Funciones base Arduino
#include <Adafruit_INA219.h>  // Sensor de corriente INA219

// ╔════════════════════════════════════════════════════════════════╗
// ║                   CONFIGURACIÓN DE PINES                      ║
// ╚════════════════════════════════════════════════════════════════╝

#define RX2 D1        // GPIO04 - Recepción UART para ESP32
#define TX2 D0        // GPIO05 - Transmisión UART para ESP32
#define RUN_BUTTON 2  // GPIO02 - Botón de arranque del TestBench
#define RELAYCom 20   // GPIO20 - Relé para alimentación por USB-C (CH340)
#define I2C_SDA 6     // GPIO06 - Línea SDA del sensor de corriente
#define I2C_SCL 7     // GPIO07 - Línea SCL del sensor de corriente
#define RELAYA 8      // GPIO08 - Relé A de fuente de alimentación
#define RELAYB 9      // GPIO09 - Relé B de fuente de alimentación

// ╔════════════════════════════════════════════════════════════════╗
// ║                   INICIALIZACIÓN DE OBJETOS                   ║
// ╚════════════════════════════════════════════════════════════════╝

HardwareSerial USB(1);             // UART2 para comunicación con PagWeb
TwoWire I2CBus = TwoWire(0);       // Bus I2C dedicado para sensores de corriente
Adafruit_INA219 ina219_in(0x40);   // Sensor de corriente de entrada (dirección 0x40)
Adafruit_INA219 ina219_out(0x41);  // Sensor de corriente de salida (dirección 0x41)

// ╔════════════════════════════════════════════════════════════════╗
// ║                  VARIABLES DE COMUNICACIÓN                    ║
// ╚════════════════════════════════════════════════════════════════╝

String JSON_entrada;                  // Buffer para recibir JSON desde PagWeb
StaticJsonDocument<256> receiveJSON;  // Estructura para parsear JSON recibido

String JSON_lectura;               // Buffer para enviar JSON de datos
StaticJsonDocument<256> sendJSON;  // Estructura para armar JSON a enviar



// ╔════════════════════════════════════════════════════════════════╗
// ║              FUNCIÓN DE MEDICIÓN DE CORRIENTE                 ║
// ╚════════════════════════════════════════════════════════════════╝

/**
 * @brief Mide la corriente utilizando el sensor INA219 de entrada
 * 
 * Calcula:
 *   - Voltaje en la resistencia Shunt
 *   - Corriente en milivoltios (mA)
 *   - Voltaje de carga
 *   - Potencia consumida
 * 
 * @return float Corriente medida en miliamperios (mA)
 * 
 * @note La resistencia Shunt es de 50 mΩ (0.05 Ω)
 */
float medirCorriente() {
  // Constantes de calibración
  const float shuntOffset_mV = 0.0;  // Offset en vacío para compensación
  const float R_SHUNT = 0.05;        // Resistencia Shunt = 50 mΩ

  // Lectura de voltajes del sensor
  float shunt_mV = ina219_in.getShuntVoltage_mV();
  float bus_V = ina219_in.getBusVoltage_V();

  // Compensación de offset
  shunt_mV -= shuntOffset_mV;

  // Cálculos derivados
  float shunt_V = shunt_mV / 1000.0;
  float current_A = shunt_V / R_SHUNT;  // Corriente en Amperios
  float load_V = bus_V + shunt_V;       // Voltaje total de carga
  float power_W = load_V * current_A;   // Potencia en Watts

  // Conversión a miliamperios
  float current_mA = current_A * 1000;

  return current_mA;
}


// ╔════════════════════════════════════════════════════════════════╗
// ║           FUNCIÓN DE INICIALIZACIÓN DEL SISTEMA               ║
// ╚════════════════════════════════════════════════════════════════╝

/**
 * @brief Inicializa todos los periféricos y comunicaciones
 * 
 * Configura:
 *   - Puertos seriales (Serial y USB UART)
 *   - Bus I2C y sensores de corriente
 *   - Pines de control (relés y botón)
 */
void setup() {
  // Inicialización de comunicaciones seriales
  Serial.begin(115200);                     // Serial hacia TestBench/PagWeb
  USB.begin(115200, SERIAL_8N1, RX2, TX2);  // UART hacia ESP32 de DualMCU

  // Configuración del bus I2C
  I2CBus.begin(I2C_SDA, I2C_SCL);

  // Inicialización del sensor INA219 de entrada
  if (!ina219_in.begin(&I2CBus)) {
    Serial.println("⚠️ Error: INA219 no encontrado en direccion 0x40");
  }

  // Configuración de pines como salidas/entradas
  pinMode(RELAYA, OUTPUT);     // Relé A - Salida
  pinMode(RELAYB, OUTPUT);     // Relé B - Salida
  pinMode(RELAYCom, OUTPUT);   // Relé USB-C - Salida
  pinMode(RUN_BUTTON, INPUT);  // Botón de arranque - Entrada

  // Estado inicial: todos los relés desactivados
  digitalWrite(RELAYCom, LOW);  // Cargador alimentando DualMCU
  digitalWrite(RELAYA, LOW);    // Relé A desactivado
  digitalWrite(RELAYB, LOW);    // Relé B desactivado
}


// ╔════════════════════════════════════════════════════════════════╗
// ║                    BUCLE PRINCIPAL                            ║
// ╚════════════════════════════════════════════════════════════════╝

/**
 * @brief Bucle principal del programa
 * 
 * Maneja tres eventos principales:
 *   1. Detección de presión del botón de arranque
 *   2. Procesamiento de comandos JSON desde Serial (PagWeb)
 *   3. Retransmisión de mensajes JSON desde USB (ESP32)
 */
void loop() {

  // ════════════════════════════════════════════════════════════════
  // ⓵ DETECCIÓN DEL BOTÓN DE ARRANQUE
  // ════════════════════════════════════════════════════════════════

  if (digitalRead(RUN_BUTTON) == HIGH) {
    delay(200);  // Anti-debounce

    if (digitalRead(RUN_BUTTON) == LOW) {
      // ✓ Botón presionado correctamente detectado
      sendJSON.clear();
      sendJSON["Run"] = "OK";
      serializeJson(sendJSON, Serial);  // Envío de confirmación a PagWeb
      Serial.println();
    }
  }

  // ════════════════════════════════════════════════════════════════
  // ⓶ PROCESAMIENTO DE COMANDOS DESDE SERIAL (PagWeb)
  // ════════════════════════════════════════════════════════════════

  if (Serial.available()) {
    // Lectura de comando JSON completo
    JSON_entrada = Serial.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);

    if (!error) {
      String Function = receiveJSON["Function"];
      int opc = 0;

      // ╭─────────────────────────────────────────────────────────────╮
      // │ MAPEO DE COMANDOS A OPCIONES (SWITCH)                       │
      // │ Esto mejora la legibilidad vs múltiples if-else             │
      // ╰─────────────────────────────────────────────────────────────╯

      // ─ Comandos de validación ESP32 ─
      if (Function == "ping") opc = 1;             // {"Function":"ping"}
      else if (Function == "mac") opc = 2;         // {"Function":"mac"}
      else if (Function == "bme") opc = 3;         // {"Function":"bme"}
      else if (Function == "test_esp32") opc = 4;  // {"Function":"test_esp32"}
      else if (Function == "vn_sensor") opc = 5;   // {"Function":"vn_sensor"}

      // ─ Comandos de validación RP2040 ─
      else if (Function == "passthrough") opc = 6;  // {"Function":"passthrough"}
      else if (Function == "hmotor") opc = 7;       // {"Function":"hmotor"}
      else if (Function == "analog_rp") opc = 8;    // {"Function":"analog_rp"}
      else if (Function == "test_rp") opc = 9;      // {"Function":"test_rp"}

      // ─ Comandos de control TestBench ─
      else if (Function == "VCC_ON") opc = 10;     // {"Function":"VCC_ON"}
      else if (Function == "VCC_OFF") opc = 11;    // {"Function":"VCC_OFF"}
      else if (Function == "meas_curr") opc = 12;  // {"Function":"meas_curr"}

      // ╭─────────────────────────────────────────────────────────────╮
      // │ EJECUCIÓN DE COMANDOS                                       │
      // ╰─────────────────────────────────────────────────────────────╯

      switch (opc) {
        // ───────────────────────────────────────────────────────────
        // ESP32 - COMANDO: PING (Verificación de conexión)
        // ───────────────────────────────────────────────────────────
        case 1:
          {
            sendJSON.clear();
            sendJSON["Function"] = "ping";
            serializeJson(sendJSON, USB);
            USB.println();
            break;
          }

        // ───────────────────────────────────────────────────────────
        // ESP32 - COMANDO: MAC (Solicitar dirección MAC)
        // ───────────────────────────────────────────────────────────
        case 2:
          {
            sendJSON.clear();
            sendJSON["Function"] = "mac";
            serializeJson(sendJSON, USB);
            USB.println();
            break;
          }

        // ───────────────────────────────────────────────────────────
        // ESP32 - COMANDO: BME (Sensor ambiental)
        // ───────────────────────────────────────────────────────────
        case 3:
          {
            sendJSON.clear();
            sendJSON["Function"] = "bme";
            serializeJson(sendJSON, USB);
            USB.println();
            break;
          }

        // ───────────────────────────────────────────────────────────
        // ESP32 - COMANDO: TEST_ESP32 (Test general del ESP32)
        // ───────────────────────────────────────────────────────────
        case 4:
          {
            sendJSON.clear();
            sendJSON["Function"] = "test_esp32";
            serializeJson(sendJSON, USB);
            USB.println();
            break;
          }

        // ───────────────────────────────────────────────────────────
        // ESP32 - COMANDO: VN_SENSOR (Sensor de voltaje nominal)
        // ───────────────────────────────────────────────────────────
        case 5:
          {
            sendJSON.clear();
            sendJSON["Function"] = "vn_sensor";
            serializeJson(sendJSON, USB);
            USB.println();
            break;
          }

        // ───────────────────────────────────────────────────────────
        // RP2040 - COMANDO: PASSTHROUGH (Pasar comandos directos)
        // ───────────────────────────────────────────────────────────
        case 6:
          {
            sendJSON.clear();
            sendJSON["Function"] = "passthrough";
            serializeJson(sendJSON, USB);
            USB.println();
            break;
          }

        // ───────────────────────────────────────────────────────────
        // RP2040 - COMANDO: HMOTOR (Motor háptico)
        // ───────────────────────────────────────────────────────────
        case 7:
          {
            sendJSON.clear();
            sendJSON["Function"] = "hmotor";
            serializeJson(sendJSON, USB);
            USB.println();
            break;
          }

        // ───────────────────────────────────────────────────────────
        // RP2040 - COMANDO: ANALOG_RP (Entradas analógicas RP2040)
        // ───────────────────────────────────────────────────────────
        case 8:
          {
            sendJSON.clear();
            sendJSON["Function"] = "analog_rp";
            serializeJson(sendJSON, USB);
            USB.println();
            break;
          }

        // ───────────────────────────────────────────────────────────
        // RP2040 - COMANDO: TEST_RP (Test general del RP2040)
        // ───────────────────────────────────────────────────────────
        case 9:
          {
            sendJSON.clear();
            sendJSON["Function"] = "test_rp";
            serializeJson(sendJSON, USB);
            USB.println();
            break;
          }

        // ───────────────────────────────────────────────────────────
        // TESTBENCH - COMANDO: VCC_ON (Activar alimentación)
        // ───────────────────────────────────────────────────────────
        case 10:
          {
            digitalWrite(RELAYCom, HIGH);  // ⚡ Activar relé
            delay(50);                     // Estabilización
            break;
          }

        // ───────────────────────────────────────────────────────────
        // TESTBENCH - COMANDO: VCC_OFF (Desactivar alimentación)
        // ───────────────────────────────────────────────────────────
        case 11:
          {
            digitalWrite(RELAYCom, LOW);  // ⭕ Desactivar relé
            delay(50);                    // Estabilización
            break;
          }

        // ───────────────────────────────────────────────────────────
        // TESTBENCH - COMANDO: MEAS_CURR (Medir corriente)
        // ───────────────────────────────────────────────────────────
        case 12:
          {
            delay(50);  // Pre-estabilización
            sendJSON.clear();

            // Tomar múltiples muestras para promediar
            float meas = 0;
            const int muestras = 10;

            for (int i = 0; i < muestras; i++) {
              float corriente = medirCorriente();
              meas += corriente;
              delay(20);  // Intervalo entre muestras
            }

            // Calcular promedio
            float avg_current = meas / muestras;

            // Enviar resultado
            sendJSON["current"] = avg_current;
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }
      }
    }
  }

  // ════════════════════════════════════════════════════════════════
  // ⓷ RETRANSMISIÓN DE MENSAJES DESDE USB (ESP32 → PagWeb)
  // ════════════════════════════════════════════════════════════════

  if (USB.available()) {
    String incoming = USB.readStringUntil('\n');  // Leer línea completa desde ESP32

    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, incoming);

    if (!error) {
      // ✓ JSON válido: reenviar limpio hacia PagWeb
      serializeJson(doc, Serial);
      Serial.println();
    }
    // ✗ Si no es JSON válido, se ignora silenciosamente
  }
}