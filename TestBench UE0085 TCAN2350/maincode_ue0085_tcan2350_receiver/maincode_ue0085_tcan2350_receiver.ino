/*
Este firmware

*/

#include <DevLab_TCAN1051HVD.h>

#define CAN_TX_PIN GPIO_NUM_7
#define CAN_RX_PIN GPIO_NUM_6


DevLab_TCAN1051HVD can(CAN_TX_PIN, CAN_RX_PIN);
String dataInput = "";
int countData = 0;

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
    /*
    Serial.print("ID: 0x");
    Serial.println(frame.id, HEX);

    Serial.print("Cantidad de bytes: ");
    Serial.println(frame.length);
    */

    for (int i = 0; i < frame.length; i++) {
      // Serial.print((char)frame.data[i]);
      dataInput += (char)frame.data[i];
    }

   //Serial.println();
    if (dataInput == "ping") {
      //Serial.println("ola");
      countData++;
    }

    if (countData == 100) {
      Serial.println("100 datos recibidos exitosamente");
      countData = 0;

      const char* text = "OK";
      uint8_t data[2] = { 0 };
      memcpy(data, text, 2);

      delay(1000);
      if (can.send(0x100, data, 2, 0, 0, 1000)) {
        Serial.println("je");
      } else {
        Serial.println("No se pudo enviar el mensaje");
      }
    }

    dataInput = "";
  }
}
