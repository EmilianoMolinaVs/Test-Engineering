


/*
*IDN?
SYST:REM
INP 0
INP 1
*CLS
SYST:ERR?
FUNC CURRent
CURRent:RANGe 30
CURRent 0.5

*/

#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include <Arduino.h>

#define RX2 4  // GPIO04 como RXD
#define TX2 5  // GPIO05 como TXD

HardwareSerial USB(1);

String JSON_entrada;
StaticJsonDocument<256> receiveJSON;

String JSON_salida;
StaticJsonDocument<256> sendJSON;

void setup() {
  Serial.begin(115200);
  USB.begin(9600, SERIAL_8N1, RX2, TX2);
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();
    delay(100);
    USB.write(cmd);
  }

  if (USB.available()) {
    char cmd = USB.read();
    delay(100);
    Serial.write(cmd);
  }
}
