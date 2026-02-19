import serial
import matplotlib.pyplot as plt

ser = serial.Serial('COM9', 115200)

frecuencias = []
ganancias = []

plt.ion()
fig, ax = plt.subplots()

while True:
    linea = ser.readline().decode().strip()
    f, g = linea.split(",")

    frecuencias.append(float(f))
    ganancias.append(float(g))

    ax.clear()
    ax.set_ylim(0, 5)      # eje fijo ganancia
    ax.set_xlim(0, 5000)   # eje fijo frecuencia

    ax.plot(frecuencias, ganancias)
    plt.pause(0.01)
