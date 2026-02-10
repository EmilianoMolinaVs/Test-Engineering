# TestBench UE0087 - TPS61023 Boost Converter Module

## Descripción

Banco de pruebas para el convertidor boost TPS61023. El `MainCode_JSON_UE0087` controla relés y mide corriente de salida con INA219, exponiendo una interfaz JSON por UART2 para la `PagWeb`.

## Estructura principal

- `MainCode_JSON_UE0087/MainCode_JSON_UE0087.ino` — Sketch principal que implementa:
  - Lecturas de corriente de salida (10 muestras y selección de máximo)
  - Control de relés de fuente
  - Comandos JSON: `Lectura`, `Fuente ON`, `Fuente OFF`
- Otros recursos y código de prueba en la carpeta raíz del TestBench

## Interfaz / JSON

- `{"Function":"Lectura"}` → Realiza 10 mediciones y responde con `{"meas": "X.XX A", "status": "OK|Fail"}`
- `{"Function":"Fuente ON"}` / `{"Function":"Fuente OFF"}` → Control de relés

El sketch envía su respuesta serializada por `PagWeb` y termina con un `\n` para delimitar mensajes.

## Wiring / Pines

- UART2: RX = D4, TX = D5
- I2C (INA219): SDA = GPIO6, SCL = GPIO7
- Relés: múltiples pines (ver sketch para asignaciones exactas)
- Botón de arranque: `RUN_BUTTON` (D4)

## Dependencias

- `Adafruit_INA219`
- `ArduinoJson`

## Uso

1. Cargar `MainCode_JSON_UE0087.ino` en la Pulsar C6.
2. Enviar `{"Function":"Lectura"}` por UART2 para obtener mediciones de corriente.

---

**Última actualización**: Febrero 2026
