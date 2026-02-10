# TestBench UE0083 - AO4410 2-Channel PWM Output Module

## Descripción

Control y verificación del módulo AO4410 para salida PWM en dos canales. El `Main_IntegracionJSON_PWM.ino` expone pruebas automatizadas que varían el duty-cycle y miden corriente con INA219, comunicándose con la interfaz de pruebas vía JSON.

## Estructura principal

- `Códigos Arduino/Main_IntegracionJSON_PWM/Main_IntegracionJSON_PWM.ino` — Sketch principal con rutinas PWM y lectura INA219
- `Códigos Arduino/codigoPrueba/` — Códigos de prueba adicionales

## Interfaz / JSON

Comandos JSON esperados por `PagWeb` (UART2):
- `{"Function":"PWM_1"}` → Ejecuta la rutina de validación para canal PWM_1
- `{"Function":"PWM_2"}` → Ejecuta la rutina de validación para canal PWM_2
- `{"Function":"Max"}` → Rutina de prueba máxima

El sketch responde con un JSON que contiene valores de corriente para distintos duty-cycles y un campo `status` (`OK`/`Fail`).

## Wiring / Pines (según sketch)

- UART2: RX = D4, TX = D5
- I2C (INA219): SDA = GPIO6, SCL = GPIO7
- PWM outputs: pines `PWM_1`, `PWM_2` (ej. GPIO4, GPIO5)
- Relevadores de conmutación: `RELAYA`, `RELAYB`, `RELAYPWM1`, `RELAYPWM2`

## Dependencias

- `Adafruit_INA219`
- `ArduinoJson`

## Uso

1. Cargar el sketch `Main_IntegracionJSON_PWM.ino` en la Pulsar C6.
2. Enviar los comandos JSON por UART2 y recibir el JSON de resultados.
3. Verificar `status` y valores de corriente devueltos.

---

**Última actualización**: Febrero 2026
