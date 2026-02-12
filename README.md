# Test-Engineering Repository

Repositorio centralizado de bancos de prueba (TestBench) para componentes electrónicos, sensores y módulos. Contiene código Arduino/ESP32 C6, ejemplos, documentación e integración con la plataforma de pruebas desarrollada por **I2D UNIT Electronics**.

## 📋 Descripción General

Colección completa de **TestBench** e **integración de hardware** para validación en línea de producción usando la plataforma **Pulsar C6 (ESP32 C6)**:

- 🔧 **30+ TestBench** funcionales: sensores, actuadores, módulos especializados
- 📡 **Integración JSON/UART** con plataforma de pruebas centralizada
- 📊 **Pulsar C6**: microcontrolador ESP32 C6 como hub de pruebas
- 🧪 **Validación automática** de componentes en manufactura
- 💾 **Código probado en producción**

Cada TestBench incluye:
- ✅ Código Arduino/ESP32 compilado y probado
- ✅ Ejemplos simples de inicio rápido
- ✅ Documentación de hardware, pines y protocolos
- ✅ Integración JSON con plataforma centralizada
- ✅ Troubleshooting y notas técnicas

## 📁 Estructura del Repositorio

```
Test-Engineering/
├── .git/                                            # Control de versiones Git
├── .gitignore
│
├── Documentacion/                                   # Docs generales y especificaciones
│   └── TestBench_UE0061_Pulsar_C6/                # Hardware Pulsar C6
│
│
├── Proyectos/                                      # Proyectos especializados
│   ├── Generador_Funciones/
│   ├── Simulador_Carga_Variable/
│   └── TestBench Guardia Nacional */
│
├── TestBench [Componentes Legacy]/                 # Módulos anteriores
│   ├── AR3775 UNIT Expansor ESP32 30 pines/
│   ├── AR4353 Señuelo de Carga/
│   ├── LM2596 Regulador de Voltaje/
│   └── Módulo DualOne/
│
├── TestBench UE00xx [Sensores & Módulos]/         # TestBench Pulsar C6 (30+ actuales)
│   ├── UE0022 MCU_DualOne/
│   ├── UE0065 DevLab I2C DRV2605L Haptic Motor Controller Module/
│   ├── UE0066 DevLab I2C BMM150 Magnetometer Sensor/
│   ├── UE0068 DevLab I2C BMI270 6-Axis IMU Sensor/
│   ├── UE0071 PulsarH2/
│   ├── UE0081 JUNR3/
│   ├── UE0082 DevLab G6K2GYTR 5V Relay Module/
│   ├── UE0083 DevLab PWM AO4410 2-Channel Output Module/
│   ├── UE0087 DevLab DCDC TPS61023 Boost Converter Module/
│   ├── UE0088 DevLab 80dB Passive Buzzer Module/
│   ├── UE0089 Cargador de Baterías/
│   ├── UE0090 DevLab CH552 MultiProtocol Programmer Module/
│   ├── UE0093 LockNode/
│   ├── UE0094 DevLab I2C ICP-10111 Barometric Pressure Sensor/
│   ├── UE0095 DevLab I2C BME688 Environmental 4-in-1 Sensor/
│   ├── UE0098 TEMT6000/
│   ├── UE0099/
│   ├── UE0101 DevLab TMP235 Analog Temperature Sensor/
│   ├── UE0102 PY32F003X/
│   ├── UE0109 DevLab I2C BMA255 UltraLow Power Accelerometer Sensor/
│   └── ... más TestBench
│
└── README.md                                       # Este archivo
```

## 🔧 TestBench Disponibles

### Sensores de Temperatura & Presión
| Código | Componente | Interfaz | Estado | Docs |
|--------|------------|----------|--------|------|
| **UE0101** | TMP235 Analog Temperature Sensor | ADC | ✅ Producción | [README](TestBench%20UE0101%20DevLab%20TMP235%20Analog%20Temperature%20Sensor/README.md) |
| **UE0094** | ICP-10111 Barometric Pressure | I2C | ✅ Probado | › |
| **UE0098** | TEMT6000 Light Sensor | ADC | ✅ Probado | › |

### Acelerómetros & IMU
| Código | Componente | Interfaz | Estado | Docs |
|--------|------------|----------|--------|------|
| **UE0109** | BMA255 UltraLow Power Accelerometer | I2C/SPI | ✅ Producción | [README](TestBench%20UE0109%20DevLab%20I2C%20BMA255%20UltraLow%20Power%20Accelerometer%20Sensor/README.md) |
| **UE0068** | BMI270 6-Axis IMU Sensor | I2C/SPI | ✅ Probado | › |

### Sensores Magnéticos & Ambientales
| Código | Componente | Interfaz | Estado | Docs |
|--------|------------|----------|--------|------|
| **UE0066** | BMM150 Magnetometer | I2C/SPI | ✅ Producción | [README](TestBench%20UE0066%20DevLab%20I2C%20BMM150%20Magnetometer%20Sensor/README.md) |
| **UE0095** | BME688 4-in-1 Environmental | I2C/SPI | ✅ Probado | › |

### Módulos de Control & Actuadores
| Código | Componente | Interfaz | Estado | Docs |
|--------|------------|----------|--------|------|
| **UE0065** | DRV2605L Haptic Motor Controller | I2C | ✅ Probado | › |
| **UE0082** | G6K2GYTR 5V Relay Module | GPIO | ✅ Probado | › |
| **UE0083** | AO4410 2-Channel PWM Output | GPIO/PWM | ✅ Probado | › |
| **UE0088** | 80dB Passive Buzzer Module | GPIO | ✅ Probado | › |
| **UE0087** | TPS61023 Boost Converter | GPIO | ✅ Probado | › |

### Plataformas & MCUs
| Código | Componente | Descripción | Estado | Docs |
|--------|------------|-------------|--------|------|
| **UE0022** | MCU_DualOne | Dual core microcontroller | ✅ Producción | › |
| **UE0071** | PulsarH2 | High-performance variant | ✅ Producción | › |
| **UE0081** | JUNR3 | Specialized controller | ✅ Probado | › |
| **UE0102** | PY32F003X | Low-cost MCU | ✅ Probado | › |
| **UE0090** | CH552 MultiProtocol Programmer | Programmer module | ✅ Probado | › |
| **UE0093** | LockNode | Secure node device | ✅ Desarrollo | › |
| **UE0099** | [Desarrollo] | | 🔨 En progreso | › |

### Módulos Especializados
| Código | Componente | Propósito | Estado | Docs |
|--------|------------|----------|--------|------|
| **UE0089** | Cargador de Baterías | Battery Management | ✅ Producción | › |
| **AR3775** | UNIT Expansor ESP32 30 pines | Expansión ESP32 | ✅ Producción | › |
| **AR4353** | Señuelo de Carga | Load Simulator | ✅ Producción | › |
| **LM2596** | Regulador de Voltaje | Buck Converter | ✅ Producción | › |

## 🚀 Empezar

### Requisitos mínimos

```
Hardware:
- ESP32 DevKit C (o compatible)
- Cable USB para programación
- Sensores/módulos específicos según TestBench

Software:
- Arduino IDE 1.8.x o superior
- PlatformIO (recomendado)
- Python 3.8+ (para scripts MicroPython)
- Git
```

### Instalación rápida (Arduino IDE)

1. **Clonar el repositorio**:
   ```bash
   git clone https://github.com/tuuser/Test-Engineering.git
   cd Test-Engineering
   ```

2. **Instalar dependencias de Arduino**:
   - Ir a `Tools > Manage Libraries...`
   - Instalar: `ArduinoJson`, `Adafruit_GFX`, `Adafruit_SSD1306`, `Adafruit_NeoPixel`

3. **Seleccionar board**:
   - `Tools > Board > ESP32 > ESP32 Dev Module`
   - `Tools > Port > [tu puerto COM]`

4. **Cargar sketch**:
   - Abrir el .ino del TestBench
   - Compilar y descargar

5. **Monitor Serial**:
   - `Tools > Serial Monitor` (115200 baud)

### Estructura típica de un TestBench

```
TestBench UE00XX - Nombre del Sensor/
├── README.md                      # Documentación completa
├── Códigos ARDUINO/               # Sketch Arduino/ESP32
│   ├── example_*.ino              # Ejemplos simples
│   └── main_code_*.ino            # Código principal
├── Códigos C/                     # Código C puro (opcional)
├── Códigos Python/                # Scripts MicroPython (opcional)
│   └── AutoDetect/
│       ├── main.py                # Ejemplo de uso
│       └── driver.py              # Drivers/librerías
└── example_*/                     # Carpetas de ejemplos especializados
    └── *.ino
```

## 📊 Plataforma de Pruebas - I2D UNIT Electronics

Plataforma centralizada de control y monitoreo en tiempo real para validación de TestBench.

**Desarrollada por**: Equipo de I2D - UNIT Electronics  
**Tecnología**: Vue.js + TypeScript  
**Integración**: UART JSON @ 115200 baud con Pulsar C6 (ESP32 C6)

### 🔗 Acceso a la Plataforma

**Servidor en red UNIT Electronics (Local)**:
```
https://192.168.15.6:6443/
```

### Características
- 🎯 **Control bidireccional** vía UART/JSON desde Pulsar C6
- 📈 **Gráficas en tiempo real** de todos los sensores
- 💾 **Almacenamiento de datos** históricos y exportación
- 🔌 **Auto-detección** de TestBench conectados
- 🧪 **Validación automática** de módulos en línea de producción
- 📱 **Interfaz responsiva** y moderna
- 🏗️ **Arquitectura escalable** para múltiples TestBench

### Ambiente Local (Desarrollo)

Para ejecutar la plataforma en tu máquina:

Abre http://localhost:5173 en tu navegador.

### Flujo de Pruebas

```
┌─────────────────────────────────────┐
│   TestBench (Pulsar C6 ESP32 C6)    │
│   - Sensores/Módulos                │
│   - Código Arduino/C                │
└──────────────┬──────────────────────┘
               │
               ↓ UART @ 115200 baud
               │ Protocolo JSON
               ↓
┌──────────────────────────────────────┐
│  Plataforma de Pruebas (I2D UNIT)    │
│  https://192.168.15.6:6443/          │
│  - Recepción de datos                │
│  - Control de componentes            │
│  - Almacenamiento integral           │
└──────────────┬───────────────────────┘
               │
               ↓
      Validación & Resultados
```

Todo el código de este repositorio está **optimizado y probado** para ejecutarse en la **Pulsar C6** conectada a la plataforma centralizada de I2D.

## 📡 Protocolo de Comunicación

Todos los TestBench modernos usan **JSON sobre UART** a **115200 baud**:

### Ejemplo: Lectura de sensor

**Request (desde PagWeb)**:
```json
{"Function":"read_sensor","param":"temperature"}
```

**Response (desde ESP32)**:
```json
{"resp":"OK","value":23.5,"unit":"C","timestamp":1234567890}
```

### Comandos universales

- `{"Function":"ping"}` → Validar comunicación
- `{"Function":"reset"}` → Reiniciar módulo
- `{"Function":"info"}` → Obtener información del dispositivo

## 🔍 Características por TestBench

### I2C / SPI
- Autodetección automática de interfaz
- Soporte multi-dirección (0x18, 0x19, etc.)
- Reintentos configurables
- Tiempos de espera optimizados

### ADC (Analógicos)
- Resolución 12-bit
- Calibración integrada
- Conversión automática a unidades físicas

### JSON
- Serialización ArduinoJson 6.x
- Descompresión en PagWeb
- Validación de campos
- Timestamps automáticos

## 📚 Documentación Adicional

```
Documentacion/
└── TestBench_UE0061_Pulsar_C6/     # Especificaciones completas de hardware

Cada TestBench incluye:
├── README.md                        # Guía completa
├── Esquemáticos (en algunos)
├── Listas de pines (pinout)
├── Ejemplos JSON
└── Notas de troubleshooting
```

## 🔧 Solución de problemas comunes

### "Puerto COM no disponible"
```bash
# Windows - listar puertos
wmic logicaldisk get name

# Linux/Mac - listar puertos
ls /dev/tty*
```

### "ArduinoJson no deserializa"
- Verificar terminador `\n` en JSON
- Validar JSON sintaxis en https://jsonlint.com/
- Aumentar tamaño del buffer si es necesario

### "Error de compilación: wifi.h no encontrado"
- Instalar ESP32 board desde Board Manager
- Verificar que seleccionaste la board correcta

### "Sensor no responde en I2C"
- Revisar pull-ups (usualmente 4.7 kΩ)
- Comprobar voltaje (típicamente 3.3V)
- Verificar dirección I2C correcta con `i2c_scanner.ino`

## 🤝 Contribuciones

Para añadir un nuevo TestBench:

1. Crear carpeta: `TestBench UE00XX - [Nombre]`
2. Incluir:
   - `README.md` con estructura estándar
   - Código Arduino en `Códigos ARDUINO/`
   - Ejemplos simples
   - Documentación de pines
3. Actualizar tabla de TestBench en este README
4. Hacer push con descripción clara

## 📋 Licencia

[Especificar licencia local]

## 👥 Autores & Contribuidores

- **Emiliano Molina** — Desarrollo y mantenimiento de Ingeniería de Pruebas
- **I2D UNIT Electronics** — Plataforma de pruebas centralizada
- Última actualización: **12 Febrero 2026**

## 📞 Contacto & Soporte

- 📧 Email: 
- 🐛 Issues: GitHub Issues
- 📖 Documentación detallada: `/Documentacion`

---

**Estado del Proyecto**: ✅ Activo en producción  
Repositorio en desarrollo continuo con nuevos TestBench regularmente.

**Plataforma de Pruebas**: 🔗 https://192.168.15.6:6443/  
Acceso exclusivo en red UNIT Electronics.

**Última revisión**: 12/02/2026
