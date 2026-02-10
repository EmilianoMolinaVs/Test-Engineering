# TestBench UE0066 - BMM150 Magnetometer (I2C / SPI)

## Descripción general

TestBench para el sensor magnético BMM150. Este proyecto incluye:
- Código en C (Arduino/ESP) para lectura por I2C (`test_i2c_bmm150_magnetometer.ino`) — probado y en uso en línea de producción.
- Scripts en Python/MicroPython para autodetección de interfaz (I2C o SPI) y lectura (`bmm150_auto_interface.py`, `main.py`).

No existe interfaz web ni página de pruebas; el propósito es facilitar la integración del sensor en la Pulsar C6 y en los procesos de producción.

## Estado

- Lectura I2C en C: probado y utilizado en producción.
- Autodetección Python: disponible y útil para diagnóstico y entornos de desarrollo.
- Producto final: aún no fabricado, por tanto no hay interfaz de pruebas visual.

## Archivos clave

- `Códigos C/test_i2c_bmm150_magnetometer/test_i2c_bmm150_magnetometer.ino` — Sketch Arduino/ESP que detecta la dirección I2C entre `0x10..0x13`, inicializa el BMM150 y publica lecturas por `Serial`.
- `Códigos Python/AutoDetect/bmm150_auto_interface.py` — Drivers MicroPython para I2C y SPI y clase de auto-detección `BMM150_Auto`.
- `Códigos Python/AutoDetect/main.py` — Ejemplo de uso que publica lecturas por UART y soporta comandos JSON (`read_mag`, `ping`).

## Notas de Hardware / Wiring

- BMM150 puede operar por **I2C** o **SPI**, pero no ambos simultáneamente en el mismo módulo (CS y SDO compartidos).
- Pines usados en los ejemplos:
  - I2C SDA → GPIO 6
  - I2C SCL → GPIO 7
  - SPI SCK → GPIO 20 (ejemplo en `main.py`), MOSI → GPIO 19, MISO → GPIO 2, CS → GPIO 18
- Si el pin CS está a GND, el sensor entra en modo SPI; si CS está alto/float con pull-up, entra en modo I2C.

Recomendaciones:
- No conectar I2C y SPI al mismo tiempo sin aislamiento.
- Usar jumpers o resistencias serie (1 kΩ) para evitar interferencias durante pruebas.

## Direcciones I2C soportadas

El código busca estas direcciones:
- `0x10`, `0x11`, `0x12`, `0x13`

Esto depende del estado de las patillas SDO/CS del módulo.

## Protocolo y ejemplos JSON (MicroPython `main.py`)

El ejemplo `main.py` envía periódicamente por UART un JSON con la lectura:

Ejemplo de salida automática:

```json
{"resp":"OK","iface":"I2C","addr":"0x10","x":123,"y":-45,"z":67}
```

Comandos soportados vía UART en formato JSON (por `main.py`):
- `{"cmd":"read_mag"}` → responde con lectura magnética inmediata
- `{"cmd":"ping"}` → responde `{"resp":"pong"}`

Ejemplo de petición y respuesta:

Petición:

```json
{"cmd":"read_mag"}
```

Respuesta:

```json
{"resp":"OK","iface":"I2C","addr":"0x10","x":123,"y":-45,"z":67}
```

## Dependencias

- Arduino/C: biblioteca `DFRobot_BMM150` (o similar compatible con BMM150), `Wire` (I2C).
- MicroPython: módulo `machine` (I2C, SPI, Pin), `ujson` (o `ujson`), `struct` y `time`.

Instalación de librería Arduino (IDE):
- Sketch → Include Library → Manage Libraries → Buscar `DFRobot_BMM150` e instalar.

## Cómo usar

Arduino (I2C):
1. Abrir `test_i2c_bmm150_magnetometer.ino` en Arduino IDE.
2. Seleccionar placa correcta (ESP32/Pulsar C6) y puerto.
3. Compilar y subir.
4. Abrir `Serial Monitor` a 115200 baudios y observar detección y lecturas.

MicroPython (AutoDetect):
1. Copiar `bmm150_auto_interface.py` y `main.py` a la Pulsar C6 (o placa con MicroPython).
2. Ajustar pines si es necesario en `main.py` (I2C/SPI/CS/ UART pins).
3. Ejecutar `main.py`. El script intentará auto-detectar I2C primero y luego SPI.
4. Leer salidas por UART (115200) o enviar comandos JSON por UART para lecturas a demanda.

## Troubleshooting

- Mensaje `BMM150 no detectado` en sketch C: comprobar wiring y que CS no esté forzado a GND si se desea I2C.
- En autotector Python, si devuelve `No se detectó BMM150 por I2C` y `No se detectó BMM150 por SPI`, verificar que CS y SDO estén en el estado correcto y que no haya bus conflictivo.
- Si se observa ruido o lecturas incorrectas, desconectar el bus SPI si se prueba por I2C (y viceversa).

## Producción y notas adicionales

- El ejemplo en C está probado en la línea de producción para lectura por I2C y es la referencia estable hoy.
- Mantener `Códigos C` como versión de referencia para producción; los scripts Python son útiles para diagnóstico y pruebas en banco.

## Posibles mejoras

- Añadir parámetro de configuración por JSON para cambiar tasa de muestreo (`rate`) o preset mode.
- Añadir registro de calibración y offsets en SD o EEPROM.
- Implementar modo de logging avanzado para la línea de producción.

## Licencia

Proyecto interno - TestBench Engineering

---

**Última actualización**: Febrero 2026
**Autor**: CesarBautista y EmilianoMolina
