/*
Firmware principal de teste para el proyecto pc0707 de enersi con sensores de corriente
ina219 sensados por un attiny85 
*/

#include <SoftwareSerial.h>
#include <Wire.h>

SoftwareSerial vrserial(3, 4);  // RX, TX

void setup() {
  vrserial.begin(9600);
  delay(200);

  // Limpieza de buffer
  while (vrserial.available() > 0) {
    vrserial.read();
  }

  vrserial.println("Hola Mundo");

  Wire.begin();
  delay(100);
}

void loop() {
  if (vrserial.available()) {

    String input = vrserial.readStringUntil('\n');
    input.trim();

    // Filtro extra: ignorar si quedó vacío o si son caracteres invisibles/basura
    if (input.length() == 0) {
      vrserial.println("mensaje vaciooOooooo");
      return;  // Aborta este ciclo y vuelve a empezar
    }

    int opc = 0;
    if (input == "ping") opc = 1;
    else if (input == "scan") opc = 2;

    switch (opc) {
      case 1:
        vrserial.println("{\"ping\":\"pong\"}");
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
              // vrserial.print("I2C device found at 0x");
              // if (addr < 16) vrserial.print("0");
              // vrserial.println(addr, HEX);

              if (addr == 0x40) found40 = true;
              else if (addr == 0x41) found41 = true;
              else if (addr == 0x44) found44 = true;
              else if (addr == 0x45) found45 = true;
            }
          }

          if (found40) vrserial.println("{\"0x40\":\"true\"}");
          if (found41) vrserial.println("{\"0x41\":\"true\"}");
          if (found44) vrserial.println("{\"0x44\":\"true\"}");
          if (found45) vrserial.println("{\"0x45\":\"true\"}");

          if (found40 && found41 && found44 && found45) {
            vrserial.println("{\"Result\":\"OK\"}");
          }


          break;
        }


      default:
        vrserial.print("invalid option: [");
        vrserial.print(input);
        vrserial.println("]");
        break;
    }
  }
}
