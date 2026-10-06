/*
Firmware principal de teste para el proyecto pc0707 de enersi con sensores de corriente
ina219 sensados por un attiny85 
*/

#include <SoftwareSerial.h>
#include <Wire.h>

SoftwareSerial vrserial(3, 4);  // RX, TX

void setup() {
  vrserial.begin(9600);
  delay(1000);
  vrserial.println("Hola Mundo");

  Wire.begin();
  // Wire.setWireTimeout(3000);
  delay(100);
}

void loop() {
  if (vrserial.available() > 0) {

    String input = vrserial.readStringUntil('\n');
    input.trim();

    int opc = 0;
    if (input == "ping") opc = 1;
    else if (input == "scan") opc = 2;

    switch (opc) {
      case 1:
        vrserial.println("pong");
        break;

      case 2:
        {
          bool found40 = false;
          bool found41 = false;
          bool found44 = false;
          bool found45 = false;

          for (uint8_t addr = 1; addr < 127; addr++) {
            Wire.beginTransmission(addr);
            if (Wire.endTransmission() == 0) {
              vrserial.print("I2C device found at 0x");
              if (addr < 16) vrserial.print("0");
              vrserial.println(addr, HEX);

              if (addr == 0x40) found40 = true;
              else if (addr == 0x41) found41 = true;
              else if (addr == 0x44) found44 = true;
              else if (addr == 0x45) found45 = true;
            }
          }
          vrserial.println("Fin del escaner");

          // ---- IMPRIMIR EL JSON A LA SALIDA ----
          vrserial.println("--- JSON STATUS ---");
          vrserial.print("{");

          vrserial.print("\"0x40\":");
          vrserial.print(found40 ? "true" : "false");
          vrserial.print(",");

          vrserial.print("\"0x41\":");
          vrserial.print(found41 ? "true" : "false");
          vrserial.print(",");

          vrserial.print("\"0x44\":");
          vrserial.print(found44 ? "true" : "false");
          vrserial.print(",");

          vrserial.print("\"0x45\":");
          vrserial.print(found45 ? "true" : "false");

          vrserial.println("}");
          vrserial.println("-------------------");
        }
        break;

      default:
        vrserial.print("invalid option: [");
        vrserial.print(input);
        vrserial.println("]");
        break;
    }
  }
}
