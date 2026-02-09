# 🎛️ AD9833 DDS Function Generator (JSON Controlled)

Generador de funciones basado en el **AD9833** controlado mediante **JSON vía Serial** usando Arduino.  
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
  - Cuadrada (Square1 y Square2)
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
- Microcontrolador compatible con Arduino
- Comunicación **SPI por pines arbitrarios**
- Osciloscopio (para visualización)

### Pines SPI usados
```cpp
PIN_DATA  = 7;   // MOSI
PIN_CLK   = 6;   // SCK
PIN_FSYNC = 18;  // CS / FSYNC
'''



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


#### Senoidal a 500 Hz con desfase de 90°
'''json 
{
  "mode": "fixed",
  "waveform": "sine",
  "frequency_hz": 500,
  "phase_deg": 90
}