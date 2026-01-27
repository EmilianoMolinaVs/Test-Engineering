import subprocess
from serial.tools import list_ports

def find_esp():
    for p in list_ports.comports():
        if p.vid == 0x1A86 and p.pid == 0x7523:
            return p.device
    return None

port = find_esp()

if not port:
    raise Exception("ESP32 no encontrado")

subprocess.run([
    r"C:\Python313\python.exe",
    "-m", "esptool",
    "--chip", "esp32",
    "--port", port,
    "--baud", "921600",
    "write_flash", "-z",
    "0x1000", "firmware.bin"
])

