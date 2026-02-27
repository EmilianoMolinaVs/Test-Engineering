#!/bin/bash

# ========= CONFIGURACIÓN =========
BAUD="921600"
FIRMWARE_PATH="blink_ESP32_VSV.ino.merged.bin"
ADDRESS="0x0"
FLASH_MODE="P"   # S = Secuencial | P = Paralelo
# ==================================

detectar_puertos() {
    PORTS_LIST=$(python -c "import serial.tools.list_ports; \
ports = [p.device for p in serial.tools.list_ports.comports() \
if '10C4' in p.hwid or '1A86' in p.hwid or '303A' in p.hwid]; \
print(' '.join(ports))")

    if [ -z "$PORTS_LIST" ]; then
        return 1
    else
        COUNT=$(echo $PORTS_LIST | wc -w)
        echo "✅ Detectados $COUNT dispositivo(s): $PORTS_LIST"
        return 0
    fi
}

programar_dispositivos() {

    if [ ! -f "$FIRMWARE_PATH" ]; then
        echo "❌ ERROR: No se encuentra $FIRMWARE_PATH"
        return
    fi

    MODE=${FLASH_MODE,,}

    if [ "$MODE" == "s" ]; then
        echo ">>> Modo SECUENCIAL"
        for PORT in $PORTS_LIST; do
            echo ">>> Grabando en $PORT..."
            python -m esptool --port "$PORT" --baud "$BAUD" write_flash -z "$ADDRESS" "$FIRMWARE_PATH"

            if [ $? -eq 0 ]; then
                echo "✅ $PORT OK"
            else
                echo "❌ $PORT ERROR"
            fi
        done

    elif [ "$MODE" == "p" ]; then
        echo ">>> Modo PARALELO"

        for PORT in $PORTS_LIST; do
            (
                python -m esptool --port "$PORT" --baud "$BAUD" \
                write_flash -z "$ADDRESS" "$FIRMWARE_PATH" \
                > "log_$PORT.txt" 2>&1

                if [ $? -eq 0 ]; then
                    echo "✅ $PORT OK"
                else
                    echo "❌ $PORT ERROR (ver log_$PORT.txt)"
                fi
            ) &
        done

        wait
        echo ">>> Todos terminaron."
    fi
}

# ========= LOOP PRODUCCIÓN =========

clear
echo "========================================"
echo "   MODO PRODUCCIÓN ESP32"
echo "   Presiona ENTER para programar"
echo "   Presiona q para salir"
echo "========================================"

while true; do

    echo ""
    echo "🔍 Escaneando dispositivos..."
    
    if detectar_puertos; then
        
        read -p "👉 ENTER = Programar | q = Salir : " opcion
        
        if [[ "$opcion" == "q" ]]; then
            echo "Saliendo..."
            exit 0
        fi

        echo "🚀 Iniciando programación..."
        programar_dispositivos
        echo "✅ Ciclo terminado."
        echo "----------------------------------------"

    else
        echo "⚠️ No se detectaron dispositivos."
        sleep 2
    fi

done