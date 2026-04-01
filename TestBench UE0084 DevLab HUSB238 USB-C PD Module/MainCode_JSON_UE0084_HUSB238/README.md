# TestBench UE0084 - HUSB238 USB-C Power Delivery Module

## 📋 Descripción General

Código de integración para el testbench del módulo **HUSB238** (Controlador de Negociación USB Power Delivery).  
Este firmware controla de forma remota un módulo USB-C Power Delivery, permitiendo:

- 🔌 Selección dinámica de voltajes (5V, 9V, 12V, 15V, 18V, 20V)
- 📊 Barrido automático de voltajes para mediciones
- 🔧 Establecimiento de voltajes fijos para caracterización
- 📡 Comunicación JSON con interfaz web (PagWeb)
- ⚡ Monitoreo de corriente máxima disponible
- 🔌 Control auxiliar de relevador para regulador 3.3V

---

## 🔧 Especificaciones Técnicas

### Hardware
| Componente | Descripción |
|---|---|
| **MCU** | ESP32 |
| **Módulo PD** | HUSB238 (I2C) |
| **Interfaz Web** | PagWeb (UART2: TX=19, RX=15) |
| **Debug Serial** | UART0 (115200 baud) |
| **Bus I2C** | SDA=22, SCL=23 |
| **Relay Control** | GPIO17 |
| **Button Input** | GPIO4 |

### Velocidades de Comunicación
```
Serial Monitor (Debug):     115200 baud, 8N1
PagWeb (Control):          115200 baud, 8N1
I2C (HUSB238):            400kHz (estándar)
```

---

## 📡 Protocolo JSON

### Estructura General de Comandos

**Entrada (desde PagWeb):**
```json
{
  "Function": "comando",
  "Value": "parámetro_opcional"
}
```

**Salida (hacia PagWeb):**
```json
{
  "Result": "OK|FAIL",
  "debug": "mensaje_descriptivo"
}
```

---

## 🎮 Modos de Operación

### 1️⃣ PING - Prueba de Conectividad
Verifica que la comunicación JSON está operativa.

**Comando:**
```json
{"Function": "ping"}
```

**Respuesta:**
```json
{"ping": "pong"}
```

---

### 2️⃣ INIT_HUSB - Inicialización del Módulo
Inicializa el módulo HUSB238 mediante I2C.  
⚠️ **Debe ejecutarse antes de cualquier operación PD**.

**Comando:**
```json
{"Function": "init_husb"}
```

**Respuesta:**
```json
{
  "Result": "OK",
  "debug": "HSUB238 Inicializado"
}
```

**Lógica:**
- Intenta 3 veces inicializar el módulo
- Espera 100ms entre intentos
- Activa bandera `state_husb` si es exitoso

---

### 3️⃣ SWEEP - Barrido de Voltajes
Realiza un barrido automático por los voltajes principales: **5V → 9V → 12V → 15V → 20V**

**Comando:**
```json
{"Function": "sweep"}
```

**Respuesta (por cada voltaje):**
```json
{"debug": "Voltaje 5 V"}
{"debug": "Voltaje 9 V"}
{"debug": "Voltaje 12 V"}
... (cada 2 segundos)
```

**Características:**
- ⏱️ **Duración por voltaje:** 2 segundos
- 🔄 **Ciclo completo:** ~10 segundos
- 📡 **Notificación:** Envía JSON por cada cambio
- ❌ **Validación:** Requiere `state_husb == true`

**Tabla de Barrido:**
| Paso | Voltaje | Duración |
|------|---------|----------|
| 1 | 5V | 2s |
| 2 | 9V | 2s |
| 3 | 12V | 2s |
| 4 | 15V | 2s |
| 5 | 20V | 2s |

---

### 4️⃣ FIXED - Voltaje Fijo
Establece un voltaje constante para mediciones o caracterización.

**Comando:**
```json
{"Function": "fixed", "Value": "12"}
```

**Parámetros válidos para Value:**
- `"5"` → 5V
- `"9"` → 9V
- `"12"` → 12V
- `"15"` → 15V
- `"18"` → 18V (si disponible)
- `"20"` → 20V

**Respuesta:**
```json
{
  "debug": "Voltaje fijo seteado a 12 V"
}
```

**Validaciones:**
- ✅ Verifica que el módulo esté inicializado
- ✅ Verifica que el voltaje esté disponible en el adaptador
- ❌ Retorna error si falta parámetro `Value`

---

### 5️⃣ RELAY ON / OFF - Control del Relevador

Activa/desactiva el relevátor de control (GPIO17) para el regulador 3.3V.

**Encender Relevador:**
```json
{"Function": "relayOn"}
```

**Apagar Relevador:**
```json
{"Function": "relayOff"}
```

**Comportamiento:**
- Cambio inmediato de estado en GPIO17
- Retardo de 100ms para estabilización

---

### 6️⃣ RESTART - Reinicio del Dispositivo

Reinicia completamente el ESP32.

**Comando:**
```json
{"Function": "restart"}
```

---

## 🖥️ Protocolo SCPI (Serial Commands)

Además de JSON, el código soporta comandos SCPI por Serial Monitor para debugging.

### Comandos Disponibles

| Comando | Descripción | Ejemplo |
|---------|-------------|---------|
| `*IDN?` | Identificación del instrumento | → `UNIT-DEVLAB,HUSB238,USBPD,1.0` |
| `STAT?` | Estado de conexión PD | → `ATTACHED` / `UNATTACHED` |
| `PD:LIST?` | Voltajes disponibles | → `5V 9V 12V 15V 20V` |
| `PD:GET?` | Voltaje actual | → `PD=12` |
| `PD:SET <V>` | Establece voltaje | `PD:SET 12` |
| `PD:SWEEP` | Barrido de voltajes | - |
| `CURR:GET?` | Corriente actual | → `CURR=2.0A` |
| `CURR:MAX? <V>` | Corriente máxima a V | `CURR:MAX? 12` → `MAX_CURR@12V=2.0A` |

### Ejemplo de Sesión SCPI

```
> *IDN?
< UNIT-DEVLAB,HUSB238,USBPD,1.0

> STAT?
< ATTACHED

> PD:LIST?
< 5V 9V 12V 15V 20V

> PD:SET 12
< OK:SET 12V

> CURR:GET?
< CURR=2.0A
```

---

## 📊 Diagramas de Flujo

### Flujo de Inicialización
```
┌─────────────────────┐
│  SETUP (Inicio)     │
└──────────┬──────────┘
           │
           ├─► Serial.begin(115200)
           ├─► PagWeb.begin(115200)
           ├─► Wire.begin(I2C)
           └─► GPIO Init
           
┌──────────┴──────────┐
│  Esperando entrada   │
│   (LOOP principal)   │
└─────────────────────┘
```

### Flujo del Modo SWEEP
```
┌──────────────────────┐
│ {"Function":"sweep"} │
└──────────┬───────────┘
           │
      ¿Inicializado?
      /           \
    SÍ            NO
    │              │
    │         Error JSON
    │
PD:SET 5V ──► Espera 2s
    │
PD:SET 9V ──► Espera 2s
    │
PD:SET 12V ──► Espera 2s
    │
PD:SET 15V ──► Espera 2s
    │
PD:SET 20V ──► Espera 2s
    │
  FIN ──► Respuesta JSON
```

---

## 🔌 Estructura del Código

### Archivos Principales
```
MainCode_JSON_UE0084_HUSB238.ino
├── Includes y definiciones
├── setup()                    # Inicialización de hardware
├── loop()                     # Bucle principal
├── handleSCPI()              # Procesador de comandos SCPI
└── Funciones auxiliares
    ├── printCurrentValue()   # Conversión de corriente a texto
    └── printCurrentSetting() # Conversión alternativa
```

### Variables Globales Importantes
```cpp
HardwareSerial PagWeb(1);           // UART para comunicación web
Adafruit_HUSB238 husb238;           // Objeto del módulo HUSB238
bool state_husb;                    // Bandera de inicialización
StaticJsonDocument<256> receiveJSON; // JSON entrante
StaticJsonDocument<256> sendJSON;   // JSON saliente
```

---

## ⚡ Ejemplo de Uso Completo

### Secuencia de Inicialización y Medición

```json
// 1. Verificar conectividad
{"Function": "ping"}
// → {"ping": "pong"}

// 2. Inicializar módulo HUSB238
{"Function": "init_husb"}
// → {"Result": "OK", "debug": "HSUB238 Inicializado"}

// 3. Verificar voltajes disponibles en el adaptador
// (Enviar comando SCPI: "PD:LIST?" por terminal)

// 4. Establecer voltaje fijo para medición
{"Function": "fixed", "Value": "12"}
// → {"debug": "Voltaje fijo seteado a 12 V"}

// 5. (Hacer mediciones)

// 6. Cambiar a siguiente voltaje
{"Function": "fixed", "Value": "15"}
// → {"debug": "Voltaje fijo seteado a 15 V"}

// 7. O realizar barrido automático
{"Function": "sweep"}
// → {"debug": "Voltaje 5 V"}
//   {"debug": "Voltaje 9 V"}
//   ... (cada 2 segundos)
```

---

## 🐛 Debugging

### Puertos Disponibles

**Puerto Serial (Monitor de Arduino):**
- Mensajes de debugging del firmware
- Comandos SCPI manuales
- Información de estado

**Interfaz JSON (PagWeb):**
- Control remoto
- Respuestas de estado
- Notificaciones de cambios

### Errores Comunes

| Error | Causa | Solución |
|-------|-------|----------|
| `HSUB238 No Inicializado` | Módulo no detectado | Revisar conexión I2C (SDA=22, SCL=23) |
| `Error: Falta parametro Value` | JSON incompleto | Verificar formato: `{"Function":"fixed", "Value":"12"}` |
| `ERR:UNAVAILABLE` | Voltaje no soportado por adaptador | Usar voltajes reales: 5,9,12,15,20 |
| `UNATTACHED` | Adaptador PD desconectado | Conectar adaptador USB-C PD |

---

## 📦 Librerías Requeridas

```
- Wire.h                (comunicación I2C - incluida)
- Adafruit_HUSB238.h    (driver del módulo HUSB238)
- HardwareSerial.h      (UART - incluida)
- Arduino.h             (API Arduino - incluida)
- ArduinoJson.h         (parseo/generación JSON)
```

**Instalación:**
```
Arduino IDE → Sketch → Include Library → Manage Libraries
- Buscar: "Adafruit HUSB238"
- Buscar: "ArduinoJson"
```

---

## 🎯 Características Principales

✅ **Comunicación Bidireccional JSON** - Control remoto desde PagWeb  
✅ **Múltiples Modos de Operación** - Sweep, Fixed, Init, Control  
✅ **Validación de Estado** - Verifica inicialización antes de operar  
✅ **Manejo de Errores** - Respuestas JSON descriptivas  
✅ **Protocolo SCPI** - Compatibilidad con instrumentos estándar  
✅ **Monitoring de Corriente** - Lectura de corriente máxima disponible  
✅ **Control Auxiliar** - Relevador para regulador 3.3V  

---

## 📝 Notas de Implementación

### Reintentos de Inicialización
El módulo intenta 3 veces inicializarse con esperas de 100ms entre intentos para mejorar la confiabilidad.

### Debounce del Botón
El botón implementa debounce por software (150ms) para evitar contactos ruidosos.

### Uso de Memoria
```
- JSON entrada:  256 bytes estáticos
- JSON salida:   256 bytes estáticos
- Strings:       ~variables (limitados por heap)
```

### Seguridad
- No hay validación criptográfica (interface privada recomendada)
- Control de errores básico
- No hay protección contra comandos maliciosos

---

## 🔄 Control de Versiones

| Versión | Fecha | Cambios |
|---------|-------|---------|
| 1.0 | 2026-04-01 | Release inicial con soporte SWEEP y FIXED |

---

## 📞 Contacto y Soporte

Para reportar problemas o sugerencias:
- Revisar mensajes de debugging en Serial Monitor
- Verificar conexiones I2C y UART
- Confirmar que el adaptador USB-C PD esté conectado

---

**TestBench UE0084** © 2026 - Test Engineering
