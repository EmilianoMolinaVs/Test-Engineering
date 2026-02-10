"""
TestBench ESP32C6 Firmware
Version: 1.0
Author: CesarBautista
Date: 2025-04-17
Usage: Run on the ESP32C6 board to test SD, OLED, Wi-Fi and UART functionalities.
"""

import machine
import time
import os
import json
from neopixel import NeoPixel
from ssd1306 import SSD1306_I2C
from sdcard import SDCard
from config import *
import ujson
import network
import select

try:
    import urequests as requests
except:
    import requests
    
np = NeoPixel(machine.Pin(NEO_PIN), 1)
pin_button = machine.Pin(9, machine.Pin.IN, machine.Pin.PULL_UP)

uart = machine.UART(1, baudrate=9600, tx=16, rx=17)

poll = select.poll()
poll.register(uart, select.POLLIN)

PINMAP = {
    "A0": 0, "D14": 0,
    "A1": 1, "D15": 1,
    "A2": 3, "D16": 3,
    "A3": 4, "D17": 4,
    "A4": 22, "D18": 22,
    "A5": 23, "D19": 23,
    "A6": 5, "D21": 5,
    "A7": 6, "D13": 6,
    "D0": 17, "D1": 16,
    "D2": 8,
    "D3": 9,
    "D4": 15, 
    "D5": 19,
    "D6": 20,
    "D7": 21,
    "D8": 12, 
    "D9": 13,
    "D10": 18, 
    "D11": 7,
    "D12": 2
}
output_pins = {}
gpio_state_web = {}
last_command = None

red = 0
green = 0
blue = 0
last_color = (0, 0, 0)
last_press = 0
press_count = 0
double_click_interval = 500

color_state = 0

hue = 0

def scanAndConnectWifi(targetSsid, password="", retries=10):
    wlan = network.WLAN(network.STA_IF)
    wlan.active(True)
    print("Escaneando redes disponibles...")
    networks = wlan.scan()
    mac = wlan.config('mac')
    mac = ':'.join(['{:02x}'.format(b) for b in mac])
    ssid_info = {net[0].decode(): net for net in networks}
    print("Redes encontradas:", list(ssid_info.keys()))
    if targetSsid not in ssid_info:
        print(f"Red '{targetSsid}' no encontrada.")
        return mac, False, None 
    authmode = ssid_info[targetSsid][4]
    print(f"AuthMode: {authmode} (0=OPEN, 3=WPA2)")
    print(f"Red '{targetSsid}' detectada. Conectando...")


    if authmode == 0:
        wlan.connect(targetSsid)
    else:
        wlan.connect(targetSsid, password)
    for i in range(retries):
        if wlan.isconnected():
            ip = wlan.ifconfig()[0]
            print("Conectado con IP:", ip)
            return mac, True, ip
        print(f"Intento de conexión ({i+1}/{retries})...")
        time.sleep(1)


    print("Fallo al conectar a la red.")
    return mac, False, None

def connectToESP32Server(serverIp, port, message):
    try:
        import socket
        addr = socket.getaddrinfo(serverIp, port)[0][-1]
        s = socket.socket()
        s.connect(addr)
        print(f"Conectado al ESP32 remoto en {serverIp}:{port}")
        s.send(message.encode())
        data = s.recv(1024)
        print("Respuesta recibida:", data.decode())
        s.close()
        return True
    except Exception as e:
        print("Error al conectar con el ESP32 remoto:", e)
        return False

def cycleColor():
    global hue, red, green, blue
    hue = (hue + 5) % 360
    red, green, blue = hsvToRgb(hue, 1, 1)
    return (red, green, blue)

def hsvToRgb(h, s, v):
    if s == 0.0:
        r = g = b = int(v * 100)
        return r, g, b
    h = h / 60.0
    i = int(h)
    f = h - i
    p = v * (1 - s)
    q = v * (1 - s * f)
    t = v * (1 - s * (1 - f))
    if i == 0:
        r, g, b = v, t, p
    elif i == 1:
        r, g, b = q, v, p
    elif i == 2:
        r, g, b = p, v, t
    elif i == 3:
        r, g, b = p, q, v
    elif i == 4:
        r, g, b = t, p, v
    else:
        r, g, b = v, p, q
    return int(r * 100), int(g * 100), int(b * 100)

def setColor(colorTuple):
    np[0] = colorTuple
    np.write()
    time.sleep(0.1)
    np[0] = (0, 0, 0)
    np.write()
    time.sleep(0.1)
    np[0] = colorTuple
    np.write()

def initSD():
    try:
        print("Inicializando SD...")
        spi = machine.SPI(1, baudrate=500000, polarity=0, phase=0,
                          sck=machine.Pin(SCK_PIN),
                          mosi=machine.Pin(MOSI_PIN),
                          miso=machine.Pin(MISO_PIN))
        sd = SDCard(spi, machine.Pin(CS_PIN))
        os.mount(sd, "/sd")
        return True
    except Exception as e:
        print("Error SD init:", e)
        return False

def testSDRW():
    try:
        print("Escribiendo y leyendo SD...")
        filename = "/sd/test.txt"
        with open(filename, "w") as f:
            f.write("SD OK\n")
        with open(filename, "r") as f:
            content = f.read()
        # with open("/sd/wifi_config.json") as f:
        #     config = ujson.load(f)
        ssid = ""
        password = ""
        os.umount("/sd")
        return True, filename, content, ssid, password
    except Exception as e:
        print("Error SD R/W:", e)
        return False, None, None, None, None

def initOLEDWithRetry(retries=3, delay=0.2):
    for attempt in range(retries):
        print(f"Intentando OLED (intento {attempt + 1})")
        setColor((255, 165, 0))
        try:
            try:
                machine.I2C(0).deinit()
                time.sleep(0.05)
            except:
                pass
            i2c = machine.I2C(0, sda=machine.Pin(QWIIC_SDA), scl=machine.Pin(QWIIC_SCL))
            oled = SSD1306_I2C(128, 64, i2c)
            return True, oled
        except Exception as e:
            print("Error OLED:", e)
            time.sleep(delay)
    return False, None

buffer = b''

def handleUartCommand(command):
    global last_command, output_pins, gpio_state_web
    d_pin_names = ["D13", "D12", "D11", "D10", "D7", "D6", "D5", "D4"]
    a_pin_names = ["A0", "A1", "A2", "A3", "A4", "A5"]
    d_pins = [machine.Pin(PINMAP[name], machine.Pin.OUT) for name in d_pin_names]
    a_pins = [machine.Pin(PINMAP[name], machine.Pin.OUT) for name in a_pin_names]
    print("Comando recibido:", command)
    def setPins(pins, state):
        for pin in pins:
            if state:
                pin.on()
            else:
                pin.off()
    try:
        if command == b'LED_ON':
            last_command = 'LED_ON'
            np[0] = (0, 255, 0)
            setPins(d_pins, True)
            setPins(a_pins, True)
            np.write()
            uart.write(b'OK\n')
        elif command == b'LED_OFF':
            last_command = 'LED_OFF'
            np[0] = (0, 0, 0)
            setPins(d_pins, False)
            setPins(a_pins, False)
            np.write()
            uart.write(b'OK\n')
        else:
            print("Comando no reconocido")
            uart.write(b'ERR\n')
    except Exception as e:
        print("Error procesando comando:", e)
        uart.write(b'ERR\n')

def checkUartCommand():
    global buffer
    if uart.any():
        buffer += uart.read()
        if b'\n' in buffer:
            lines = buffer.split(b'\n')
            for line in lines[:-1]:
                command = line.strip()
                handleUartCommand(command)
            buffer = lines[-1]


sd_ok = initSD()
if sd_ok:
    sd_rw_ok, filename, sd_content, ssid, password = testSDRW()
else:
    sd_rw_ok, filename, sd_content, ssid, password = False, None, None, None, None

try:
    machine.SPI(1).deinit()
    print("SPI desactivado.")
except:
    pass



mac, wifi_ok, ip = scanAndConnectWifi("TestBench2", "12345678")

if not wifi_ok:
    print("Wi-Fi no disponible. Abortando ejecución.")
    setColor((255, 0, 0))


oled_ok, oled = initOLEDWithRetry()

if oled_ok:
    print("Mostrando en OLED...")
    oled.fill(0)
    if sd_ok and sd_rw_ok:
        oled.text("SD + OLED OK", 0, 0)
        oled.text("Write OK", 0, 15)
        oled.text("Read: " + sd_content.strip(), 0, 30)
        oled.text("File: " + filename.split("/")[-1], 0, 50)
        setColor((0, 120, 0))
    elif not sd_ok:
        oled.text("OLED OK", 0, 0)
        oled.text("No SD detected", 0, 20)
        setColor((0, 0, 120))
    elif not sd_rw_ok:
        oled.text("OLED OK", 0, 0)
        oled.text("SD R/W Fail", 0, 20)
        setColor((120, 0, 120))
    oled.show()
elif sd_ok:
    print("OLED FAIL, pero SD OK")
    setColor((120, 120, 0))
else:
    print("OLED y SD fallaron")
    setColor((120, 0, 0))

result = {
    "mac": mac,
    "wifi_ok": wifi_ok,
    "ip": ip,
    "oled_ok": oled_ok,
    "sd_detected": sd_ok,
    "sd_rw": sd_rw_ok,
    "filename": filename.split("/")[-1] if filename else None,
    "content": sd_content.strip() if sd_content else None
}

print("Resultado del test:")
print(ujson.dumps(result))

time.sleep(1)

while True:
    checkUartCommand()
    if pin_button.value() == 0:
        now = time.ticks_ms()
        if time.ticks_diff(now, last_press) < double_click_interval:
            press_count += 1
        else:
            press_count = 1
        last_press = now
        if press_count == 2:
            np[0] = (0, 0, 0)
            np.write()
            last_color = (0, 0, 0)
        else:
            while pin_button.value() == 0:
                last_color = cycleColor()
                np[0] = last_color
                np.write()
                time.sleep(0.1)
        time.sleep(0.3)
    else:
        np[0] = last_color
        np.write()
    time.sleep(0.05)
