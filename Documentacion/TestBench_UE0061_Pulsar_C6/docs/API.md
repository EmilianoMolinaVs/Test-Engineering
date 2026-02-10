# API y Protocolos - TestBench UE0061

## Protocolo de Comunicación WiFi

### Conexión Inicial

La Pulsar C6 se conecta a la red WiFi "TestBench2" con contraseña "12345678".

```
SSID: TestBench2
Contraseña: 12345678
Tipo de Seguridad: WPA2
```

### Envío de Datos

Los resultados de pruebas se envían en formato JSON a través de la red local al servidor maestro.

## Estructura JSON de Respuesta

### Campos Principales

| Campo | Tipo | Descripción |
|-------|------|-------------|
| `mac` | String | Dirección MAC de la Pulsar C6 |
| `wifi_ok` | Boolean | Estado de conexión WiFi |
| `ip` | String | IP asignada en la red |
| `oled_ok` | Boolean | Pantalla OLED detectada |
| `sd_detected` | Boolean | Tarjeta SD detectada |
| `sd_rw` | Boolean | Lectura/escritura en SD OK |
| `filename` | String | Nombre del archivo de prueba |
| `content` | String | Contenido del archivo escrito |

### Ejemplo de Respuesta

```json
{
  "mac": "84:F7:03:AB:CD:EF",
  "wifi_ok": true,
  "ip": "192.168.4.45",
  "oled_ok": true,
  "sd_detected": true,
  "sd_rw": true,
  "filename": "test.txt",
  "content": "SD OK"
}
```

## Protocolo UART

La Pulsar C6 expone un puerto UART para comandos remotos.

### Parámetros UART

- **Baudrate**: 9600 bps
- **Bits de datos**: 8
- **Paridad**: Ninguna
- **Bits de parada**: 1
- **TX**: GPIO 17
- **RX**: GPIO 16

### Comandos Disponibles

#### LED_ON

```
Entrada: LED_ON\n
Respuesta: OK\n
Acción: Enciende LED RGB (color verde) y todos los GPIOs
```

#### LED_OFF

```
Entrada: LED_OFF\n
Respuesta: OK\n
Acción: Apaga LED RGB y todos los GPIOs
```

#### Comando No Reconocido

```
Entrada: UNKNOWN_COMMAND\n
Respuesta: ERR\n
```

## GPIO Control

### Pines Digitales (D)

Los siguiente pines pueden ser controlados:
- D13, D12, D11, D10, D7, D6, D5, D4

### Pines Analógicos (A)

- A0, A1, A2, A3, A4, A5

### Mapeo de Pines

```python
PINMAP = {
    "A0": 0,   "D14": 0,
    "A1": 1,   "D15": 1,
    "A2": 3,   "D16": 3,
    "A3": 4,   "D17": 4,
    "A4": 22,  "D18": 22,
    "A5": 23,  "D19": 23,
    "A6": 5,   "D21": 5,
    "A7": 6,   "D13": 6,
    "D0": 17,  "D1": 16,
    "D2": 8,
    "D3": 9,
    "D4": 15,
    "D5": 19,
    "D6": 20,
    "D7": 21,
    "D8": 12,
    "D9": 13,
    "D10": 18,
    "D11": 7,
    "D12": 2
}
```

## I2C / QWIIC (bus 0)

### Configuración

- **SDA**: GPIO 6
- **SCL**: GPIO 7
- **Velocidad**: 400 kHz (standard I2C)

### Dispositivos Detectados

- **OLED SSD1306**: Pantalla 128x64 I2C

## SPI (bus 1)

### Configuración

- **MISO**: GPIO 2
- **MOSI**: GPIO 7
- **SCK**: GPIO 6
- **CS**: GPIO 19
- **Velocidad**: 500 kHz

### Dispositivos

- **SD Card**: Almacenamiento externo

## LED RGB (NeoPixel)

### Pin

- **GPIO**: 8
- **Tipo**: WS2812B (addressable RGB LED)

### Colores de Estado

| Color | RGB | Significado |
|-------|-----|-------------|
| Naranja | (255, 165, 0) | Inicializando |
| Verde | (0, 255, 0) | Pruebas OK |
| Azul | (0, 0, 255) | OLED fallaron, SD OK |
| Morado | (128, 0, 128) | SD R/W falló |
| Rojo | (255, 0, 0) | WiFi no disponible |

### Ciclo de Color

El LED cambia de color cada 5 grados de hue al mantener presionado el botón.

## Botón (INPUT)

### Pin

- **GPIO**: 9
- **Tipo**: INPUT con PULL_UP
- **Intervalo de doble clic**: 500 ms

### Comportamiento

- **Click único**: Cicla colores del LED
- **Click doble**: Apaga el LED
- **Presión prolongada**: Cambia colores continuamente

## Almacenamiento SD

### Operaciones de Prueba

1. **Inicialización**: Monta SD usando protocolo SPI
2. **Escritura**: Crea archivo `/sd/test.txt` con contenido "SD OK\n"
3. **Lectura**: Lee el archivo grabado
4. **Desmontaje**: Desmonta el sistema de archivos

### Estructura de Archivos

```
/sd/
└── test.txt    (archivo de prueba)
```

## Pantalla OLED

### Resolución

- 128 x 64 píxeles
- Monocromo (blanco/negro)

### Información Mostrada

- Estado de SD e OLED
- Resultado de lectura/escritura
- Nombre del archivo de prueba
- IP asignada

## Ciclo de Ejecución

```
1. Inicializar SD Card
   └─ Intentar 1 vez

2. Probar SD Read/Write
   └─ Escribir test.txt
   └─ Leer test.txt
   └─ Desmontar

3. Inicializar OLED
   └─ Intentar hasta 3 veces

4. Conectar WiFi
   └─ Escanear redes
   └─ Buscar "TestBench2"
   └─ Conectar con contraseña
   └─ Reintentar hasta 10 veces

5. Mostrar Resultados
   └─ OLED (si está disponible)
   └─ Puerto Serial (115200 baud)
   └─ RED WiFi (a servidor maestro)

6. Loop Principal
   └─ Procesar comandos UART
   └─ Controlar LED por botón
   └─ Mantener conexión WiFi
```

## Códigos de Error

### Inicialización SD

- **OK**: SD detectada y accesible
- **FAIL**: SD no detectada o error en SPI

### Prueba SD R/W

- **OK**: Lectura y escritura exitosas
- **FAIL**: Error en escritura o lectura

### OLED

- **OK**: Pantalla detectada en I2C
- **RETRY x3**: Reintentos fallidos
- **FAIL**: OLED no disponible

### WiFi

- **OK**: Conectado a TestBench2
- **NOT FOUND**: Red TestBench2 no disponible
- **AUTH ERROR**: Contraseña incorrecta
- **TIMEOUT**: No se conectó después de 10 intentos

## Velocidades de Baudrate

| Interfaz | Baudrate |
|----------|----------|
| UART (Comandos) | 9600 |
| Serial Monitor | 115200 |

## Timing y Sleeps

| Evento | Delay |
|--------|-------|
| Intento WiFi | 1000 ms |
| Debounce botón | 300 ms |
| Loop principal | 50 ms |
| Cambio de color LED | 100 ms |

---

Para más información, consultar el README.md y FIRMWARE_SETUP.md
