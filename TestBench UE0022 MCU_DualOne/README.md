# TestBench UE0022 - DualOne MCU

Este repositorio contiene código y utilidades para programar y probar las placas ESP32 y RP2040 usadas en el proyecto "DualOne".

Este README documenta en particular los scripts Bash dentro de las carpetas `Programador ESP32` y `Programador RP2040`: qué hacen, qué implican y cómo usarlos en Windows (Git Bash / WSL) o Linux.

**Prerequisitos**:
- Tener un entorno Bash disponible en Windows (Git Bash o WSL) o usar Linux/macOS.
- Python 3 instalado y los paquetes `pyserial` y `esptool` (solo para ESP32).
	- Instalar con: `pip install pyserial esptool`
- Para RP2040: usar Git Bash o WSL para que las unidades Windows aparezcan como `/d`, `/e`, etc., o ajustar los scripts si se trabaja desde PowerShell.
- Permisos: los scripts son ejecutables; si da error, ejecutar `chmod +x` sobre ellos.

**Avisos de seguridad y uso**:
- Al grabar se sobrescribe la memoria de flash; haga copias de seguridad de datos importantes.
- Asegúrese de seleccionar el puerto correcto antes de flashear (los scripts ofrecen detección automática o puerto fijo según el script).
- No desconectar la alimentación durante el flasheo.

----------------------------------------------------------------

Sección: `Programador ESP32`

Archivos principales y propósito:
- `flash_esp32_autodetect.sh`: Herramienta interactiva que detecta automáticamente el puerto serie del ESP32 (buscando VIDs comunes: CP210x, CH340, Espressif). Ofrece opciones para consultar chip ID, borrar flash y grabar firmware mediante `esptool`.
- `flash_esp32.sh`: Versión similar pero con `PORT` preconfigurado (ej. `COM37`). Útil cuando ya se conoce el puerto.
- `flash_multi_esp32.sh`: Diseñado para programar múltiples placas conectadas a la vez. Soporta modo secuencial (S) o paralelo (P). Genera logs por puerto en modo paralelo.

Cómo usar (ejemplos):
- Abre Git Bash o WSL en la carpeta `Programador ESP32`.
- Hacer ejecutable (si hace falta):

```bash
chmod +x flash_esp32_autodetect.sh flash_esp32.sh flash_multi_esp32.sh
```

- Ejecutar (autodetección):

```bash
./flash_esp32_autodetect.sh
```

- Ejecutar (puerto fijo):

```bash
./flash_esp32.sh
```

- Grabar múltiples placas (modo paralelo por defecto):

```bash
./flash_multi_esp32.sh
```

Notas técnicas e implicaciones (ESP32):
- Los scripts llaman a `python -m esptool` para realizar operaciones de `chip_id`, `erase_flash` y `write_flash`.
- Asegúrese de que el archivo de firmware (`*.bin`) exista en la carpeta; los nombres esperados están en la variable `FIRMWARE_PATH` dentro del script (p. ej. `DualOneESP32.ino.merged.bin`).
- `flash_multi_esp32.sh` detecta puertos por HWID y lanza procesos en paralelo, generando archivos `log_<PUERTO>.txt` cuando está en modo paralelo.

----------------------------------------------------------------

Sección: `Programador RP2040`

Archivos principales y propósito:
- `auto_flash_rp2040.sh`: Automatiza el proceso de reinicio (envía 1200 baud a los puertos detectados) y espera que las unidades RP2040 aparezcan como discos USB (RPI-RP2). Copia el `.uf2` a cada unidad detectada.
- `flash_rp2040.sh`: Herramienta interactiva para detectar una unidad RP2040 en modo bootloader (busca `INFO_UF2.TXT`) y copiar un único archivo `.uf2` al dispositivo.
- `flash_multi_rp2040.sh`: Detecta múltiples unidades RPI-RP2 y copia en paralelo un archivo `.uf2` a todas.

Cómo usar (ejemplos):
- Abre Git Bash o WSL en la carpeta `Programador RP2040`.
- Hacer ejecutable (si hace falta):

```bash
chmod +x auto_flash_rp2040.sh flash_rp2040.sh flash_multi_rp2040.sh
```

- Ejecutar ciclo completo (envía 1200 baud para reiniciar a bootloader y copia `.uf2`):

```bash
./auto_flash_rp2040.sh
```

- Flasheo manual (si ya puso los RP2040 en BOOTSEL):

```bash
./flash_rp2040.sh
```

- Flashear múltiples unidades detectadas:

```bash
./flash_multi_rp2040.sh
```

Notas técnicas e implicaciones (RP2040):
- Los scripts esperan que, al entrar en bootloader, cada RP2040 monte una unidad con `INFO_UF2.TXT`. En Git Bash/WSL esas unidades suelen estar accesibles como `/d`, `/e`, etc.
- `auto_flash_rp2040.sh` envía un pulso a 1200 baud para forzar el reinicio al bootloader en los puertos detectados; esto depende de que el micro implemente la secuencia USB-serial a bootloader.
- El flasheo se realiza mediante una simple copia del `.uf2` al volumen RPI-RP2. Si la copia falla, el script informa el error.

----------------------------------------------------------------

Sección final: Solución de problemas y recomendaciones
- Si `flash_esp32_autodetect.sh` no detecta puertos, comprueba drivers (CP210x/CH340) y que el cable soporte datos.
- Si los scripts de RP2040 no detectan unidades, prueba a conectar manteniendo presionado `BOOTSEL` o revisa que el cable soporte datos.
- Para depuración: abrir los `log_*.txt` generados por `flash_multi_esp32.sh`.
- Si trabajas desde PowerShell, los comportamientos de montaje y comandos (`cp`, `/d`) pueden diferir; se recomienda Git Bash o WSL para ejecutar estos scripts sin cambios.

- ¿Quieres que aplique este README al archivo `README.md` del proyecto ahora y que cree README individuales dentro de `Programador ESP32` y `Programador RP2040`? Indica si prefieres texto más técnico, más breve o incluir ejemplos concretos de comandos para Windows (PowerShell) también.

