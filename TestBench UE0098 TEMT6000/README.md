# TestBench UE0098 - TEMT6000 Light Sensor

## Descripción

Integración para el sensor TEMT6000 en modo analógico/digital. El `mainCode_JSON_UE0098_TEMT6000` expone pruebas vía JSON por UART2 y devuelve porcentajes y estados (`Env`, `IR`).

## Estructura principal

- `mainCode_JSON_UE0098_TEMT6000/mainCode_JSON_UE0098_TEMT6000.ino` — Sketch principal que implementa:
  - Lecturas analógicas del pin del sensor
  - Rutinas `env` (ambient) e `IR` (infrarrojo) que promedian 10 muestras
  - Envío de JSON con `Meas_Env`/`Meas_IR` y estado `OK`/`Fail`
- Otras carpetas de código en el TestBench

## Interfaz / JSON

- `{"Function":"env"}` → Prueba ambiental; devuelve `{"Env":"OK|Fail","Meas_Env":"XX.XX %"}`
- `{"Function":"IR"}` → Prueba IR; devuelve `{"IR":"OK|Fail","Meas_IR":"XX.XX %"}`

## Wiring / Pines

- UART2: RX = D4, TX = D5
- Sensor analog pin: `SENSOR_PIN` = GPIO4 (ajustable)
- Relevador de puerto COM: `RELAYCom` = GPIO20

## Dependencias

- `ArduinoJson`

## Uso

1. Cargar `mainCode_JSON_UE0098_TEMT6000.ino` en la Pulsar C6.
2. Enviar `{"Function":"env"}` o `{"Function":"IR"}` por UART2; esperar JSON respuesta.

---

**Última actualización**: Febrero 2026
