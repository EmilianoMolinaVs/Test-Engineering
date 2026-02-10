# TestBench UE0065 - Integración DRV2605L (Haptic Motor)

## Descripción

Proyecto para controlar un motor háptico usando el módulo I2C DRV2605L desde una Pulsar C6. No existe interfaz web ni página de pruebas: el sketch recibe comandos JSON por UART (UART2) y ejecuta patrones de vibración predefinidos.

Este repositorio contiene el sketch `IntegracionJSON.ino` que integra:
- Lectura de JSON por `UART2` (puerto serial hardware)
- Control del DRV2605L vía I2C
- Efectos hápticos `Mode1`, `Mode2`, `Mode3`

## Características principales

- Control por JSON vía UART (sin interfaz web)
- Tres modos de vibración preconfigurados
- Uso de `Adafruit_DRV2605` para manejar el driver háptico
- Respuesta y logs por `Serial` para depuración

## Dependencias

Instalar las siguientes bibliotecas en el Arduino IDE o PlatformIO:

- `Adafruit_DRV2605`
- `Adafruit_NeoPixel` (si se usara en variantes)
- `ArduinoJson` (para parsear los mensajes JSON)
- `Wire` (I2C, normalmente incluida en la plataforma ESP32)

En Arduino IDE: Sketch → Include Library → Manage Libraries → buscar e instalar.

## Conexiones / Wiring

- DRV2605L → Pulsar C6 via I2C
  - SDA → GPIO 6
  - SCL → GPIO 7
- UART2 (comunicación JSON)
  - RX2 (entrada del micro, leer desde maestro) → GPIO 15
  - TX2 (salida del micro) → GPIO 19
- Alimentación: 3.3V y GND a DRV2605L según la placa

> Ver `IntegracionJSON.ino` para confirmación de pines y configuración.

## Formato JSON de Entrada

El sketch espera un JSON por línea (terminado en `\n`) a través de `UART2`.

Ejemplo mínimo:

```json
{"Function":"Mode1"}
```

Campos soportados:
- `Function`: `Mode1`, `Mode2`, `Mode3` (sensibles a mayúsculas como en el sketch)

Al recibirlo, el firmware selecciona el modo correspondiente y ejecuta el patrón háptico.

## Modos disponibles

- Mode1: Vibración fuerte + pausa + pulso corto
- Mode2: Doble pulso rápido (notificación corta)
- Mode3: Patrón tipo alarma (fuerte → zumbido largo → fuerte)

Los efectos están mapeados con índices de waveform en `drv.setWaveform(...)` dentro del sketch.

## Uso / Instrucciones

1. Abrir el proyecto en Arduino IDE o PlatformIO y seleccionar placa ESP32 compatible (Pulsar C6/ESP32-C6 si disponible).
2. Instalar bibliotecas requeridas.
3. Compilar y cargar el sketch en la Pulsar C6.
4. Conectar un dispositivo que envíe JSON por UART2 (otra Pulsar maestro, adaptador UART-USB, etc.).
5. Enviar, por ejemplo:

```
{"Function":"Mode2"}\n
```

6. Verificar logs en el `Serial Monitor` (115200 baud) y observar el motor háptico ejecutar el patrón.

## Parámetros Serial

- `Serial` (depuración): 115200 bps
- `PagWeb` (UART2 usado para recibir JSON en el sketch): 115200 bps

## Notas de desarrollo

- El sketch usa `StaticJsonDocument<200>`; ajustar tamaño si se añaden campos.
- El driver DRV2605 está inicializado en `DRV2605_MODE_INTTRIG` y biblioteca seleccionada con `drv.selectLibrary(1)`.
- Se usa `Wire.begin(SDA_PIN, SCL_PIN)` en plataformas ESP32; en otras placas puede requerir `Wire.begin()` simple.

## Posibles mejoras

- Añadir parámetros en JSON para intensidad/duración (por ejemplo `{"Function":"Mode1","Level":80}`)
- Añadir confirmación por UART de ejecución (`OK`/`ERR`) para integrar con el maestro
- Mapear efectos a nombres configurables desde SD o EEPROM

## Archivos relevantes

- `IntegracionJSON.ino` — Sketch principal (recepción JSON y control DRV2605L)

## Licencia

Proyecto interno - TestBench Engineering

---

**Última actualización**: Febrero 2026
