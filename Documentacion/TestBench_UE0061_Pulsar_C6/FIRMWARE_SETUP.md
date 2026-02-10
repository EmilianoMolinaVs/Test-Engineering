# Guía de Instalación de Firmware - TestBench UE0061

## Requisitos Previos

- **Python 3.7+** instalado
- **esptool.py** para programación del ESP32
- **pyserial** para comunicación serie
- Conexión USB a la Pulsar C6

## Paso 1: Instalar Herramientas

### Windows (PowerShell)

```powershell
pip install esptool pyserial
```

### Linux / macOS (Terminal)

```bash
pip3 install esptool pyserial
```

## Paso 2: Preparar la Pulsar C6

1. Conectar la Pulsar C6 al puerto USB
2. Identificar el puerto COM:
   - **Windows**: `COM3`, `COM4`, etc. (ver Administrador de dispositivos)
   - **Linux/Mac**: `/dev/ttyUSB0`, `/dev/ttyACM0`, etc.

## Paso 3: Borrar Flash (Importante)

```bash
esptool.py --port COM3 erase_flash
```

Reemplazar `COM3` con el puerto identificado.

**Esto eliminará todo el contenido previo en la memoria flash.**

## Paso 4: Grabar Firmware MicroPython

```bash
esptool.py --port COM3 --baud 460800 write_flash -z 0x1000 ESP32_GENERIC_C6-20241129-v1.24.1.bin
```

Esperar a que termine completamente.

## Paso 5: Cargar código MicroPython

Existen varias opciones:

### Opción A: Usar WebREPL (Recomendado)

1. Conectar por WebREPL a través de la IP de la Pulsar
2. Cargar los archivos:
   - `config.py`
   - `main.py`
   - `sdcard.py`
   - `ssd1306.py`

### Opción B: Usar Ampy

```bash
pip install adafruit-ampy
```

Cargar archivos:

```bash
ampy --port COM3 put firmware/config.py config.py
ampy --port COM3 put firmware/main.py main.py
ampy --port COM3 put firmware/sdcard.py sdcard.py
ampy --port COM3 put firmware/ssd1306.py ssd1306.py
```

### Opción C: Usar Thonny IDE

1. Descargar e instalar Thonny
2. Configurar puerto COM en Thonny
3. Copiar archivos a la Pulsar usando la interfaz gráfica

## Paso 6: Verificar Instalación

1. AbrirtoMinitérm o Putty con los siguientes parámetros:
   - Puerto: `COM3` (o el identificado)
   - Velocidad: `115200`
   - Bits de datos: 8
   - Paridad: Ninguna
   - Bits de parada: 1

2. Presionar botón RESET en la Pulsar C6 o desconectar/reconectar USB

3. Debería mostrar:
   ```
   Inicializando SD...
   Escaneando redes disponibles...
   Conectando a TestBench2...
   ```

## Solución de Problemas

### Error: "No module named 'config'"

**Causa**: `config.py` no fue cargado correctamente
**Solución**: Verificar que `config.py` está en el sistema de archivos de la Pulsar

### Error: "OLED no detectado"

**Causa**: Conexión I2C deficiente o OLED no conectado
**Solución**: 
- Revisar conexión física de pines SDA/SCL
- Verificar voltaje (3.3V) en QWIIC

### Error: "SD Card no inicializa"

**Causa**: Tarjeta SD no formateada o conexión SPI deficiente
**Solución**:
- Formatear SD Card como FAT32
- Revisar conexión física de pines SPI
- Reintentar con SD Card diferente

### No hay conexión WiFi

**Causa**: Red "TestBench2" no disponible
**Solución**:
- Verificar que Pulsar maestro está encendida
- Confirmar SSID "TestBench2" y contraseña "12345678"
- Acercarse a la antena WiFi

## Comandos Útiles

### Ver contenido de archivos

```bash
ampy --port COM3 ls
```

### Eliminar archivo

```bash
ampy --port COM3 rm main.py
```

### Ver espacio disponible

```bash
ampy --port COM3 du
```

## Versiones de Firmware

- **MicroPython v1.24.1**: Versión estable recomendada
- **Framework**: ESP32-C6 (RISC-V)

## Notas Adicionales

- La Pulsar C6 ejecuta `main.py` automáticamente al iniciar
- Para editar código, usar WebREPL o Thonny
- El LED RGB parpadea en naranja durante inicio
- Esperar ~10 segundos para que complete todas las pruebas

---

Para más información consultar el README.md principal.
