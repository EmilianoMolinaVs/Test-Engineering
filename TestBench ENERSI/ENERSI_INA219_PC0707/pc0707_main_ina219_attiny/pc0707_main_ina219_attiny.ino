/*
Firmware principal de teste para el proyecto pc0707 de enersi con sensores de corriente
ina219 sensados por un attiny85 
*/

// ==== DECLARACIÓN DE LIBRERIAS ====
#include <SoftwareSerial.h>
#include <Wire.h>

// ==== CREACIÓN DE OBJETOS ====
SoftwareSerial vrserial(3, 4);  // PINES RX y TX

/*
Adafruit_INA219 ina219_u1(0x40);  // Sensor de corriente INA219 U1
Adafruit_INA219 ina219_u2(0x41);  // Sensor de corriente INA219 U2
Adafruit_INA219 ina219_u3(0x44);  // Sensor de corriente INA219 U3
Adafruit_INA219 ina219_u4(0x45);  // Sensor de corriente INA219 U4
*/


// ==== FUNCIONES DE UTILIDAD ====
/*
float voltageSensor() {
  // float shunt_mV = ina219_u1.getShuntVoltage_mV();
  float bus_V = ina219_u1.getBusVoltage_V();

  // shunt_mV -= shuntOffset_mV;
  // float shunt_V = shunt_mV / 1000.0;
  // float current_A = shunt_V / R_SHUNT;  // Corriente de interés
  // float load_V = bus_V + shunt_V;
  // float power_W = load_V * current_A;
  return bus_V;
}
*/

float readINA219BusVoltage(uint8_t i2c_addr) {
  Wire.beginTransmission(i2c_addr);
  Wire.write(0x02);

  // Agregar 'false' mantiene el bus activo (Repeated Start)
  if (Wire.endTransmission(false) != 0) {
    return -1.0;
  }

  Wire.requestFrom(i2c_addr, (uint8_t)2);
  if (Wire.available() == 2) {
    uint16_t value = (Wire.read() << 8) | Wire.read();
    value >>= 3;
    return value * 0.004;
  }
  return -1.0;
}

void setup() {

  // ==== Inicialización de Serial Virtual ====s
  vrserial.begin(9600);
  delay(200);

  while (vrserial.available() > 0) {  // Limpieza de buffer
    vrserial.read();
  }

  // ==== Inicialización de BUS I2C ====
  Wire.begin();
  delay(100);

  // ---- Attiny inicializado correctamente... ----
  vrserial.println("Hi World...");
}
void loop() {
  if (vrserial.available()) {

    char input[15];  // Buffer pequeño para el comando
    // Leer hasta el salto de línea o hasta llenar el buffer
    int len = vrserial.readBytesUntil('\n', input, sizeof(input) - 1);
    input[len] = '\0';  // Terminar el string de C

    // Limpiar retorno de carro '\r' si tu monitor serial lo envía (equivalente a trim)
    if (len > 0 && input[len - 1] == '\r') {
      input[len - 1] = '\0';
      len--;
    }

    // Filtro extra
    if (len == 0) {
      vrserial.println("mensaje vaciooOooooo");
      return;
    }

    int opc = 0;
    // Uso de strcmp para comparar arreglos de caracteres
    if (strcmp(input, "ping") == 0) opc = 1;
    else if (strcmp(input, "scan") == 0) opc = 2;
    else if (strcmp(input, "u1") == 0) opc = 3;

    switch (opc) {
      case 1:
        vrserial.println("{\"ping\":\"pong\"}");
        break;

      case 2:
        {
          bool found40 = false, found41 = false, found44 = false, found45 = false;

          for (uint8_t addr = 1; addr < 127; addr++) {
            Wire.beginTransmission(addr);
            if (Wire.endTransmission() == 0) {
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

      case 3:
        {
          float bus_V = readINA219BusVoltage(0x40);  // Leer directo de la dirección u1

          if (bus_V < 0) {
            vrserial.println("Error I2C en 0x40");
          } else {
            vrserial.print("Voltaje: ");
            vrserial.println(bus_V);
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