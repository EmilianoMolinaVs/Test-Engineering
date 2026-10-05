/*
Este firmware

*/

#include <DevLab_TCAN1051HVD.h>

#define CAN_TX_PIN GPIO_NUM_7
#define CAN_RX_PIN GPIO_NUM_6


DevLab_TCAN1051HVD can(CAN_TX_PIN, CAN_RX_PIN);

void setup() {
  Serial.begin(115200);
  delay(1000);

  if (!can.begin()) {
    Serial.println("Error al iniciar CAN");
    while (true) delay(1000);
  }

  Serial.println("CAN iniciado correctamente");
  Serial.println("Esperando mensajes...");
}

void loop() {
  TCAN1051_Frame frame;

  if (can.receive(frame, 100)) {
    Serial.print("ID: 0x");
    Serial.println(frame.id, HEX);

    Serial.print("Cantidad de bytes: ");
    Serial.println(frame.length);

    if (frame.length > 0) {
      Serial.print("Dato recibido: ");
      Serial.println(frame.data[0]);
    }

    Serial.println();
  }
}
