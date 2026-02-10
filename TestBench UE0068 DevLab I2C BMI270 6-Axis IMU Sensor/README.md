# TestBench UE0068 - BMI270 6-Axis IMU Sensor

## Descripción general

Conjunto de herramientas y ejemplos para integrar y probar el sensor IMU BMI270 (6-axis) en la plataforma Pulsar C6. La carpeta `Códigos C` contiene cuatro subcarpetas: `mainCode_UE0068_BMI271`, `Scanner_I2C_example`, `UE0068_Test_I2C` y `UE0068_Test_SPI`.

- `mainCode_UE0068_BMI271` — Programa principal que se comunica con la interfaz de pruebas vía JSON (UART2). Soporta lectura por I2C en ambas direcciones posibles (0x68/0x69) y modo SPI.
- `Scanner_I2C_example` — Escáner auxiliar que recorre el bus I2C para listar direcciones detectadas (útil para debugging y fabricación).
- `UE0068_Test_I2C` — Código específico para operar el BMI270 por I2C (ejemplo de lectura continuo). Código probado en línea de producción.
- `UE0068_Test_SPI` — Código específico para operar el BMI270 por SPI (ejemplo de lectura continuo).

> Estado: El código I2C en C es el de referencia y está probado en producción. No hay interfaz web integrada; la comunicación con la "PagWeb" se realiza por UART/JSON.

## Dependencias (Arduino)

Instalar en Arduino IDE:
- `SparkFun_BMI270_Arduino_Library` (o librería equivalente que provea `BMI270`/`BMI2_OK`)
- `ArduinoJson`
- `Wire` y `SPI` (normalmente incluidas)

## Wiring / Pines (tal como están en los ejemplos)

Comunes:
- I2C SDA → GPIO 6
- I2C SCL → GPIO 7
- SDO pin → `D1` (se usa para seleccionar 0x68/0x69)
- CS pin → `D0`
- Relay (opcional en `mainCode`) → `D5`
- Botón Run (opcional) → `D4`

UART (PagWeb / interfaz JSON):
- `PagWeb` / UART2 RX → `D4` (macro `RX2`)
- `PagWeb` / UART2 TX → `D5` (macro `TX2`)

SPI (ejemplo):
- SCK → pin 7 (en `UE0068_Test_SPI` se usa `SPI.begin(7, D1, 6, D0)`)
- MOSI → pin 6
- MISO → pin D1
- CS → D0

Ajustar pines según su placa si fuera necesario.

## `mainCode_UE0068_BMI271` — descripción detallada

El `mainCode` es el programa que se enlaza a la interfaz de pruebas por medio de JSON. Principales comportamientos:

- Inicializa `PagWeb` como `HardwareSerial(1)` a `115200` (pines `RX2`/`TX2` definidos en el código).
- Escucha mensajes JSON terminados en `\n` desde la interfaz.
- Soporta las siguientes funciones (campo JSON `Function`):
  - `{"Function":"scan"}`: realiza un escaneo I2C en ambas configuraciones de SDO (0x68 y 0x69). Para cada dirección intenta inicializar el BMI270 y si lo detecta lee datos (10 muestras) y envía resultados por JSON.
  - `{"Function":"SPI"}`: intenta inicializar el BMI270 por SPI y, si es exitoso, lee muestras y envía `{"SPI":"OK"}` (o `{"SPI":"Fail"}` en fallo).
  - `{"Function":"relayOn"}` / `{"Function":"relayOff"}`: controla un relé físico (pin `RELAY`) usado para cambiar estados de CS o alimentar circuitos de prueba.
  - `{"Function":"ping"}`: responde `{"ping":"pong"}` para comprobar conectividad.

Cuando se detecta el IMU y se leen datos, el `mainCode` envía por `PagWeb` un JSON que contiene campos como:
- `accelX`, `accelY`, `accelZ` — aceleraciones (g)
- `gyroX`, `gyroY`, `gyroZ` — giroscopio (deg/s)
- `i2c0x68`, `i2c0x69` — indicadores `OK`/`Fail` para cada dirección
- `addr` — dirección I2C detectada (si procede)
- `SPI` — `OK`/`Fail` en caso de prueba SPI

Ejemplo de petición y respuesta (scan):
Petición:

```json
{"Function":"scan"}
```

Posible respuesta parcial (ejemplo):

```json
{"i2c0x68":"OK","addr":"0x68","accelX":0.001234,"gyroZ":1.23}
```

Nota: el `mainCode` serializa y envía varios JSON durante la rutina de lectura (envía datos de cada `readIMU()`), por lo que la interfaz debe procesar múltiples líneas JSON.

## `Scanner_I2C_example`

Código sencillo que fuerza el pin `SDO` en LOW y HIGH para comprobar la presencia de dispositivos I2C en las direcciones 0x68 y 0x69. Útil para verificar cableado y que el módulo responde en la dirección esperada.

## `UE0068_Test_I2C` y `UE0068_Test_SPI`

Ejemplos dedicados que inicializan el sensor en el bus correspondiente y publican lecturas por `Serial` de forma continua. `UE0068_Test_I2C` intenta inicializar en 0x68 y 0x69 y se queda bloqueado si no detecta el sensor (comportamiento esperado para pruebas de producción).

## Comportamiento importante a considerar

- Reinicio requerido para cambios de protocolo: después de completar una lectura/operación del sensor (especialmente si se cambia entre I2C y SPI o si se manipulan las líneas SDO/CS), es importante **reiniciar la Pulsar** o **cerrar y volver a abrir el monitor serie** de la interfaz de pruebas. Esto asegura que los protocolos se re-inicialicen correctamente y evita estados inconsistentes del bus.

- Motivo técnico: cambiar el estado físico de SDO/CS o el modo (I2C ↔ SPI) puede dejar al bus en un estado en el que `Wire`/`SPI` no se re-inicializan correctamente sin reinicio, por lo que la sesión serie/prueba debe reiniciarse para garantizar lecturas fiables.

## Ejemplos de uso rápido

- Escanear y leer I2C (ambas direcciones): enviar `{"Function":"scan"}\n` por UART a la `PagWeb` (UART2).
- Probar SPI: enviar `{"Function":"SPI"}\n`.
- Encender relé: `{"Function":"relayOn"}\n`.
- Apagar relé: `{"Function":"relayOff"}\n`.
- Ping: `{"Function":"ping"}\n`.

Asegúrese de enviar `\n` al final de cada JSON.

## Troubleshooting

- Si no se detecta el sensor en I2C:
  - Verificar wiring (SDA=SDA_PIN=6, SCL=SCL_PIN=7), alimentación y que SDO/CS estén en estado correcto.
  - Ejecutar `Scanner_I2C_example` para comprobar direcciones visibles en el bus.
  - Desconectar cualquier SPI activo al probar por I2C.

- Si no se detecta en SPI:
  - Verificar conexiones SPI y CS.
  - Revisar que no haya conflicto con I2C (CS a GND pone el módulo en modo SPI).

- Si los resultados aparecen inconsistentes entre cambios de modo, reiniciar la Pulsar o cerrar/reabrir el monitor serie antes de volver a ejecutar pruebas.

## Recomendaciones de integración

- En la interfaz de pruebas, trate cada operación (`scan`, `SPI`) como una transacción independiente: enviar petición, esperar varias líneas JSON de respuesta (o timeout), luego inducir reinicio del dispositivo antes de cambiar el modo de comunicación.
- Considerar agregar un comando JSON que fuerce un soft-restart en el firmware para no depender del monitor serie (opcional mejora).

## Licencia

Proyecto interno - TestBench Engineering

---

**Última actualización**: Febrero 2026
