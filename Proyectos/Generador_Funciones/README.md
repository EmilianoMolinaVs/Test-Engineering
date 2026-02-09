# 🎛️ AD9833 DDS Function Generator (JSON Controlled)

Generador de funciones basado en el **AD9833** controlado mediante **JSON vía Serial** usando una UNIT PULSAR C6.  
Este proyecto implementa un **instrumento programable** capaz de generar señales senoidales, triangulares y cuadradas, con modos de operación **fijo** y **barrido (sweep)**.

---

## ✨ Características

- ✅ Control completo por **JSON vía Monitor Serie**
- ✅ Modos de operación:
  - **Fixed** (frecuencia fija)
  - **Sweep** (barrido de frecuencia no bloqueante)
- ✅ Formas de onda:
  - Senoidal
  - Triangular
  - Cuadrada Square 1 (Mantiene la **frecuencia original**)
  - Cuadrada Square 2 (Divide la **frecuencia a la mitad**)
- ✅ Control de:
  - Frecuencia
  - Fase
- ✅ Arquitectura modular y extensible
- ✅ Preparado para futuras expansiones (amplitud, offset, GUI, etc.)

---

## 🧠 Nota importante sobre amplitud y offset

El **AD9833 no permite controlar amplitud ni offset DC por software**.  
Estos parámetros se aceptan a nivel de diseño (JSON) para:
- Documentación
- Escalabilidad
- Integración futura con:
  - Amplificadores operacionales
  - DACs externos
  - Potenciómetros digitales

---

## 🧩 Hardware utilizado

- Módulo **AD9833 DDS**
- UNIT PULSAR C6
- Comunicación **SPI por conector QWIIC**
- Osciloscopio (para visualización)

### Pines SPI usados
```cpp
PIN_DATA  = 7;   // MOSI
PIN_CLK   = 6;   // SCK
PIN_FSYNC = 18;  // CS / FSYNC
```

---


## 🧪 Ejemplos de comandos JSON

A continuación se muestran algunos **ejemplos de comandos JSON** que pueden enviarse por el **Monitor Serie** (con salto de línea `\n`) para controlar el generador.

---

### 🔹 Modo FIXED (frecuencia fija)

#### Senoidal a 1 kHz con fase 0°
```json
{
  "mode": "fixed",
  "waveform": "sine",
  "frequency_hz": 1000,
  "phase_deg": 0
}
```


#### Senoidal a 500 Hz con desfase de 90°
```json
{
  "mode": "fixed",
  "waveform": "sine",
  "frequency_hz": 500,
  "phase_deg": 90
}
```

#### Onda triangular a 100 Hz
```json
{
  "mode": "fixed",
  "waveform": "triangle",
  "frequency_hz": 100,
  "phase_deg": 0
}
```

#### Onda cuadrada (Square2) a 10 kHz
```json
{
  "mode": "fixed",
  "waveform": "square2",
  "frequency_hz": 10000,
  "phase_deg": 0
}
```

### 🔹 Modo SWEEP (barrido de frecuencia)
#### Barrido senoidal de 10 Hz a 500 Hz
``` json
{
  "mode": "sweep",
  "waveform": "sine",
  "phase_deg": 0,
  "sweep": {
    "start_frequency_hz": 10,
    "stop_frequency_hz": 500,
    "step_hz": 1,
    "interval_ms": 20
  }
}
```

#### Barrido rápido (tipo chirp discreto)
```json
{
  "mode": "sweep",
  "waveform": "sine",
  "phase_deg": 0,
  "sweep": {
    "start_frequency_hz": 100,
    "stop_frequency_hz": 5000,
    "step_hz": 50,
    "interval_ms": 5
  }
}
```

#### Barrido lento de baja frecuencia
```json
{
  "mode": "sweep",
  "waveform": "triangle",
  "phase_deg": 0,
  "sweep": {
    "start_frequency_hz": 1,
    "stop_frequency_hz": 50,
    "step_hz": 0.5,
    "interval_ms": 200
  }
}
```