
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HardwareSerial.h>

#define RX_PIN 4
#define TX_PIN 5

HardwareSerial UART(1);
StaticJsonDocument<128> receiveJSON;
StaticJsonDocument<128> sendJSON;


void serialDebug(String cmd) {
  StaticJsonDocument<128> doc;
  doc["debug"] = cmd;
  serializeJson(doc, Serial);
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  UART.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
  delay(1000);

  serialDebug("Hola pulsar C6");
}

void loop() {
  /*
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    sendJSON.clear();
    sendJSON["Serial"] = input;
    serializeJson(sendJSON, UART);
    UART.println();
  }
*/

  if (UART.available()) {
    String input = UART.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, input);

    if (!error) {
      String Function = receiveJSON["Function"];
      if (Function == "ping") {
        sendJSON.clear();
        sendJSON["ping"] = "pong";
        sendJSON["status"] = "wireless communication validated";
        serializeJson(sendJSON, UART);
        UART.println();
      }
    }
  }
}
