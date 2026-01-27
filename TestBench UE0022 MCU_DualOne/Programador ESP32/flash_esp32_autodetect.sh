#!/bin/bash

# --- CONFIGURACIÓN INICIAL ---
BAUD="921600"
FIRMWARE_PATH="blink_esp32.ino.merged.bin" 
ADDRESS="0x0"
PORT="" # Lo dejaremos vacío para que lo llene la autodeteción

# --- FUNCIÓN PARA DETECTAR PUERTO ---
detectar_puerto() {
    echo "Buscando ESP32 conectado..."
    # Usamos python para buscar en los puertos COM disponibles.
    # Buscamos los VIDs comunes:
    # 10C4 = CP210x (Silicon Labs, muy común en DevKits)
    # 1A86 = CH340 (Drivers chinos, muy común en clones)
    # 303A = Espressif (USB Nativo del ESP32-S2/S3/C3)
    
    PORT=$(python -c "import serial.tools.list_ports; print(next((p.device for p in serial.tools.list_ports.comports() if '10C4' in p.hwid or '1A86' in p.hwid or '303A' in p.hwid), ''))")

    if [ -z "$PORT" ]; then
        echo "⚠️  ADVERTENCIA: No se detectó ningún ESP32 automáticamente."
        echo "Por favor, escribe el puerto manualmente (ej. COM3):"
        read PORT
    else
        echo "✅ ESP32 detectado en: $PORT"
    fi
}

# Ejecutamos la detección al iniciar el script
detectar_puerto
sleep 1

# --- MENÚ ---
while true; do
    # clear # Descomenta si quieres limpiar pantalla en cada vuelta
    echo "========================================"
    echo "   HERRAMIENTA ESP32 (ESPTOOL WRAPPER)"
    echo "========================================"
    echo "Configuración actual:"
    if [ -z "$PORT" ]; then
        echo "Puerto: [NO DEFINIDO]" 
    else
        echo "Puerto: $PORT | Baudios: $BAUD"
    fi
    echo "Firmware: $FIRMWARE_PATH"
    echo "========================================"
    echo "1. Verificar conexión (Chip ID)"
    echo "2. Borrar Flash (Factory Reset)"
    echo "3. Grabar Firmware (Flash)"
    echo "4. Redetectar Puerto (Buscar de nuevo)"
    echo "5. Salir"
    echo "========================================"
    read -p "Selecciona una opción: " opcion

    case $opcion in
        1)
            echo "Consultando Chip ID..."
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
                echo "Grabando firmware en $PORT..."
                python -m esptool --port "$PORT" --baud "$BAUD" write_flash -z "$ADDRESS" "$FIRMWARE_PATH"
            else
                echo "ERROR: No se encuentra el archivo $FIRMWARE_PATH"
            fi
            read -p "Presiona Enter para continuar..."
            ;;
        4)
            detectar_puerto
            read -p "Presiona Enter para continuar..."
            ;;
        5)
            echo "Saliendo..."
            exit 0
            ;;
        *)
            echo "Opción no válida."
            ;;
    esac
done