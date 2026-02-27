# Programador Test Producción

Este directorio contiene las herramientas y los binarios finales usados por el equipo de fabricación para programar las placas ESP32.

**Propósito**: flashear lotes de dispositivos de forma rápida y reproducible.

**Archivos en esta carpeta**
- `blink_ESP32_VSV.ino.merged.bin` : binario final para ESP32.
- `flash_multi_esp32.sh` : script interactivo para grabar múltiples ESP32 (usa `esptool`).
- `log_COM34.txt` : log de grabación de pruebas anteriores.

Requisitos previos
- Tener acceso a un terminal Bash (Git Bash, WSL o similar) en Windows.
- Python instalado con el paquete `pyserial` y `esptool` disponible:

```bash
pip install pyserial esptool
```

- Permisos para acceder a puertos COM en el equipo de flashing.

Resumen de uso

1) Preparación física
- Conectar las placas ESP32 por USB.
- Verificar en el PC los puertos COM asignados.

2) Flasheo ESP32 (múltiples)
- El script `flash_multi_esp32.sh` detecta automáticamente los puertos soportados y ofrece un menú.
- Modos de grabación configurables dentro del script:
  - `S` = Secuencial (uno a uno).
  - `P` = Paralelo (graba todos a la vez y genera logs `log_<PUERTO>.txt`).

Ejemplo (desde Bash en esta carpeta):

```bash
./flash_multi_esp32.sh
```

Pasos típicos dentro del menú:
- 1 Verificar conexión (chip id) en todos los puertos.
- 2 Borrar flash en todos.
- 3 Grabar firmware (usa `blink_ESP32_VSV.ino.merged.bin` por defecto).
- 4 Redetectar puertos.

Logs
- En modo paralelo el script crea archivos `log_COMx.txt` por puerto. Si una unidad falla, revisar su `log_<PUERTO>.txt`.

2) Flasheo ESP32 (múltiples)
- El script `flash_multi_esp32.sh` detecta automáticamente los puertos soportados y ofrece un menú.
- Modos de grabación configurables dentro del script:
  - `S` = Secuencial (uno a uno).
  - `P` = Paralelo (graba todos a la vez y genera logs `log_<PUERTO>.txt`).

Ejemplo (desde Bash en esta carpeta):

```bash
./flash_multi_esp32.sh
```

Pasos típicos dentro del menú:
- 1 Verificar conexión (chip id) en todos los puertos.
- 2 Borrar flash en todos.
- 3 Grabar firmware (usa `DualOneESP32.ino.merged.bin` por defecto).
- 4 Redetectar puertos.

Logs
- En modo paralelo el script crea archivos `log_COMx.txt` por puerto. Si una unidad falla, revisar su `log_<PUERTO>.txt`.

3) Flasheo RP2040 (múltiples)
- El script `flash_multi_rp2040.sh` detecta unidades tipo RP2040 buscando `INFO_UF2.TXT` en las letras de unidad (d..z).
- Copia el archivo UF2 (`DualOne.ino.uf2`) a cada unidad detectada en paralelo. Las placas se reinician automáticamente después de copiar.

Ejemplo (desde Bash en esta carpeta):

```bash
./flash_multi_rp2040.sh
```

Flujo típico en RP2040:
- Seleccionar opción "Redetectar" para comprobar unidades.
- Elegir "Flashear TODAS" para copiar el UF2 a todas las unidades detectadas.

Comandos útiles (PowerShell / Bash)
- Abrir Git Bash o WSL y situarse en esta carpeta:

```bash
cd "C:/Users/emili/OneDrive/Documentos/Ingenieria de Pruebas/Test-Engineering/TestBench UE0022 MCU_DualOne/Programador Test Produccion"
./flash_multi_esp32.sh
./flash_multi_rp2040.sh
```

Notas de verificación (post-flash)
- ESP32: usar `esptool --port COMx chip_id` o verificar que el dispositivo responde.
- RP2040: la unidad desaparece al reiniciarse; comprobar que el dispositivo aparece en el sistema como prevista la placa de destino.

Solución de problemas rápida
- Si no detecta ESP32: comprobar drivers CP210x/CH340/USB-Serial y que el cable soporta datos.
- Si no detecta RP2040: asegurar que la placa está en modo BOOTSEL y que aparece como unidad removible.
- Revisa los archivos `log_<PUERTO>.txt` generados por `flash_multi_esp32.sh`.
- Permisos: ejecutar Bash con permisos suficientes para acceder a COM/USB.

Buenas prácticas para fabricación
- Mantener los cables y hubs USB dedicados y probados.
- Usar un host con puertos estables y fuente de alimentación adecuada.
- Antes de lanzar un lote, probar 2-3 unidades en el proceso completo.

Contacto
- Si hay problemas recurrentes, contactar con el equipo de ingeniería con el log correspondiente.

-----
Archivo creado automáticamente para el equipo de fabricación. Si quiere que añada ejemplos adicionales o un script de verificación post-flash, indíquelo y lo añado.
