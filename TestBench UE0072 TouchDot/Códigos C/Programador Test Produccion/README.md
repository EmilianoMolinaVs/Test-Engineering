# Programador Test Producción - TouchDot UE0072

Sistema automático para flashear y programar placas **ESP32-S3** con firmware **TouchDot UE0072**.

**Propósito**: Programación rápida y reproducible de múltiples dispositivos ESP32-S3 en línea de producción.

---

## Contenido de la Carpeta

### Scripts de Flasheo
- **`flash_prod_esp32.sh`** - Script automático para producción. Espera indefinidamente a que conectes un ESP32, lo flashea y vuelve a esperar (ideal para línea de montaje).
- **`flash_multi_esp32.sh`** - Script interactivo con menú para programar múltiples ESP32 ó modificar configuración.

### Firmware
- **`blink_TouchDot.ino.merged.bin`** - Binario compilado listo para flashear. Incluye bootloader y particiones.

### Código Fuente
- **`MainCode_TouchDot_UE0072_JSON/`** - Carpeta con el código fuente (.ino) y proyecto de compilación.
  - `MainCode_TouchDot_UE0072_JSON.ino` - Código principal
  - `build/` - Artefactos compilados

### Logs
- **`log_COMxx.txt`** - Archivos de registro de flasheos previos (para debugging).

### Carpetas
- **`build/`** - Caché de compilación y binarios intermedios.

---

## Requisitos Previos (Windows)

### 1. **Python 3.6 o superior**
Descargar e instalar desde [python.org](https://www.python.org/downloads/)
- ✅ Marcar **"Add Python to PATH"** durante la instalación
- Verificar: Abrir PowerShell y ejecutar:
  ```powershell
  python --version
  ```

### 2. **Git Bash (para ejecutar scripts .sh)**
Descargar desde [git-scm.com](https://git-scm.com/download/win)
- Instalar con opciones por defecto
- Permite ejecutar scripts Bash en Windows

### 3. **Paquetes Python necesarios**
Abrir PowerShell o Command Prompt y ejecutar:

```powershell
pip install pyserial esptool
```

Verificar instalación:
```powershell
esptool.py version
pip list | findstr esptool
```

### 4. **Drivers USB para ESP32-S3**
Los drivers suelen ser automáticos, pero si aparece dispositivo sin reconocer en Device Manager:

**Opción A - CH340 (común en placas chinas):**
- Descargar: [CH340 Driver](http://www.wch.cn/downloads/CH341SER_EXE.html)
- Instalar y reiniciar

**Opción B - Verificar con esptool:**
```powershell
esptool.py chip_id
```
Conecta el ESP32 y ejecuta esto. Si detecta automáticamente, el driver está OK.

---

## Cómo Usar

### **Opción 1: Modo Producción (RECOMENDADO para línea de montaje)**

```bash
./flash_prod_esp32.sh
```

**Flujo:**
1. Script espera indefinidamente
2. Conecta un ESP32 por USB
3. Se detecta automáticamente
4. Se flashea con `blink_TouchDot.ino.merged.bin`
5. Vuelve a esperar por el siguiente dispositivo
6. Presiona `q` para salir

### **Opción 2: Múltiples Dispositivos (modo interactivo)**

```bash
./flash_multi_esp32.sh
```

**Menú disponible:**
- **1** - Verificar ID de chip en todos los puertos
- **2** - Borrar memoria flash de todos
- **3** - Grabar firmware
- **4** - Redetectar puertos conectados
- **R** - Cambiar velocidad BAUD
- **Q** - Salir

### **Opción 3: Flashear un ESP32 Individual (desde PowerShell)**

```powershell
esptool.py --port COM3 write_flash -z 0x0 ./blink_TouchDot.ino.merged.bin
```

Reemplaza `COM3` por el puerto correcto.

---

## Troubleshooting

### ❌ **Error: "esptool: command not found"**
- Reinstala Python y asegúrate que está en PATH
- Intenta: `python -m esptool ...` en lugar de `esptool ...`

### ❌ **Error: "No devices detected" / "ListPorts failed"**
- Abre Device Manager (Win+X → Device Manager)
- Busca "COM" en los puertos
- Si aparece con símbolo `⚠`, falta driver USB
- Instala drivers CH340 o FTDI según corresponda

### ❌ **Port busy o access denied**
- Cierra Arduino IDE, PuTTY, o cualquier software que use el puerto
- Reinicia VS Code o la terminal

### ❌ **Flasheo fallando a mitad**
- Intenta con velocidad BAUD más baja: edita `BAUD="460800"` en el script
- Revisa el cable USB (algunos cables no soportan datos)
- Prueba otro puerto USB en el PC

### ✅ **Verificar que la programación fue exitosa**
```powershell
esptool.py --port COM3 chip_id
```

Debería mostrar el ID del chip ESP32-S3.

---

## Notas Importantes

- Los scripts detectan automáticamente puertos COM asignados a ESP32 (busca IDs: 10C4, 1A86, 303A)
- En modo paralelo, se generan logs `log_COMx.txt` por cada dispositivo
- El firmware es `.merged.bin` (incluye bootloader + app + particiones)
- La configuración BAUD por defecto es 921600 bps (muy rápido)

---

## Comandos de Prueba Rápida

```bash
# Desde Git Bash en esta carpeta:

# Detectar puertos disponibles
python -c "import serial.tools.list_ports; [print(p.device) for p in serial.tools.list_ports.comports()]"

# Ver ID del chip
esptool.py --port COM3 chip_id

# Ver info de flash
esptool.py --port COM3 flash_id
```
