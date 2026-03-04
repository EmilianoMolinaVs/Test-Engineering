#!/bin/bash

# ========= CONFIGURACIÓN =========
BAUD="921600"
FIRMWARE_PATH="blink_TouchDot.ino.merged.bin"
ADDRESS="0x0"
FLASH_MODE="P"
# ==================================

GREEN='\033[1;32m'
RED='\033[1;31m'
YELLOW='\033[1;33m'
CYAN='\033[1;36m'
NC='\033[0m'

# -------- SPINNER ANIMADO --------
spinner() {
    local pid=$1
    local delay=0.1
    local spinstr='|/-\'
    while ps -p $pid > /dev/null 2>&1; do
        local temp=${spinstr#?}
        printf " ${YELLOW}Flasheando... [%c]${NC}  " "$spinstr"
        spinstr=$temp${spinstr%"$temp"}
        sleep $delay
        printf "\r"
    done
    printf "\r"
}

# -------- TEXTO GRANDE --------

detectar_puertos() {
    PORTS_LIST=$(python -c "import serial.tools.list_ports; \
ports = [p.device for p in serial.tools.list_ports.comports() \
if '10C4' in p.hwid or '1A86' in p.hwid or '303A' in p.hwid]; \
print(' '.join(ports))")

    if [ -z "$PORTS_LIST" ]; then
        return 1
    else
        COUNT=$(echo $PORTS_LIST | wc -w)
        echo -e "${GREEN}✔ $COUNT dispositivo(s) detectado(s): $PORTS_LIST${NC}"
        return 0
    fi
}

programar_dispositivos() {

    OK_COUNT=0
    FAIL_COUNT=0

    MODE=${FLASH_MODE,,}

    for PORT in $PORTS_LIST; do
        (
            python -m esptool --port "$PORT" --baud "$BAUD" \
            write_flash -z "$ADDRESS" "$FIRMWARE_PATH" \
            > "log_$PORT.txt" 2>&1
        ) &
        PID=$!
        spinner $PID
        wait $PID

        if [ $? -eq 0 ]; then
            echo -e "${GREEN}✔ $PORT PROGRAMADO OK${NC}"
            ((OK_COUNT++))
        else
            echo -e "${RED}✖ $PORT ERROR${NC}"
            ((FAIL_COUNT++))
        fi
    done

    echo ""
    echo -e "${CYAN}========================================${NC}"
    echo -e "${GREEN}   OK: $OK_COUNT${NC}"
    echo -e "${RED}   ERROR: $FAIL_COUNT${NC}"
    echo -e "${CYAN}========================================${NC}"
}

# ========= LOOP PRODUCCIÓN =========

while true; do

    echo -e "${CYAN}MODO PRODUCCIÓN AUTOMÁTICO${NC}"
    echo ""
    echo "Presiona ENTER para programar"
    echo "Presiona q para salir"
    echo ""

    echo -e "${YELLOW}Escaneando dispositivos...${NC}"

    if detectar_puertos; then

        read -p "👉 ENTER = Programar | q = Salir : " opcion

        if [[ "$opcion" == "q" ]]; then
            clear
            echo "Saliendo..."
            exit 0
        fi

        echo ""
        echo -e "${YELLOW}Iniciando programación...${NC}"
        echo ""

        programar_dispositivos

        echo ""
        read -p "Presiona ENTER para nuevo ciclo..."

    else
        echo -e "${RED}⚠ No se detectaron dispositivos.${NC}"
        sleep 2
    fi

done