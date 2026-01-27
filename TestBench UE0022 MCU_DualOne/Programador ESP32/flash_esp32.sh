#!/bin/bash

# --- CONFIGURACIÓN ---
PORT="COM37"
BAUD="921600"
FIRMWARE_PATH="blink_esp32.ino.merged.bin" # Asegúrate que este nombre sea exacto
ADDRESS="0x0"

# --- MENÚ ---
while true; do
    echo "========================================"
    echo "   HERRAMIENTA ESP32 (ESPTOOL WRAPPER)"
    echo "========================================"
    echo "Configuración actual:"
    echo "Puerto: $PORT | Baudios: $BAUD"
    echo "Firmware: $FIRMWARE_PATH"
    echo "========================================"
    echo "1. Verificar conexión (Chip ID)"
    echo "2. Borrar Flash (Factory Reset)"
    echo "3. Grabar Firmware (Flash)"
    echo "4. Salir"
    echo "========================================"
    read -p "Selecciona una opción: " opcion

    case $opcion in
        1)
            echo "Consultando Chip ID..."
            # USAMOS python -m esptool
            python -m esptool --port "$PORT" chip_id
            read -p "Presiona Enter para continuar..."
            ;;
        2)
            echo "Borrando memoria flash..."
            python -m esptool --port "$PORT" erase_flash
            read -p "Presiona Enter para continuar..."
            ;;
        3)
            if [ -f "$FIRMWARE_PATH" ]; then
                echo "Grabando firmware..."
                # USAMOS python -m esptool Y COMILLAS EN LAS VARIABLES
                python -m esptool --port "$PORT" --baud "$BAUD" write_flash -z "$ADDRESS" "$FIRMWARE_PATH"
            else
                echo "ERROR: No se encuentra el archivo $FIRMWARE_PATH"
            fi
            read -p "Presiona Enter para continuar..."
            ;;
        4)
            echo "Saliendo..."
            exit 0
            ;;
        *)
            echo "Opción no válida."
            ;;
    esac
done