# TestBench UE0095 - BME688 Environmental 4-in-1 Sensor

## Descripción

Proyecto de integración para el sensor BME688 (Temperatura, Presión, Humedad y Gas). El `mainCodeJSONUE0095` maneja el sensor principalmente por SPI en la versión actual y responde a comandos JSON desde la interfaz de pruebas (UART2).

## Estructura principal

- `mainCodeJSONUE0095/mainCodeJSONUE0095.ino` — Sketch principal (SPI) que realiza:
  - Inicialización del sensor por SPI (`mySPI.begin` y `bme.begin(PIN_CS, mySPI)`)
  - Mediciones en modo forzado (10 muestras) y promedio
  - Respuesta JSON con `temp`, `press`, `hum`, `gas` y `status`
- `CodigoPruebaUE0095/` — Códigos de prueba auxiliares (I2C examples, etc.)

## Interfaz / JSON

- `{"Function":"Lectura"}` → Ejecuta ciclo de mediciones y devuelve:
  - `temp`: "XX.X °C"
  - `press`: "XXXXX Pa"
  - `hum`: "XX.X %"
  - `gas`: valor de resistencia
  - `status`: `OK`/`Fail`

## Wiring / Pines

- UART2: RX = D4, TX = D5
- SPI: SCK = GPIO6, MOSI = GPIO7, MISO = GPIO2, CS = GPIO3 (según sketch)

## Dependencias

- `bme68xLibrary` (driver BME688)
- `ArduinoJson`

## Uso

1. Cargar `mainCodeJSONUE0095.ino` en la Pulsar C6.
2. Enviar `{"Function":"Lectura"}` por UART2; recibir JSON con medidas.

## Notas

- El sketch incluye rutinas para reconfigurar el heater del sensor y tomar medidas en `BME68X_FORCED_MODE`.
- El proyecto soporta variantes I2C/SPI; ajustar pines y llamadas `begin` según hardware.

---

**Última actualización**: Febrero 2026
