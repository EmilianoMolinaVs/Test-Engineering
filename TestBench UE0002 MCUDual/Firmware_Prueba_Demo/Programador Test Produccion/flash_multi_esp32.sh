#!/bin/bash

# --- CONFIGURACIÓN ---
BAUD="921600"
FIRMWARE_PATH="blink_DualMCU_ESP32.ino.merged.bin"
ADDRESS="0x0"

# CONFIGURACIÓN DEL MODO DE GRABACIÓN
# "S" = Secuencial (Uno por uno, ves el progreso en vivo)
# "P" = Paralelo (Todos a la vez, genera archivos de log)
FLASH_MODE="P"

PORTS_LIST="" 

# --- FUNCIÓN PARA DETECTAR MÚLTIPLES PUERTOS ---
detectar_puertos() {
    echo "Buscando múltiples ESP32 conectados..."
    PORTS_LIST=$(python -c "import serial.tools.list_ports; ports = [p.device for p in serial.tools.list_ports.comports() if '10C4' in p.hwid or '1A86' in p.hwid or '303A' in p.hwid]; print(' '.join(ports))")

    if [ -z "$PORTS_LIST" ]; then
        echo "⚠️  ADVERTENCIA: No se detectaron placas automáticamente."
    else
        COUNT=$(echo $PORTS_LIST | wc -w)
        echo "✅ Se detectaron $COUNT dispositivos: $PORTS_LIST"
    fi
}

# Detectar al inicio
detectar_puertos
sleep 1

# --- MENÚ PRINCIPAL ---
while true; do
    echo "========================================"
    echo "   HERRAMIENTA ESP32 MULTI-PUERTO"
    echo "========================================"
    echo "Firmware: $FIRMWARE_PATH"
    
    # Mostramos el modo configurado
    if [ "${FLASH_MODE,,}" == "p" ]; then
        echo "Modo Grabación: PARALELO (Rápido)"
    else
        echo "Modo Grabación: SECUENCIAL (Detallado)"
    fi

    if [ -z "$PORTS_LIST" ]; then
        echo "Placas detectadas: NINGUNA"
    else
        echo "Placas detectadas: $PORTS_LIST"
    fi
    echo "========================================"
    echo "1. Verificar conexión (Chip ID) en TODOS"
    echo "2. Borrar Flash en TODOS"
    echo "3. Grabar Firmware (Usando modo $FLASH_MODE)"
    echo "4. Redetectar Puertos"
    echo "5. Salir"
    echo "========================================"
    read -p "Selecciona una opción: " opcion

    case $opcion in
        1)
            for PORT in $PORTS_LIST; do
                echo "----------------------------------------"
                echo ">>> Consultando Chip ID en $PORT..."
                python -m esptool --port "$PORT" chip_id
            done
            read -p "Presiona Enter para continuar..."
            ;;
        2)
            for PORT in $PORTS_LIST; do
                echo "----------------------------------------"
                echo ">>> Borrando memoria flash en $PORT..."
                python -m esptool --port "$PORT" erase_flash
            done
            read -p "Presiona Enter para continuar..."
            ;;
        3)
            # VERIFICAR SI EXISTE EL ARCHIVO
            if [ ! -f "$FIRMWARE_PATH" ]; then
                echo "❌ ERROR: No se encuentra el archivo $FIRMWARE_PATH"
                read -p "Presiona Enter para continuar..."
                continue
            fi

            # Normalizamos la variable a minúscula (s/p)
            MODE=${FLASH_MODE,,}

            if [ "$MODE" == "s" ]; then
                # === MODO SECUENCIAL ===
                echo ">>> Iniciando grabación SECUENCIAL..."
                for PORT in $PORTS_LIST; do
                    echo "----------------------------------------"
                    echo ">>> GRABANDO EN $PORT..."
                    python -m esptool --port "$PORT" --baud "$BAUD" write_flash -z "$ADDRESS" "$FIRMWARE_PATH"
                    if [ $? -eq 0 ]; then
                        echo ">>> ✅ ÉXITO en $PORT"
                    else
                        echo ">>> ❌ ERROR en $PORT"
                    fi
                done

            elif [ "$MODE" == "p" ]; then
                # === MODO PARALELO ===
                echo ">>> Iniciando grabación PARALELA..."
                echo ">>> Los logs se guardarán en archivos log_COMx.txt"
                
                for PORT in $PORTS_LIST; do
                    (
                        echo ">>> Iniciando $PORT..."
                        python -m esptool --port "$PORT" --baud "$BAUD" write_flash -z "$ADDRESS" "$FIRMWARE_PATH" > "log_$PORT.txt" 2>&1
                        
                        if [ $? -eq 0 ]; then
                            echo ">>> ✅ $PORT Terminado OK"
                        else
                            echo ">>> ❌ $PORT Falló (Revisa log_$PORT.txt)"
                        fi
                    ) & 
                done
                
                wait
                echo ">>> Todos los procesos han terminado."

            else
                echo "❌ ERROR: La variable FLASH_MODE no es válida (Debe ser 'S' o 'P')."
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