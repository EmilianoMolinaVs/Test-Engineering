
/*
Firmware Test para Placa base de ENERSI
*/

#define LED_1 13
#define LED_2 7

void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(LED_1, HIGH);
  digitalWrite(LED_2, HIGH);
  delay(1000);
  digitalWrite(LED_1, LOW);
  digitalWrite(LED_2, LOW);
  delay(1000);
}
