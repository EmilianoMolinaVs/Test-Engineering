#include <Wire.h>
#include "Adafruit_HUSB238.h"
#include <HardwareSerial.h>
#include <Arduino.h>
#include <ArduinoJson.h>


#define RX2 15        // D4 - Recepción UART para PagWeb
#define TX2 19        // D5 - Transmisión UART para PagWeb
#define RUN_BUTTON 4  // GPIO04 - Botón de arranque del TestBench
#define I2C_SDA 22    // PIN SDA de I2C
#define I2C_SCL 23    // PIN SCL de I2C


HardwareSerial PagWeb(1);
Adafruit_HUSB238 husb238;


String JSON_entrada;                  // Buffer para recibir JSON desde PagWeb
StaticJsonDocument<256> receiveJSON;  // Estructura para parsear JSON recibido

String JSON_lectura;               // Buffer para enviar JSON de datos
StaticJsonDocument<256> sendJSON;  // Estructura para armar JSON a enviar

String cmd = "";
bool state_husb = false;

void setup() {
  Serial.begin(115200);
  PagWeb.begin(115200, SERIAL_8N1, RX2, TX2);
  Serial.println("{\"debug\": \"Serial Inicializado\"}");

  Wire.begin(I2C_SDA, I2C_SCL);

  pinMode(RUN_BUTTON, INPUT);
}

void loop() {

  // === Manipulación de módulo por SCPI ====
  while (Serial.available()) {
    char ch = Serial.read();
    if (ch == '\n') {
      handleSCPI(cmd);
      cmd = "";
    } else {
      cmd += ch;
    }
  }



  if (digitalRead(RUN_BUTTON) == HIGH) {
    delay(150);
    if (digitalRead(RUN_BUTTON) == LOW) {
      sendJSON.clear();
      sendJSON["Run"] = "OK";           // Envio de corriente JSON para corto
      serializeJson(sendJSON, PagWeb);  // Envío de datos por JSON a la PagWeb
      PagWeb.println();
    }
  }


  if (PagWeb.available()) {

    JSON_entrada = PagWeb.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);

    if (!error) {

      String Function = receiveJSON["Function"];
      String Value = receiveJSON["Value"] | "";

      int opc = 0;
      if (Function == "ping") opc = 1;            // {"Function": "ping"}
      else if (Function == "init_husb") opc = 2;  // {"Function": "init_husb"}
      else if (Function == "sweep") opc = 3;      // {"Function": "sweep"}
      else if (Function == "fixed") opc = 4;      // {"Function": "fixed", "Value": "5"}
      else if (Function == "restart") opc = 5;    // {"Function": "restart"}


      switch (opc) {
        case 1:
          {
            sendJSON.clear();
            Serial.println("Comunicación PagWeb -> PULSAR OK");
            sendJSON["ping"] = "pong";
            serializeJson(sendJSON, PagWeb);
            PagWeb.println();
            break;
          }

        case 2:
          {
            // ESP.restart();  // Reinicio de ESP32

            for (int i = 0; i < 3; i++) {
              sendJSON.clear();

              if (husb238.begin(HUSB238_I2CADDR_DEFAULT, &Wire)) {  // Initialize the HUSB238
                Serial.println("HUSB238 Inicializado...");          // Mensaje debug en Serial
                sendJSON["Result"] = "OK";                          // Respuesta JSON a PagWeb
                sendJSON["debug"] = "HSUB238 Inicializado";         // Debug JSON a PagWeb
                state_husb = true;                                  // Bandera de inicialización
                break;
              } else {
                // Serial.println("{\"ping\": \"FAIL\"}");
                Serial.println("HUSB238 NO inicializado...");   // Mensaje debug en Serial
                sendJSON["debug"] = "HSUB238 No Inicializado";  // Debug JSON a PagWeb
                sendJSON["Result"] = "FAIL";                    // Respuesta JSON a PagWeb
                state_husb = false;                             // Bandera de inicialización
              }

              delay(100);
            }

            serializeJson(sendJSON, PagWeb);
            PagWeb.println();
            break;
          }

        case 3:
          {
            sendJSON.clear();
            if (state_husb) {
              Serial.println("Voltage Sweep HSUB...");

              int voltajes[] = { 5, 9, 12, 15, 20 };
              int delay_ms = 4000;

              for (int i = 0; i < 5; i++) {
                String cmd = "PD:SET " + String(voltajes[i]);
                handleSCPI(cmd);
                Serial.println("Voltaje en: " + String(voltajes[i]) + " V");

                sendJSON["debug"] = "Voltaje " + String(voltajes[i]) + " V";
                serializeJson(sendJSON, PagWeb);
                PagWeb.println();
                sendJSON.clear();

                delay(delay_ms);
              }

              Serial.println("Sweep finalizated...");  // Mensaje debug en Serial
              sendJSON["debug"] = "Sweep finalizated";

            } else {
              Serial.println("HUSB238 NO inicializado...");  // Mensaje debug en Serial
              sendJSON["debug"] = "HSUB no inicializado";
            }

            serializeJson(sendJSON, PagWeb);
            PagWeb.println();
            break;
          }

        case 4:
          {
            sendJSON.clear();

            if (Value != "") {

              // Verificamos que el módulo esté inicializado para evitar errores
              if (state_husb) {
                Serial.println("Configurando voltaje fijo a: " + Value + " V");

                // Aprovechamos tu función handleSCPI para enviar el comando
                String cmd = "PD:SET " + Value;
                handleSCPI(cmd);

                sendJSON["debug"] = "Voltaje fijo seteado a " + Value + " V";
              } else {
                Serial.println("HUSB238 NO inicializado...");
                sendJSON["debug"] = "Error: HSUB no inicializado";
              }

            } else {
              // Se recibió {"Function": "fixed"} sin clave "Value"
              Serial.println("Error: JSON no contiene el valor de voltaje");
              sendJSON["debug"] = "Error: Falta parametro Value";
            }

            serializeJson(sendJSON, PagWeb);
            PagWeb.println();

            break;
          }

        case 5:
          {
            ESP.restart();
            delay(1000);

            sendJSON.clear();
            Serial.println("Reinicio de dispositivo");
            sendJSON["debug"] = "Reinicio de dispositivo...";
            serializeJson(sendJSON, PagWeb);
            PagWeb.println();
            break;
          }


        default: break;
      }
    }
  }
}

void handleSCPI(String c) {
  c.trim();
  c.toUpperCase();

  if (c == "*IDN?") {
    Serial.println("UNIT-DEVLAB,HUSB238,USBPD,1.0");
  }

  else if (c == "STAT?") {
    Serial.println(husb238.isAttached() ? "ATTACHED" : "UNATTACHED");
  }

  else if (c == "PD:LIST?") {
    HUSB238_PDSelection voltages[] = { PD_SRC_5V, PD_SRC_9V, PD_SRC_12V, PD_SRC_15V, PD_SRC_18V, PD_SRC_20V };
    int voltageValues[] = { 5, 9, 12, 15, 18, 20 };

    for (int i = 0; i < 6; i++) {
      if (husb238.isVoltageDetected(voltages[i])) {
        Serial.print(voltageValues[i]);
        Serial.print("V ");
      }
    }
    Serial.println();
  }

  else if (c == "PD:GET?") {
    Serial.print("PD=");
    Serial.println(husb238.getPDSrcVoltage());
  }

  else if (c.startsWith("PD:SET")) {
    int v = c.substring(6).toInt();
    HUSB238_PDSelection sel;

    switch (v) {
      case 5: sel = PD_SRC_5V; break;
      case 9: sel = PD_SRC_9V; break;
      case 12: sel = PD_SRC_12V; break;
      case 15: sel = PD_SRC_15V; break;
      case 18: sel = PD_SRC_18V; break;
      case 20: sel = PD_SRC_20V; break;
      default:
        Serial.println("ERR:INVALID_VOLTAGE");
        return;
    }

    if (husb238.isVoltageDetected(sel)) {
      husb238.selectPD(sel);
      husb238.requestPD();
      Serial.print("OK:SET ");
      Serial.print(v);
      Serial.println("V");
    } else {
      Serial.println("ERR:UNAVAILABLE");
    }
  }

  else if (c == "PD:SWEEP") {
    HUSB238_PDSelection levels[] = {
      PD_SRC_5V, PD_SRC_9V, PD_SRC_12V,
      PD_SRC_15V, PD_SRC_18V, PD_SRC_20V
    };

    for (int i = 0; i < 6; i++) {
      if (husb238.isVoltageDetected(levels[i])) {
        husb238.selectPD(levels[i]);
        husb238.requestPD();
        Serial.print("SWEEP ");
        Serial.print((i + 1) * 5);
        Serial.println("V");
        delay(1500);
      }
    }
    Serial.println("SWEEP DONE");
  }

  else if (c == "CURR:GET?") {
    HUSB238_CurrentSetting curr = husb238.getPDSrcCurrent();
    Serial.print("CURR=");
    printCurrentValue(curr);
    Serial.println();
  }

  else if (c.startsWith("CURR:MAX?")) {
    int v = c.substring(9).toInt();
    HUSB238_PDSelection sel;

    switch (v) {
      case 5: sel = PD_SRC_5V; break;
      case 9: sel = PD_SRC_9V; break;
      case 12: sel = PD_SRC_12V; break;
      case 15: sel = PD_SRC_15V; break;
      case 18: sel = PD_SRC_18V; break;
      case 20: sel = PD_SRC_20V; break;
      default:
        Serial.println("ERR:INVALID_VOLTAGE");
        return;
    }

    if (husb238.isVoltageDetected(sel)) {
      HUSB238_CurrentSetting curr = husb238.currentDetected(sel);
      Serial.print("MAX_CURR@");
      Serial.print(v);
      Serial.print("V=");
      printCurrentValue(curr);
      Serial.println();
    } else {
      Serial.println("ERR:UNAVAILABLE");
    }
  }

  else {
    Serial.println("ERR:UNKNOWN_CMD");
  }
}

void printCurrentValue(HUSB238_CurrentSetting curr) {
  switch (curr) {
    case CURRENT_0_5_A: Serial.print("0.5A"); break;
    case CURRENT_0_7_A: Serial.print("0.7A"); break;
    case CURRENT_1_0_A: Serial.print("1.0A"); break;
    case CURRENT_1_25_A: Serial.print("1.25A"); break;
    case CURRENT_1_5_A: Serial.print("1.5A"); break;
    case CURRENT_1_75_A: Serial.print("1.75A"); break;
    case CURRENT_2_0_A: Serial.print("2.0A"); break;
    case CURRENT_2_25_A: Serial.print("2.25A"); break;
    case CURRENT_2_50_A: Serial.print("2.50A"); break;
    case CURRENT_2_75_A: Serial.print("2.75A"); break;
    case CURRENT_3_0_A: Serial.print("3.0A"); break;
    case CURRENT_3_25_A: Serial.print("3.25A"); break;
    case CURRENT_3_5_A: Serial.print("3.5A"); break;
    case CURRENT_4_0_A: Serial.print("4.0A"); break;
    case CURRENT_4_5_A: Serial.print("4.5A"); break;
    case CURRENT_5_0_A: Serial.print("5.0A"); break;
    default: Serial.print("UNKNOWN"); break;
  }
}


void printCurrentSetting(HUSB238_CurrentSetting srcCurrent) {
  switch (srcCurrent) {
    case CURRENT_0_5_A:
      Serial.print("0.5A ");
      break;
    case CURRENT_0_7_A:
      Serial.print("0.7A ");
      break;
    case CURRENT_1_0_A:
      Serial.print("1.0A ");
      break;
    case CURRENT_1_25_A:
      Serial.print("1.25A ");
      break;
    case CURRENT_1_5_A:
      Serial.print("1.5A ");
      break;
    case CURRENT_1_75_A:
      Serial.print("1.75A ");
      break;
    case CURRENT_2_0_A:
      Serial.print("2.0A ");
      break;
    case CURRENT_2_25_A:
      Serial.print("2.25A ");
      break;
    case CURRENT_2_50_A:
      Serial.print("2.50A ");
      break;
    case CURRENT_2_75_A:
      Serial.print("2.75A ");
      break;
    case CURRENT_3_0_A:
      Serial.print("3.0A ");
      break;
    case CURRENT_3_25_A:
      Serial.print("3.25A ");
      break;
    case CURRENT_3_5_A:
      Serial.print("3.5A ");
      break;
    case CURRENT_4_0_A:
      Serial.print("4.0A ");
      break;
    case CURRENT_4_5_A:
      Serial.print("4.5A ");
      break;
    case CURRENT_5_0_A:
      Serial.print("5.0A ");
      break;
    default:
      break;
  }
}