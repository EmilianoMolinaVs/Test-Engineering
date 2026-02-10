# TestBench UE0082 - G6K2GYTR 5V Relay Module

## Descripción

Banco de pruebas para el módulo de relevadores G6K2GYTR (5V). El proyecto incluye código Arduino que recibe comandos JSON desde la interfaz de pruebas (UART2 / "PagWeb") y controla múltiples relés, además de medir corriente con INA219.

## Estructura principal

- `Códigos ARDUINO/IntegracionJSON/IntegracionJSON.ino` — Sketch principal que integra:
  - Control de relés (varios pines definidos)
  - Lectura del sensor INA219 por I2C
  - Comunicación JSON por `PagWeb` (UART2)
- `Códigos ARDUINO/codigoPrueba/` — Códigos de prueba adicionales

## Interfaz / JSON

El `IntegracionJSON.ino` espera JSON terminados en `\n` por UART2. Campos `Function` soportados (ejemplos):
- `{"Function":"ON Rele"}` → Activa el relé de señal (`signalRele`)\n- `{"Function":"OFF Rele"}` → Desactiva el relé de señal\n- `{"Function":"Fuente ON"}` / `{"Function":"Fuente OFF"}` → Control de relés de fuente

El firmware no envía un JSON de confirmación explícito para cada comando, pero escribe logs por `Serial` USB para depuración.

## Wiring / Pines (ejemplo en el sketch)

- UART2 RX: GPIO15 (`RX2` / D4)
- UART2 TX: GPIO19 (`TX2` / D5)
- I2C SDA: GPIO6
- I2C SCL: GPIO7
- Relevadores: pines `RELAYA`, `RELAYB`, `RELAY1`..`RELAY4`, `signalRele` (ver sketch para asignaciones exactas)

## Dependencias

- `Adafruit_INA219` (sensor de corriente)
- `ArduinoJson`

## Uso

1. Cargar `IntegracionJSON.ino` en la Pulsar C6.
2. Enviar comandos JSON por UART2 desde la interfaz de pruebas.
3. Ver logs por `Serial Monitor` a 115200 bps.

## Notas

- Diseñado para integrarse con la `PagWeb` que envía los JSON. No cuenta con interfaz web propia.
- Ajustar pines en el sketch si su hardware difiere.

---

**Última actualización**: Febrero 2026
