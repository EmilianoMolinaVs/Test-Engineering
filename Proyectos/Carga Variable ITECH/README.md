# 🔌 FIRMWARE PUENTE ITECH LOAD - PULSAR C6

## 📋 Descripción General

Este firmware actúa como **puente de comunicación entre una tarjeta Pulsar C6 y cargas variables ITECH**. 

**¿Qué hace?** Recibe comandos JSON desde una PC, los procesa y los envía a la carga ITECH para controlar:
- ✅ Activación/Desactivación de la carga
- ✅ Modo Dinámico: Alternancia automática entre dos valores de corriente
- ✅ Modo Lista: Ejecución de una secuencia de pasos de corriente
- ✅ Lectura de mediciones: Voltaje, Corriente y Potencia

---

## 🔧 Hardware - Conexiones

```
┌─────────────────────────────────────────────────────────┐
│                     PULSAR C6                           │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  Serial USB ──────> PC (Envía JSON, recibe respuestas) │
│                                                         │
│  GPIO 4 (RX) ──────> DB9 Pin 3 (RX Carga ITECH)       │
│  GPIO 5 (TX) ──────> DB9 Pin 2 (TX Carga ITECH)       │
│  GND ─────────────> DB9 Pin 5 (GND)                    │
│                                                         │
└─────────────────────────────────────────────────────────┘
                          ↓
                  ┌───────────────┐
                  │  CARGA ITECH  │
                  │  (Pruebas)    │
                  └───────────────┘
```

**Parámetros de comunicación:**
- **Baudrate:** 9600 bps (USB y UART1)
- **Datos:** 8 bits
- **Paridad:** Ninguna
- **Stop bits:** 1

---

## 📨 Formatos JSON de Entrada

### 1️⃣ ACTIVAR CARGA

**Para encender la carga (mantiene la corriente anterior o usa la predeterminada)**

```json
{
  "Funcion": "Other",
  "Config": {
    "Resolution": [],
    "Range": []
  },
  "Start": "CFG_ON"
}
```

**¿Qué hace internamente?**
- Envía comando SCPI `INP 1` a la carga
- La carga comienza a demandar corriente inmediatamente

**Respuesta esperada:**
```json
{"CONF":"CFG_ON","response":true}
```

**Ejemplo de uso:**
```
💻 ENTRADA: {"Funcion": "Other", "Config": {"Resolution": [], "Range": []}, "Start": "CFG_ON"}
⚡ ACCIÓN: Carga ITECH se ACTIVA
📱 SALIDA: {"CONF":"CFG_ON","response":true}
```

---

### 2️⃣ DESACTIVAR CARGA

**Para apagar la carga y detener cualquier operación en curso**

```json
{
  "Funcion": "Other",
  "Config": {
    "Resolution": [],
    "Range": []
  },
  "Start": "CFG_OFF"
}
```

**¿Qué hace internamente?**
- Envía comando SCPI `INP 0` a la carga
- Detiene modo dinámico si estaba activo
- Limpia la lista si estaba configurada

**Respuesta esperada:**
```json
{"CONF":"CFG_OFF","response":true}
```

**Ejemplo de uso:**
```
💻 ENTRADA: {"Funcion": "Other", "Config": {"Resolution": [], "Range": []}, "Start": "CFG_OFF"}
⚡ ACCIÓN: Carga ITECH se DESACTIVA
📱 SALIDA: {"CONF":"CFG_OFF","response":true}
```

---

### 3️⃣ MODO DINÁMICO (DYN)

**Alterna automáticamente entre DOS valores de corriente en intervalos definidos**

```json
{
  "Funcion": "DYN",
  "Config": {
    "Resolution": [3, 1, 3, 1],
    "Range": [50, 30]
  },
  "Start": "CFG_ON"
}
```

**Parámetros en `Resolution`:**
- `[0]` = **Corriente Estado A** → 3 Amperes
- `[1]` = **Tiempo en Estado A** → 1 segundo  
- `[2]` = **Corriente Estado B** → 3 Amperes
- `[3]` = **Tiempo en Estado B** → 1 segundo

**Parámetros en `Range`:**
- `[0]` = Voltaje máximo (ejemplo: 50V) - *No se usa actualmente*
- `[1]` = Rango de corriente máx (ejemplo: 30A)

**¿Qué hace internamente?**
1. Configura la carga para modo corriente
2. Establece rango máximo a 30A
3. Activa la carga (`INP 1`)
4. Comienza a alternar:
   - **1 seg @ 3A** → **1 seg @ 3A** → **Repite...**
5. La alternancia continúa hasta que se envíe `CFG_OFF`

**Respuesta esperada:**
```json
{"CONF":"DYN","response":true}
```

**Ejemplo de uso:**
```
💻 ENTRADA: 
{
  "Funcion": "DYN", 
  "Config": {"Resolution": [2.5, 0.5, 5.0, 1.5], "Range": [48, 30]}, 
  "Start": "CFG_ON"
}

⚡ ACCIÓN: 
- Carga alterna así:
  0.5 seg @ 2.5A
  1.5 seg @ 5.0A
  0.5 seg @ 2.5A
  1.5 seg @ 5.0A
  (repite hasta CFG_OFF)

📱 SALIDA: {"CONF":"DYN","response":true}
```

---

### 4️⃣ MODO LISTA (LIST)

**Ejecuta una SECUENCIA de pasos con diferentes corrientes y duraciones**

```json
{
  "Funcion": "LIST",
  "Config": {
    "Resolution": [10, 0.2, 0.5, "STEP1"],
    "Range": [50, 30]
  },
  "Start": "CFG_ON"
}
```

**Parámetros en `Resolution`:**
- `[0]` = **Número de Pasos** → 10 pasos
- `[1]` = **Corriente Base por Paso** → 0.2 A
- `[2]` = **Duración de cada Paso** → 0.5 segundos
- `[3]` = Identificador (ignorado actualmente)

**¿Cómo se genera la lista?**
El firmware **escala automáticamente** la corriente:
- Paso 1 = 1 × 0.2A = **0.2A**
- Paso 2 = 2 × 0.2A = **0.4A**
- Paso 3 = 3 × 0.2A = **0.6A**
- ...
- Paso 10 = 10 × 0.2A = **2.0A**

Cada paso dura **0.5 segundos**.

**¿Qué hace internamente?**
1. Configura la carga para modo corriente  
2. Establece rango máximo a 30A
3. Activa la carga (`INP 1`)
4. Ejecuta secuencialmente:
   - Aplica 0.2A durante 0.5 seg
   - Aplica 0.4A durante 0.5 seg
   - ...
   - Aplica 2.0A durante 0.5 seg
5. Desactiva la carga (`INP 0`)
6. Envía confirmación

**Respuesta esperada (inicio de ejecución):**
```json
{"CONF":"LIST","response":true}
```

**Respuesta esperada (fin de ejecución):**
```json
{"DEBUG":"LIST finalizado"}
```

**Cronograma de ejecución:**
```
Tiempo (seg) │ Corriente │ Estado
──────────────────────────────────
0.0 - 0.5    │   0.2A    │ Paso 1
0.5 - 1.0    │   0.4A    │ Paso 2
1.0 - 1.5    │   0.6A    │ Paso 3
...
2.0 - 2.5    │   1.0A    │ Paso 5
...
4.5 - 5.0    │   2.0A    │ Paso 10
```

**Ejemplo de uso:**
```
💻 ENTRADA: 
{
  "Funcion": "LIST", 
  "Config": {"Resolution": [3, 1.0, 2.0], "Range": [48, 30]}, 
  "Start": "CFG_ON"
}

⚡ ACCIÓN: 
- Paso 1: Aplica 1.0A durante 2.0 seg
- Paso 2: Aplica 2.0A durante 2.0 seg
- Paso 3: Aplica 3.0A durante 2.0 seg
- Desactiva carga
- Envía: {"DEBUG":"LIST finalizado"}

📱 SALIDA INICIOS: {"CONF":"LIST","response":true}
📱 SALIDA FINAL: {"DEBUG":"LIST finalizado"}
```

---

### 5️⃣ LEER MEDICIONES

**Obtiene las mediciones instantáneas de la carga: Voltaje, Corriente y Potencia**

```json
{
  "Funcion": "Other",
  "Config": {
    "Resolution": [],
    "Range": []
  },
  "Start": "Read"
}
```

**¿Qué hace internamente?**
1. Solicita a la carga ITECH: `MEAS:VOLT?;CURR?;POW?`
2. Espera respuesta máximo 300ms
3. Procesa y formatea los valores
4. Envía resultado JSON al PC

**Respuesta esperada:**
```json
{"medicion": "12.345;2.890;35.678"}
```

**Formato de la respuesta:**
```
"V.VVV;I.III;P.PPP"
  ↓     ↓     ↓
  V     I     P
  
Volts ; Amperes ; Wats
(3 decimales cada uno)
```

**Ejemplo de uso:**
```
💻 ENTRADA: {"Funcion": "Other", "Config": {"Resolution": [], "Range": []}, "Start": "Read"}

⚡ ACCIÓN: Lee V, I, P de la carga ITECH

📱 SALIDA: {"medicion": "48.523;5.123;247.845"}
           
Interpretación:
- Voltaje: 48.523 V
- Corriente: 5.123 A
- Potencia: 247.845 W
```

---

## 🎯 Resumen de Respuestas del Sistema

| Comando | Respuesta JSON | Significado |
|---------|---|---|
| `"Start": "CFG_ON"` | `{"CONF":"CFG_ON","response":true}` | Carga activada correctamente |
| `"Start": "CFG_OFF"` | `{"CONF":"CFG_OFF","response":true}` | Carga desactivada correctamente |
| `"Funcion": "DYN"` + `"Start": "CFG_ON"` | `{"CONF":"DYN","response":true}` | Modo dinámico activado |
| `"Funcion": "LIST"` + `"Start": "CFG_ON"` | `{"CONF":"LIST","response":true}` | Lista configurada, **iniciando ejecución** |
| (Lista finaliza) | `{"DEBUG":"LIST finalizado"}` | Secuencia completada |
| `"Start": "Read"` | `{"medicion": "V.VVV;I.III;P.PPP"}` | Mediciones instantáneas |
| Sistema listo | `{"System":"Ready"}` | Firmware inicializado al encender/resetear |

---

## 🚀 Guía de Uso Paso a Paso

### Escenario 1: Activar carga a corriente fija (10A)

```
Paso 1: Configurar corriente (previamente en la carga ITECH):
        ➜ Pulsar C6 debe enviar comando de configuración

Paso 2: Enviar JSON de activación:
        {"Funcion": "Other", "Config": {"Resolution": [], "Range": []}, "Start": "CFG_ON"}

Paso 3: Esperar respuesta:
        {"CONF":"CFG_ON","response":true}

Paso 4: La carga está activa @ 10A
```

### Escenario 2: Ejecutar prueba con modo dinámico (0.5A - 2.0A)

```
Paso 1: Enviar JSON de configuración DYN:
        {
          "Funcion": "DYN",
          "Config": {"Resolution": [0.5, 0.2, 2.0, 0.3], "Range": [50, 30]},
          "Start": "CFG_ON"
        }

Paso 2: Esperar respuesta:
        {"CONF":"DYN","response":true}

Paso 3: La carga alterna automáticamente:
        - 0.2 seg @ 0.5A
        - 0.3 seg @ 2.0A
        - 0.2 seg @ 0.5A
        - (repite...)

Paso 4: Cuando termines, detener:
        {"Funcion": "Other", "Config": {"Resolution": [], "Range": []}, "Start": "CFG_OFF"}

Paso 5: Esperar respuesta:
        {"CONF":"CFG_OFF","response":true}
```

### Escenario 3: Ejecutar secuencia de pasos (Caracterización)

```
Paso 1: Enviar JSON de configuración LIST:
        {
          "Funcion": "LIST",
          "Config": {"Resolution": [5, 0.5, 1.0], "Range": [50, 30]},
          "Start": "CFG_ON"
        }

Paso 2: Respuestas esperadas:
        Inicio: {"CONF":"LIST","response":true}
        Final:  {"DEBUG":"LIST finalizado"}

Paso 3: Secuencia ejecutada:
        Paso 1: 0.5A durante 1.0 seg
        Paso 2: 1.0A durante 1.0 seg
        Paso 3: 1.5A durante 1.0 seg
        Paso 4: 2.0A durante 1.0 seg
        Paso 5: 2.5A durante 1.0 seg
        ➜ Total: 5 segundos de prueba

Paso 4: Carga desactivada automáticamente
```

### Escenario 4: Monitorear durante carga

```
Mientras la carga está activa, puedes hacer lecturas con:
        {"Funcion": "Other", "Config": {"Resolution": [], "Range": []}, "Start": "Read"}

Respuesta:
        {"medicion": "12.456;0.523;6.514"}
        
Significa:
        - Voltaje: 12.456 V
        - Corriente: 0.523 A
        - Potencia: 6.514 W
```

---

## 📊 Tabla de Referencia Rápida

### JSON Template para cada función:

**ACTIVAR:**
```json
{"Funcion": "Other", "Config": {"Resolution": [], "Range": []}, "Start": "CFG_ON"}
```

**DESACTIVAR:**
```json
{"Funcion": "Other", "Config": {"Resolution": [], "Range": []}, "Start": "CFG_OFF"}
```

**DINÁMICO (CorrA=2A, TiempoA=0.5s, CorrB=5A, TiempoB=1s):**
```json
{"Funcion": "DYN", "Config": {"Resolution": [2, 0.5, 5, 1], "Range": [50, 30]}, "Start": "CFG_ON"}
```

**LISTA (10 pasos, 0.2A por paso, 0.5s cada paso):**
```json
{"Funcion": "LIST", "Config": {"Resolution": [10, 0.2, 0.5], "Range": [50, 30]}, "Start": "CFG_ON"}
```

**LEER:**
```json
{"Funcion": "Other", "Config": {"Resolution": [], "Range": []}, "Start": "Read"}
```

---

## ⚠️ Notas Importantes para Fabricación

1. **Baudrate:** Asegúrate que PC, Pulsar C6 y Carga ITECH estén configuradas a **9600 bps**

2. **Timeouts:** El firmware espera máximo **300ms** una respuesta de la carga

3. **Precisión:** Las mediciones se redondean a **3 decimales** (ejemplo: 5.123A)

4. **Modo LIST:** 
   - Máximo **30 pasos**
   - Las corrientes se escalan automáticamente (paso 1 = 1×corriente base, paso 2 = 2×corriente base, etc.)

5. **Modo DYN:**
   - La alternancia **continúa indefinidamente** hasta enviar `CFG_OFF`
   - Útil para pruebas de estabilidad térmica

6. **Orden de parámetros Range:**
   - `[0]` = Voltaje máx (actualmente no se usa)
   - `[1]` = Corriente máx (se configura en la carga)

7. **Si algo no responde:**
   - Verificar conexión USB
   - Verificar conexión DB9 entre Pulsar y Carga
   - Setear **baud rate en el puerto serial a 9600**
   - Resetear la Pulsar C6 (debería mostrar `{"System":"Ready"}`)

---

## 📝 Estructura Interna del Código

```
┌─────────────────────────────────────────────────────┐
│           main.ino (Firmware Principal)             │
├─────────────────────────────────────────────────────┤
│                                                     │
│ • sendSCPI()         → Envía comandos a carga     │
│ • takeMeasurements() → Lee V, I, P                 │
│ • configLoad()       → Configura DYN o LIST        │
│ • runSoftwareList()  → Ejecuta secuencia LIST     │
│                                                     │
│ • setup()            → Inicialización              │
│ • loop()             → Procesamiento de JSON       │
│                                                     │
└─────────────────────────────────────────────────────┘
```

---

## 🔄 Flujo de Comunicación en Tiempo Real

```
PC (Serial Monitor)
    │
    ├─> JSON {"Funcion":"DYN", ...}
    │
    ↓
PULSAR C6 (deserializa JSON)
    │
    ├─> configLoad() prepara variables
    │
    ├─> Responde: {"CONF":"DYN","response":true}
    │
    ├─> loop() alterna corriente cada intervalo
    │
    ├─> sendSCPI("CURRent 2.0")
    │
    ↓
CARGA ITECH
    │
    ├─> Recibe comando SCPI
    │
    ├─> Aplica corriente
    │
    ├─> (Espera intervalo A)
    │
    ├─> Recibe "CURRent 5.0"
    │
    ├─> Cambia a 5.0A
    │
    └─> (Repite hasta CFG_OFF)
```

---

## 📞 Soporte Técnico

Para dudas o problemas:
1. Revisar los "Códigos ARDUINO" en la carpeta del TestBench
2. Verificar conexiones físicas (USB, DB9)
3. Confirmar baudrate = 9600 en todas las interfaces
4. Enviar el JSON exacto (sin espacios adicionales) si el comando no responde

---

**Última actualización:** Marzo 2026  
**Versión del Firmware:** Con comentarios y documentación completa
