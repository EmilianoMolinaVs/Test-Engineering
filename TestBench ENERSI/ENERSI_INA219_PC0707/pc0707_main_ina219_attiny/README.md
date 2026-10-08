# PC0707 - Firmware ATtiny85 + INA219

Firmware de prueba para el proyecto **PC0707 de Enersi**, diseñado para utilizar un **ATtiny85** como controlador de varios sensores de corriente **INA219**.

El ATtiny85 se comunica con los INA219 mediante **I2C** y utiliza `SoftwareSerial` para recibir comandos y enviar resultados a un equipo externo.

## Hardware

### Microcontrolador

* ATtiny85

### Sensores INA219

Se utilizan hasta cuatro sensores INA219, cada uno con una dirección I2C diferente:

| Sensor    | Dirección I2C | Comando |
| --------- | ------------- | ------- |
| INA219 #1 | `0x40`        | `u1`    |
| INA219 #2 | `0x41`        | `u2`    |
| INA219 #3 | `0x44`        | `u3`    |
| INA219 #4 | `0x45`        | `u4`    |

### Comunicación serial

El firmware utiliza:

```cpp
SoftwareSerial vrserial(3, 4);
```

Por lo tanto:

* Pin `3`: RX
* Pin `4`: TX
* Velocidad: `9600 baud`

## Comunicación I2C

La comunicación con los INA219 se realiza mediante la librería:

```cpp
#include <Wire.h>
```

El firmware utiliza las direcciones:

```text
0x40
0x41
0x44
0x45
```

El comando `scan` puede utilizarse para comprobar qué sensores están respondiendo en el bus.

## Lectura de voltaje

La función:

```cpp
readINA219BusVoltage(uint8_t i2c_addr)
```

lee el registro `0x02` del INA219, correspondiente al **Bus Voltage Register**.

El valor leído es de 16 bits. Los 3 bits inferiores se descartan:

```cpp
value >>= 3;
```

Después se convierte a volts utilizando una resolución de:

```text
4 mV/bit
```

por lo que:

```cpp
voltage = value * 0.004
```

La función devuelve el voltaje en volts.

> Nota: el valor de **Bus Voltage** del INA219 corresponde al voltaje del nodo `VIN-` respecto a `GND`.

## Comandos

### `ping`

Comprueba que el ATtiny85 está recibiendo y procesando comandos.

Comando:

```text
ping
```

Respuesta:

```json
{"ping":"pong"}
```

---

### `scan`

Realiza un escaneo del bus I2C y comprueba la presencia de los cuatro INA219 esperados.

Comando:

```text
scan
```

Ejemplo de respuesta:

```json
{"0x40":"true"}
{"0x41":"true"}
{"0x44":"true"}
{"0x45":"true"}
{"Result":"OK"}
```

El resultado `OK` se envía únicamente cuando los cuatro sensores están presentes.

---

### `u1`

Lee el voltaje de bus del INA219 con dirección `0x40`.

Comando:

```text
u1
```

Ejemplo de respuesta:

```json
{"voltage":"12.096"}
```

---

### `u2`

Lee el voltaje de bus del INA219 con dirección `0x41`.

Comando:

```text
u2
```

Ejemplo:

```json
{"voltage":"12.104"}
```

---

### `u3`

Lee el voltaje de bus del INA219 con dirección `0x44`.

Comando:

```text
u3
```

Ejemplo:

```json
{"voltage":"11.987"}
```

---

### `u4`

Lee el voltaje de bus del INA219 con dirección `0x45`.

Comando:

```text
u4
```

Ejemplo:

```json
{"voltage":"12.015"}
```

## Manejo de errores

Si no es posible leer un INA219, el firmware devuelve:

```text
error
```

Los comandos que no están definidos generan:

```text
invalid option: [comando]
```

## Flujo general

El funcionamiento del firmware es:

```text
Equipo externo
      |
      | SoftwareSerial @ 9600 baud
      v
   ATtiny85
      |
      | I2C
      +-------- INA219 0x40 (u1)
      |
      +-------- INA219 0x41 (u2)
      |
      +-------- INA219 0x44 (u3)
      |
      +-------- INA219 0x45 (u4)
```

## Librerías utilizadas

El firmware utiliza:

```cpp
#include <SoftwareSerial.h>
#include <Wire.h>
```

### SoftwareSerial

Se utiliza para la comunicación serial con el equipo externo.

### Wire

Se utiliza para la comunicación I2C con los sensores INA219.

## Resumen de comandos

| Comando | Función                       |
| ------- | ----------------------------- |
| `ping`  | Comprueba comunicación serial |
| `scan`  | Busca los INA219              |
| `u1`    | Lee INA219 `0x40`             |
| `u2`    | Lee INA219 `0x41`             |
| `u3`    | Lee INA219 `0x44`             |
| `u4`    | Lee INA219 `0x45`             |

## Estado actual

Esta versión del firmware está orientada a **pruebas de comunicación y lectura de voltaje de bus** de los cuatro INA219 mediante un ATtiny85.

La lectura de corriente y otros registros del INA219 pueden agregarse posteriormente.
