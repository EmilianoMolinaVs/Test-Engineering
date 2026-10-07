#include <SoftwareSerial.h>

#define RX_PIN 3
#define TX_PIN 4
#define BLINK_PIN 1  // >> BLINK en GPIO PB1

SoftwareSerial miSerial(RX_PIN, TX_PIN);  // RX, TX

// Variables para millis()
unsigned long previousMillis = 0;
const long interval = 1000;
bool ledState = HIGH;

void setup() {
  miSerial.begin(9600);
  miSerial.println("Hola Mundo");

  pinMode(BLINK_PIN, OUTPUT);
  digitalWrite(BLINK_PIN, HIGH);
}

void loop() {

  // 1. Escucha Serial No Bloqueante
  if (miSerial.available() > 0) {
    String input = miSerial.readStringUntil('\n');
    input.trim();

    if (input == "ping") {
      miSerial.println("{\"Result\":\"OK\"}");
    } else {
      miSerial.println("FAIL: invalid option");
    }
  }

  // 2. Parpadeo No Bloqueante
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    ledState = !ledState;
    digitalWrite(BLINK_PIN, ledState);
  }
}