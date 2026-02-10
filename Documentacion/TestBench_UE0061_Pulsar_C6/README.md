# TestBench UE0061 - Pulsar Nano C6

## Descripción

Banco de pruebas para el **ESP32-C6** en formato nano ("Pulsar C6"). Este sistema realiza una rutina de pruebas automatizadas que evalúa las principales funcionalidades de la placa, valida la integridad de sus componentes y reporta los resultados en formato JSON a una red WiFi centralizada gestionada por una Pulsar maestro.

El firmware ejecuta pruebas de GPIO, I2C (QWIIC), SD Card, UART y conectividad WiFi, transmitiendo los datos a una base de datos local para monitoreo en tiempo real.

## Características Principales

- **Automatización de Pruebas**: Rutina completa de validación de hardware
- **Comunicación WiFi**: Conexión a red tipo AP/STA para reportar datos
- **Protocolo JSON**: Envío de resultados en formato JSON estructurado
- **Integración en Red**: Reporte centralizado en base de datos local
- **Indicadores Visuales**: LED RGB (NeoPixel) para estados de prueba
- **Pantalla OLED**: Visualización local de resultados
- **Almacenamiento SD**: Validación de lectura/escritura

## Hardware

- **MCU**: ESP32-C6 (formato nano/Pulsar)
- **Pantalla**: OLED 128x64 (I2C/QWIIC)
- **Almacenamiento**: SD Card (SPI)
- **LED RGB**: NeoPixel
- **Comunicación**: UART, WiFi, I2C

## Estructura del Proyecto

```
TestBench_UE0061_Pulsar_C6/
├── README.md                       # Este archivo
├── FIRMWARE_SETUP.md               # Guía de instalación de firmware
├── firmware/
│   ├── main.py                     # Firmware principal con rutina de pruebas
│   ├── config.py                   # Configuración de pines
│   ├── sdcard.py                   # Driver SD Card
│   └── ssd1306.py                  # Driver OLED
├── examples/
│   └── test_results.json           # Ejemplo de resultado de pruebas
└── docs/
    └── API.md                      # Documentación de API y protocolos
```

## Instalación y Programación

### Requisitos

- **Python 3.7+**
- **esptool.py**
- **pyserial**
- Conexión USB a la Pulsar C6

### Instalación de Dependencias

```bash
pip install esptool pyserial
```

### Windows

```powershell
# Borrar flash
esptool.py erase_flash

# Grabar firmware MicroPython
esptool.py --baud 460800 write_flash 0x1000 ESP32_GENERIC_C6-20241129-v1.24.1.bin

# Cargar código MicroPython
# Usar Ampy u otra herramienta para cargar los archivos de firmware/
```

### Linux / macOS

```bash
# Borrar flash
esptool.py erase_flash

# Grabar firmware MicroPython
esptool.py --baud 460800 write_flash 0x1000 ESP32_GENERIC_C6-20241129-v1.24.1.bin

# Cargar código MicroPython
# Usar Ampy u otra herramienta para cargar los archivos de firmware/
```

## Pruebas Ejecutadas

La rutina de pruebas valida automáticamente:

| Componente | Función | Status Reportado |
|-----------|----------|-----------------|
| **GPIO** | Activación de pines D y A | `gpio_state` |
| **I2C (QWIIC)** | Detecta OLED en bus I2C | `oled_ok` |
| **SD Card** | Inicializa y detecta tarjeta | `sd_detected` |
| **SD R/W** | Escribe y lee archivo de prueba | `sd_rw` |
| **WiFi** | Conecta a red "TestBench2" | `wifi_ok` |
| **Dirección MAC** | Identifica dispositivo | `mac` |
| **IP Asignada** | Obtiene IP de la red | `ip` |

## Protocolo de Comunicación

### Resultado de Pruebas (JSON)

```json
{
  "mac": "AA:BB:CC:DD:EE:FF",
  "wifi_ok": true,
  "ip": "192.168.4.XX",
  "oled_ok": true,
  "sd_detected": true,
  "sd_rw": true,
  "filename": "test.txt",
  "content": "SD OK"
}
```

### Conexión a Servidor Maestro

La Pulsar C6 envía los datos a la red WiFi creada por la **Pulsar maestro** (TestBench2). Los datos se reportan en base de datos local para:
- Monitoreo en tiempo real
- Histórico de pruebas
- Detección de fallos
- Análisis de trazabilidad

### Comandos UART

Se pueden enviar comandos seriales para control remoto:

```
LED_ON   → Enciende LED RGB y GPIOs
LED_OFF  → Apaga LED RGB y GPIOs
```

## Indicadores Visuales

El LED RGB (NeoPixel) indica estados mediante colores:

- 🟠 **Naranja**: Iniciando/conectando WiFi
- 🟢 **Verde**: Todas las pruebas OK
- 🔵 **Azul**: Falló OLED, SD OK
- 🟣 **Morado**: Falló SD R/W
- 🔴 **Rojo**: WiFi no disponible o error crítico

## Pantalla OLED

Muestra localmente:
- Estado de pruebas (SD, OLED, WiFi)
- Archivo de prueba creado
- Contenido escrito/leído
- IP asignada

## Configuración de Pines

Definidos en `firmware/config.py`:

### I2C/QWIIC
- **SDA**: Pin 6
- **SCL**: Pin 7

### SD Card (SPI)
- **MISO**: Pin 2
- **MOSI**: Pin 7
- **SCK**: Pin 6
- **CS**: Pin 19

### UART
- **TX**: Pin 17 (Modem)
- **RX**: Pin 16 (Modem)

### GPIO Digitales (D13-D21)
- D pins controlables para pruebas

### NeoPixel
- **PIN**: 8

## Versiones

| Versión | Fecha | Cambios |
|---------|-------|---------|
| 1.0 | 17/04/2025 | Versión inicial estable |
| 3 | 18/03/2025 | Versión anterior con MicroPython 1.24.1 |

## Troubleshooting

| Problema | Solución |
|----------|----------|
| Error de conexión WiFi | Verificar SSID "TestBench2" y contraseña "12345678" |
| OLED no detectado | Revisar conexión I2C y voltaje en pines SDA/SCL |
| SD Card no reconocida | Verificar conexión SPI e insertar tarjeta formateada |
| Error en grabación de firmware | Usar `esptool.py erase_flash` primero, después grabar |
| No hay respuesta UART | Confirmar puerto serial y velocidad (9600 baud) |

## Notas de Operación

1. **Conexión WiFi**: Requiere que la Pulsar maestro esté activa con red "TestBench2"
2. **Base de Datos**: Los datos se reportan automáticamente a BD local en servidor maestro
3. **Autorización**: Usa contraseña "12345678" para red WiFi
4. **Tiempo de prueba**: ~10 segundos para completar todas las validaciones
5. **Reintento**: Reintenta conexión WiFi hasta 10 veces

## Licencia

Proyecto interno - TestBench Engineering

---

**Última actualización**: Febrero 2026  
**Autor**: CesarBautista  
**Estado**: Producción
