# TestBench UE0094 - ICP-10111 Barometric Pressure Sensor

## Descripción

Integración y pruebas para el sensor de presión y temperatura ICP-10111. El `mainCodeJSONUE0094` expone una interfaz JSON por UART2 que solicita lecturas y devuelve promedios de temperatura y presión.

## Estructura principal

- `mainCodeJSONUE0094/mainCodeJSONUE0094.ino` — Sketch principal que realiza:
  - Detección del sensor en la dirección I2C `0x63`
  - Lecturas repetidas (10 muestras) para calcular promedios
  - Respuesta JSON con `temp`, `press` y `status`
- `codigoPruebaUE0094/` — Códigos de prueba auxiliares

## Interfaz / JSON

- `{"Function":"Lectura"}` → Inicia ciclo de 10 mediciones; devuelve JSON:
  - `temp`: "XX.X °C"
  - `press`: "XXXXX Pa"
  - `status`: `OK` o `Fail` (según rangos definidos)

## Wiring / Pines

- UART2: RX = D4, TX = D5
- I2C SDA = GPIO6, SCL = GPIO7 (sensor en 0x63)

## Dependencias

- `ArduinoJson`
- Biblioteca específica `ue_i2c_icp_10111_sen.h` incluida en el sketch

## Uso

1. Cargar `mainCodeJSONUE0094.ino` en la Pulsar C6.
2. Enviar `{"Function":"Lectura"}` por UART2; esperar JSON respuesta.

---

**Última actualización**: Febrero 2026
