# UE0116 DevLab - MPR121QR2 Touch Sensor I2C

Firmware de prueba para el módulo UE0116: sensor capacitivo de toque MPR121QR2.

## Descripción

Este firmware configura el MPR121QR2 en modo híbrido:
- Electrodos E0..E3 se usan como entradas táctiles.
- Electrodos E4..E11 se usan como salidas GPIO digitales para verificar el estado de la placa.

La comunicación con la interfaz de pruebas `PagWeb` se realiza por UART2 usando mensajes JSON.

## Archivos

- `MainCode_UE0116_MPR121QR2.ino`: Firmware principal del test bench.
- `README.md`: Documentación de uso y conexiones.

## Conexiones principales

- `SDA` → GPIO 6
- `SCL` → GPIO 7
- `UART2 RX` → GPIO 15
- `UART2 TX` → GPIO 19

### Pines de verificación GPIO del MPR121

| Electrode | Pin microcontrolador | Uso |
|----------:|----------------------|-----|
| E4        | GPIO 20              | Lectura estado digital |
| E5        | GPIO 21              | Lectura estado digital |
| E6        | GPIO 18              | Lectura estado digital |
| E7        | GPIO 2               | Lectura estado digital |
| E8        | GPIO 14              | Lectura estado digital |
| E9        | GPIO 0               | Lectura estado digital |
| EA        | GPIO 1               | Lectura estado digital |
| EB        | GPIO 3               | Lectura estado digital |

## Comandos JSON soportados

Enviar los siguientes mensajes JSON por UART2 hacia `PagWeb`:

- `{"Function":"ping"}`
  - Respuesta: `{"ping":"pong"}`
- `{"Function":"init"}`
  - Reinicia la configuración del MPR121 y reitera la inicialización.
- `{"Function":"digitalScan"}`
  - Escanea salidas E4..E11 y devuelve un reporte con `port0..port7`.
- `{"Function":"restart"}`
  - Reinicia el microcontrolador.

## Formato de respuesta

- Para eventos táctiles:
  - `{"touch":"E0"}`
  - `{"release":"E0"}`
- Para `digitalScan`:
  - `{"port0":"OK","port1":"OK",...,"Result":"OK"}`

## Notas

- El firmware asume que el MPR121 está conectado correctamente al bus I2C.
- Los puertos E4..E11 se configuran como GPIO de salida en el MPR121, y el firmware verifica su estado con entradas físicas en el microcontrolador.
- Si no hay datos entrantes por UART2, el firmware supervisa continuamente los eventos táctiles en E0..E3.
