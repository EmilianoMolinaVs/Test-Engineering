_example /***************************************************************
 * @file    CAN_Transmitter.ino
 * @author  Jonathan Mejorado Lopez
 * @brief   Minimal CAN transmitter example for the DevLab TCAN1051HVD
 *
 * Features
 * - CAN init on default pins
 * - Send a message with a counter value
 *
 * Notes
 * - The CAN transceiver must be properly connected
 * - The message will be sent every second
 *
 * Wiring (ESP32 C6 Probe default CAN)
 * - TX   -> GPIO06
 * - RX   -> GPIO07
 * - VCC  -> 3V3
 * - GND  -> GND
 ***************************************************************/
#include <DevLab_TCAN1051HVD.h>

#define CAN_TX_PIN GPIO_NUM_6
#define CAN_RX_PIN GPIO_NUM_7

  DevLab_TCAN1051HVD can(CAN_TX_PIN, CAN_RX_PIN);

uint8_t contador = 0;

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
  uint8_t data[1] = { contador };

  if (can.send(0x100, data, 1, 0, 0, 1000)) {
    Serial.print("Enviado: ");
    Serial.println(contador);
    contador++;
  } else {
    Serial.println("No se pudo enviar el mensaje");
  }

  delay(1000);
}
