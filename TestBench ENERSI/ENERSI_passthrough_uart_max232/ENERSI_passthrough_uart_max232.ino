
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HardwareSerial.h>

#define RX_PIN 4  // >> GPIO04 asignado a UART2
#define TX_PIN 5  // >> GPIO05 asignado a UART2

HardwareSerial UART(1);
StaticJsonDocument<128> sendJSON;
StaticJsonDocument<128> receiveJSON;

void serialDebug(String cmd) {
  StaticJsonDocument<128> doc;
  doc.clear();
  doc["debug"] = cmd;
  serializeJson(doc, Serial);
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  UART.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
  delay(100);

  serialDebug("uart passthrough initialized...");
}

void loop() {

  if (UART.available()) {
    String inJSON = UART.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, inJSON);

    String Function = receiveJSON["Function"];
    int opc = 0;
    if (Function == "ping") opc = 1;  // {"Function":"ping"}

    switch (opc) {
      case 1:
        {
          sendJSON.clear();
          serialDebug("ping received");
          sendJSON["Result"] = "OK";
          sendJSON["msg"] = "Hi MAX232! :D";
          serializeJson(sendJSON, UART);
          UART.println();
          break;
        }
    }
  }
}
