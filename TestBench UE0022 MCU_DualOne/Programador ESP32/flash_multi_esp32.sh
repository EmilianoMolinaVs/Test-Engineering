#!/bin/bash

# --- CONFIGURACIÓN ---
BAUD="921600"
FIRMWARE_PATH="blink_esp32.ino.merged.bin"
ADDRESS="0x0"
PORTS_LIST="" # Aquí guardaremos la lista (ej: "COM3 COM4 COM8")

# --- FUNCIÓN PARA DETECTAR MÚLTIPLES PUERTOS ---
detectar_puertos() {
    echo "Buscando múltiples ESP32 conectados..."
    
    # El comando de Python ahora usa una lista de comprensión [...] y ' '.join()
    # para devolver todos los puertos separados por espacio.
    PORTS_LIST=$(python -c "import serial.tools.list_ports; ports = [p.device for p in serial.tools.list_ports.comports() if '10C4' in p.hwid or '1A86' in p.hwid or '303A' in p.hwid]; print(' '.join(ports))")

    if [ -z "$PORTS_LIST" ]; then
        echo "⚠️  ADVERTENCIA: No se detectaron placas automáticamente."
    else
        # Contamos cuántos puertos hay (separados por espacio)
        COUNT=$(echo $PORTS_LIST | wc -w)
        echo "✅ Se detectaron $COUNT dispositivos: $PORTS_LIST"
    fi
}

# Detectar al inicio
detectar_puertos
sleep 1

# --- MENÚ ---
while true; do
    echo "========================================"
    echo "   HERRAMIENTA ESP32 MULTI-PUERTO"
    echo "========================================"
    echo "Firmware: $FIRMWARE_PATH"
    if [ -z "$PORTS_LIST" ]; then
        echo "Placas detectadas: NINGUNA"
    else
        echo "Placas detectadas: $PORTS_LIST"
    fi
    echo "========================================"
    echo "1. Verificar conexión (Chip ID) en TODOS"
    echo "2. Borrar Flash en TODOS"
    echo "3. Grabar Firmware en TODOS"
    echo "4. Redetectar Puertos"
    echo "5. Salir"
    echo "========================================"
    read -p "Selecciona una opción: " opcion

    case $opcion in
        1)
            # BUCLE PARA RECORRER TODOS LOS PUERTOS
            for PORT in $PORTS_LIST; do
                echo "----------------------------------------"
                echo ">>> Consultando Chip ID en $PORT..."
                python -m esptool --port "$PORT" chip_id
            done
            echo "----------------------------------------"
            read -p "Presiona Enter para continuar..."
            ;;
        2)
            for PORT in $PORTS_LIST; do
                echo "----------------------------------------"
                echo ">>> Borrando memoria flash en $PORT..."
                python -m esptool --port "$PORT" erase_flash
            done
            echo "----------------------------------------"
            read -p "Presiona Enter para continuar..."
            ;;
        3)
            if [ -f "$FIRMWARE_PATH" ]; then
                for PORT in $PORTS_LIST; do
                    echo "----------------------------------------"
                    echo ">>> GRABANDO EN $PORT (Inicio)..."
                    python -m esptool --port "$PORT" --baud "$BAUD" write_flash -z "$ADDRESS" "$FIRMWARE_PATH"
                    
                    # Verificamos si el comando anterior fue exitoso
                    if [ $? -eq 0 ]; then
                        echo ">>> ✅ ÉXITO en $PORT"
                    else
                        echo ">>> ❌ ERROR en $PORT"
                    fi
                done
            else
                echo "ERROR: No se encuentra el archivo $FIRMWARE_PATH"
            fi
            read -p "Presiona Enter para continuar..."
            ;;
        4)
            detectar_puertos
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