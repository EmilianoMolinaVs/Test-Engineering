/*

Este firmware funciona como main 
*/

// ==== DECLARACIÓN DE LIBRERIAS ====
#include <DevLab_TCAN1051HVD.h>
#include <ArduinoJson.h>

// ==== DECLARACIÓN DE PINES ====
#define CAN_TX_PIN GPIO_NUM_6
#define CAN_RX_PIN GPIO_NUM_7

DevLab_TCAN1051HVD can(CAN_TX_PIN, CAN_RX_PIN);

// ==== CREACIÓN DE OBJETOS ====
StaticJsonDocument<128> receiveJSON;
StaticJsonDocument<128> sendJSON;

// ==== CONSTANTES GLOBALES ====
int contador = 0;
String dataInput = "";

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Iniciando ESP32-H2...");

  if (!can.begin()) {
    Serial.println("Error al iniciar CAN");
    while (true) delay(1000);
  }

  Serial.println("CAN iniciado correctamente");
}

void loop() {

  TCAN1051_Frame frame;

  if (Serial.available()) {
    String inJSON = Serial.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, inJSON);

    if (error) {
      sendJSON["state"] = "FAIL";
      sendJSON["error"] = error.c_str();
      serializeJson(sendJSON, Serial);
      Serial.println();
    } else {

      String Function = receiveJSON["Function"];
      int opc = 0;
      if (Function == "ping") opc = 1;          // {"Function":"ping"}
      else if (Function == "testCAN") opc = 2;  // {"Function":"testCAN"}

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

            const char* text = "ping";
            uint8_t data[4] = { 0 };
            memcpy(data, text, 4);

            for (int i = 0; i < 100; i++) {
              if (can.send(0x100, data, 4, 0, 0, 1000)) {
                Serial.println(contador);
              } else {
                Serial.println("No se pudo enviar el mensaje");
                break;
              }

              contador++;
              delay(50);
            }
            contador = 0;
            break;
          }

        default:
          sendJSON["status"] = "FAIL";
          sendJSON["error"] = "invalid option";
          serializeJson(sendJSON, Serial);
          Serial.println("");
          break;
      }
    }
  }

  if (can.receive(frame, 100)) {

    for (int i = 0; i < frame.length; i++) {
      // Serial.print((char)frame.data[i]);
      dataInput += (char)frame.data[i];
    }


    if (dataInput == "OK") {
      //Serial.println("ola");
      Serial.println("Result OK");
    }
    dataInput = "";
  }
}
