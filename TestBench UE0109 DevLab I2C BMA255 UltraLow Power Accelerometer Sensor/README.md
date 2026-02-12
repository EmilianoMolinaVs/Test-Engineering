# TestBench UE0109 - BMA255 Accelerometer (I2C / SPI)

## Descripción general

TestBench para el acelerómetro de bajo consumo BMA255. Este proyecto incluye:
- Código principal en Arduino para lectura por I2C y SPI (`mainCode_JSON_i2c_spi_BMA255.ino`) — selección dinámica de protocolo.
- Ejemplo I2C (`example_i2c_bma255.ino`) — inicio rápido para lectura por I2C.
- Ejemplo SPI (`example_spi_bma255.ino`) — inicio rápido para lectura por SPI.

El código integra lectura de aceleración (x, y, z) y temperatura, con reportes JSON a la página web de pruebas/interfaz de control remoto.

## Estado

- Lectura I2C: probado y funcional
- Lectura SPI: probado y funcional
- Autoselección de protocolo: activa, controlada por pines de selección
- Interfaz web: disponible para monitoreo y control
- Integración Pulsar C6: soportada

## Archivos clave

- `mainCode_JSON_i2c_spi_BMA255/mainCode_JSON_i2c_spi_BMA255.ino` — Código principal con soporte para lectura en I2C y SPI. Detecta automáticamente el protocolo según configuración de pines. Reporta aceleración x, y, z y temperatura por JSON vía UART a 115200 baud.
- `example_i2c_bma255/example_i2c_bma255.ino` — Ejemplo simple de lectura por I2C (dirección automática 0x18 ó 0x19).
- `example_spi_bma255/example_spi_bma255.ino` — Ejemplo simple de lectura por SPI.

## Notas de Hardware / Wiring

### Pins de comunicación I2C
- **SDA_PIN** → GPIO 6 (MOSI en modo SPI)
- **SCL_PIN** → GPIO 7 (SCL en modo I2C)
- **SDO_PIN** → GPIO 2 (MISO en SPI, selector de dirección I2C en I2C)

### Pins de comunicación SPI
- **SCK** → GPIO 7 (reloj SPI)
- **MOSI** → GPIO 6 (datos hacia sensor)
- **MISO** → GPIO 2 (datos desde sensor)
- **CS_PIN** → GPIO 18 (selección de esclavo)

### Pines de control / selector de protocolo
- **PS_PIN** → GPIO 21 (selector de protocolo: LOW = I2C, HIGH = SPI)
- **SDO_PIN** → GPIO 2 (selector de dirección I2C: LOW = 0x18, HIGH = 0x19)
- **RUN_BUTTON** → GPIO 4 (pulsador de arranque/control)

### Especificaciones del BMA255

El sensor BMA255 es un acelerómetro de 3 ejes que:
- Rango programable: ±2g, ±4g, ±8g, ±16g
- Comunicación: I2C o SPI (no simultánea en el mismo módulo)
- Direcciones I2C: 0x18 (SDO=0) ó 0x19 (SDO=1)
- Resolución: calibración automática hasta 12 bits
- Bajo consumo: ~130 µA en modo normal
- Rango de temperatura operativa: -40°C a +85°C

### Modo de operación

El sensor selecciona automáticamente el protocolo:
1. **SDA_PIN/SCL_PIN + PS_PIN=LOW** → Modo I2C
2. **SPI pins + PS_PIN=HIGH** → Modo SPI
3. El pin **CS_PIN** se utiliza sólo en modo SPI

**Importante**: No conectar I2C y SPI simultáneamente sin aislamiento. Usar jumpers o resistencias serie (1 kΩ) para cambiar entre modos.

## Configuración de compilación

**IDE**: Arduino IDE 1.8.x o PlatformIO
**Board**: ESP32 DevKit C (DOIT)
**Compilación**:
- Velocidad serial: 115200 baud
- Frecuencia CPU: 80 MHz (o 160 MHz si se requiere)
- Librerías requeridas:
  - `ArduinoJson` (versión 6.x)
  - `BMA250` (incluida en el proyecto)

## Protocolo y ejemplos JSON

El código `mainCode_JSON_i2c_spi_BMA255.ino` implementa un protocolo JSON bidireccional:

### Comandos esperados desde PagWeb:

#### 1. Ping (validación de comunicación)
Entrada:
```json
{"Function":"ping"}
```

Respuesta:
```json
{"ping":"pong"}
```

#### 2. Lectura I2C dirección 0x18
Entrada:
```json
{"Function":"i2c18"}
```

Respuesta (éxito):
```json
{
  "addr": "0x18",
  "x": 125,
  "y": -45,
  "z": 980,
  "temp": 28,
  "unit_accel": "mg",
  "unit_temp": "C"
}
```

Respuesta (fallo):
```json
{"i2c0x18":"Fail"}
```

#### 3. Lectura I2C dirección 0x19
Entrada:
```json
{"Function":"i2c19"}
```

Respuesta (similar a i2c18 pero con addr "0x19")

#### 4. Lectura SPI
Entrada:
```json
{"Function":"spi"}
```

Respuesta:
```json
{
  "iface": "SPI",
  "x": 125,
  "y": -45,
  "z": 980,
  "temp": 28,
  "unit_accel": "mg",
  "unit_temp": "C"
}
```

### Variables JSON en respuesta:
- **addr** o **iface**: Dirección I2C hexadecimal (0x18/0x19) o interfaz ("SPI")
- **x, y, z**: Aceleración en miligramos (mg) en los tres ejes
- **temp**: Temperatura interna del sensor en °C
- **unit_accel**: Unidad de aceleración ("mg")
- **unit_temp**: Unidad de temperatura ("C")

## Funcionalidad del código principal

### Características
- **Selección dinámica de protocolo**: GPIO 21 elige entre I2C (LOW) y SPI (HIGH)
- **Autodetección de dirección I2C**: intenta conectar y detecta 0x18 ó 0x19
- **Lectura de 3 ejes**: captura aceleración en x, y, z simultáneamente
- **Lectura de temperatura**: sensor integrado del módulo BMA255
- **JSON serializado**: envío de datos en formato estándar
- **Comunicación UART**: protocolo bidireccional con PagWeb (115200 baud)
- **Bucle de espera**: aguarda comandos JSON antes de ejecutar lectura

### Flujo de operación

1. **Inicialización**: configura pines, inicia UART y aguarda comandos
2. **Recepción**: espera JSON desde UART con campo "Function"
3. **Procesamiento**: decodifica el comando y ejecuta la acción:
   - "ping" → valida comunicación
   - "i2c18" → lectura por I2C @ 0x18
   - "i2c19" → lectura por I2C @ 0x19
   - "spi" → lectura por SPI
4. **Envío**: serializa respuesta en JSON y transmite por UART

## Integración con Pulsar C6 / PagWeb

La página web de pruebas:
1. Envía comandos JSON con el protocolo descrito
2. Recibe datos en tiempo real del acelerómetro
3. Puede cambiar rango y configuración dinámicamente
4. Almacena datos históricos para análisis

### Velocidades recomendadas

- **Lectura periódica**: cada 100-500 ms
- **Tasa de muestreo del sensor**: 64 ms (configurable en código)
- **Baudrate UART**: 115200 bps

## Verificación / Testing

1. **Compilar y cargar** el código en ESP32
2. **Abrir Monitor Serial** (115200 baud)
3. **Enviar comando**: `{"Function":"ping"}`
4. **Verificar respuesta**: debe recibir `{"ping":"pong"}`
5. **Probar lectura I2C**: `{"Function":"i2c18"}`
6. **Probar lectura SPI**: primero cambiar PS_PIN a HIGH, luego `{"Function":"spi"}`
7. **Conectar a PagWeb** y validar comunicación bidireccional

## Notas y solución de problemas

### Problemas comunes

- **"Addr I2C: Fail"**: El sensor no responde en I2C
  - Verificar conexión SDA/SCL
  - Revisar pull-ups (4.7kΩ recomendados)
  - Validar que PS_PIN esté en LOW para modo I2C

- **SPI no funciona**: 
  - Asegurar PS_PIN en HIGH
  - Verificar CS_PIN en GPIO 18
  - Comprobar conexiones MOSI/MISO/SCK

- **Valores anómalos de aceleración**:
  - El sensor está en movimiento; colocar en superficie plana
  - Puede requerir calibración (ver biblioteca BMA250)
  - Revisar rango configurado (por defecto ±2g)

- **JSON no deserializa**:
  - Verificar terminador `\n` al final del comando
  - Asegurar baudrate 115200
  - Revisar formato JSON en PagWeb

## Registros de cambios

### v1.0 (Actual)
- Soporte I2C y SPI
- Protocolo JSON bidireccional
- Lectura de 3 ejes + temperatura
- Integración con Pulsar C6

## Referencias

- **BMA255 Datasheet**: Sensor de aceleración de 3 ejes, bajo consumo
- **Arduino JSON Library**: Documentación en https://arduinojson.org/
- **I2C Protocol**: Velocidad estándar 100 kHz, rápida 400 kHz
- **SPI Protocol**: Velocidad típica 1-5 MHz para BMA255

## Línea de producción

Este código está **probado y funcional** para:
- Integración en sistemas Pulsar C6
- Monitoreo de vibraciones y movimiento
- Testing de aceleración en procesos de manufactura
- Control remoto vía página web de pruebas
