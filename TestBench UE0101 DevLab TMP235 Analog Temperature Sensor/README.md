# TestBench UE0101 - TMP235 Analog Temperature Sensor

## Descripción general

TestBench para el sensor analógico de temperatura TMP235. Este proyecto incluye:
- Código principal en Arduino para lectura de sensor analógico (`maincode_UE0101_TMP235.ino`) — integración con shield Pulsar C6.
- Ejemplo simple de lectura (`example_temp_sensor.ino`) — inicio rápido para nuevos usuarios.

El código incluye integración con OLED, NeoPixel, pulsadores, relés y envío de datos por JSON a la página web de pruebas.

## Estado

- Lectura analógica: probado y utilizado en línea de producción.
- Integración con Shield Pulsar C6: funcional.
- Interfaz web: disponible para monitoreo y control remoto.
- Producto final: en fabricación, con interfaz de pruebas visual.

## Archivos clave

- `maincode_UE0101_TMP235/maincode_UE0101_TMP235.ino` — Código principal para lectura de 6 sensores TMP235 simultáneamente. Incluye comunicación UART a 115200 baud, manejo de OLED (SSD1306), NeoPixel de estado, y control de relés. Reporta datos por JSON a la página web.
- `example_temp_sensor/example_temp_sensor.ino` — Ejemplo simple y didáctico para comenzar con la lectura del sensor.

## Notas de Hardware / Wiring

### Pins de sensor analógico (ADC)
- **SENS_1** → GPIO 0 (entrada analógica)
- **SENS_2** → GPIO 1 (entrada analógica)
- **SENS_3** → GPIO 2 (entrada analógica)
- **SENS_4** → GPIO 4 (entrada analógica)
- **SENS_5** → GPIO 5 (entrada analógica)
- **SENS_6** → GPIO 6 (entrada analógica)

### Pins de comunicación I2C (OLED)
- **SDA_PIN** → GPIO 22 (dato)
- **SCL_PIN** → GPIO 23 (reloj)

### Pins de control / auxiliares
- **SWITCH** → GPIO 15 (pulsador de entrada)
- **RELAY** → GPIO 14 (relé de salida)
- **NEOPIX** → GPIO 8 (franja NeoPixel WS2812B de 3 LED)

### Especificaciones del TMP235

El sensor TMP235 es un conversor analógico que:
- Rango operativo: -40°C a +125°C
- Salida analógica lineal: 0 mV @ -20°C, 10 mV/°C
- Voltaje de alimentación: 1.5V a 5.5V
- Resolución del ADC (ESP32): 12 bits @ 5V → ~1.22 mV/LSB

### Calibración de lectura

La función `readTempC()` realizará:
1. Lectura del ADC (12 bits) en el pin analógico
2. Conversión a voltaje: `Vsensor = (lectura_ADC / 4095) * 3.3V`
3. Cálculo de temperatura: `T(°C) = (Vsensor - offset) / 0.01` (dependiendo del offset del módulo)

**Nota**: verificar la hoja de datos del TMP235 para el offset exacto en tu módulo específico.

## Configuración de compilación

**IDE**: Arduino IDE 1.8.x o PlatformIO
**Board**: ESP32 DevKit C (DOIT)
**Compilación**:
- Velocidad serial: 115200 baud
- Frecuencia CPU: 80 MHz (o 160 MHz si se requiere)
- ADC: Resolución 12 bits, Atenuación 11dB para rango 0-3.3V

## Protocolo y ejemplos JSON

El código `maincode_UE0101_TMP235.ino` envía periódicamente por UART un JSON con las lecturas:

### Formato de salida automática:

```json
{
  "resp": "OK",
  "sensor": "TMP235",
  "s1": 23.5,
  "s2": 24.1,
  "s3": 22.8,
  "s4": 25.0,
  "s5": 23.3,
  "s6": 24.7,
  "timestamp": 1234567890
}
```

### Variables JSON:
- **resp**: Estado de respuesta ("OK" o "ERROR")
- **sensor**: Identificador del dispositivo ("TMP235")
- **s1 a s6**: Lecturas de temperatura en °C de cada sensor
- **timestamp**: Marca de tiempo Unix (opcional)

## Integración con Pulsar C6 / PagWeb

La página web de pruebas puede:
1. **Enviar comandos** vía UART (JSON con formato reconocido)
2. **Recibir datos** de temperatura en tiempo real
3. **Controlar relés** mediante comandos JSON
4. **Monitorear estado** del NeoPixel (indicador visual)

### Respuesta esperada:
```json
{
  "resp": "OK",
  "s1": 23.5,
  "s2": 24.1,
  "s3": 22.8,
  "s4": 25.0,
  "s5": 23.3,
  "s6": 24.7
}
```

## Características del código principal

- **Lectura múltiple**: 6 sensores analógicos simultáneamente
- **Comunicación UART**: puente entre módulo e interfaz web (115200 baud)
- **pantalla OLED**: muestra en tiempo real valores, animaciones y estado
- **Control de relés**: permite encender/apagar cargas externas
- **NeoPixel RGB**: indica estado del sistema (verde = OK, rojo = error)
- **JSON serializado**: formato estándar para comunicación con página web

## Verificación / Testing

1. **Compilar y cargar** el código en ESP32
2. **Abrir Monitor Serial** (115200 baud)
3. **Verificar lecturas** en pantalla OLED y salida JSON
4. **Conectar a PagWeb** y validar comunicación bidireccional
5. **Probar relés** con comando `{"Function": "relay_on"}` / `relay_off`

## Notas y solución de problemas

- **OLED no se inicializa**: verificar conexión I2C y pull-ups (4.7kΩ recomendados)
- **Lecturas inestables**: verificar capacitores de filtrado en línea de alimentación
- **JSON no se deserializa**: revisar terminador de línea (`\n`) desde PagWeb
- **Relé no funciona**: asegurar que el pin RELAY está en nivel alto (normalmente abierto)

## Línea de producción

Este código está **probado y activo en línea de producción** para:
- Integración en sistemas Pulsar C6
- Monitoreo de temperatura en procesos de manufactura
- Control automático de cargas mediante relés
