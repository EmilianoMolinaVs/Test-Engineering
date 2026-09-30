from machine import Pin, PWM
import i2c_slave


# ==================================================
# Barrido autonomo del 74HC4067 mediante PWM
# ==================================================

# Un ciclo completo recorre cuatro canales. S0 trabaja al doble de S1:
#   S1 S0 = 00, 01, 10, 11
# El PWM continua en hardware mientras Python atiende el esclavo I2C.
SCAN_FREQUENCY_HZ = 20000

S0_PIN = Pin(0, Pin.OUT, value=0)
S1_PIN = Pin(1, Pin.OUT, value=0)
S2 = Pin(3, Pin.OUT, value=0)
S3 = Pin(4, Pin.OUT, value=0)
EN = Pin(15, Pin.OUT, value=1)  # Activo en nivel bajo.


def crear_pwm_50(pin, frecuencia):
    pwm = PWM(pin)
    pwm.freq(frecuencia)

    # Compatibilidad entre firmwares que usan duty_u16 y duty (0-1023).
    if hasattr(pwm, "duty_u16"):
        pwm.duty_u16(32768)
    else:
        pwm.duty(512)

    return pwm


# S0 cambia dos veces por cada ciclo de S1.
pwm_s0 = crear_pwm_50(S0_PIN, SCAN_FREQUENCY_HZ * 2)
pwm_s1 = crear_pwm_50(S1_PIN, SCAN_FREQUENCY_HZ)


def seleccionar_bloque(bloque):
    # Se deshabilita el mux solamente durante el cambio de bloque.
    EN.value(1)

    if bloque == 1:       # Canales 0-3, reles 1-4
        S2.value(0)
        S3.value(0)
    elif bloque == 2:     # Canales 4-7, reles 5-8
        S2.value(1)
        S3.value(0)
    elif bloque == 3:     # Canales 8-11, reles 9-12
        S2.value(0)
        S3.value(1)
    else:                 # Canales 12-15, reles 13-16
        S2.value(1)
        S3.value(1)

    EN.value(0)


# ==================================================
# Direccion I2C mediante DIP switch
# ==================================================

DIP_PINS = (
    Pin(21, Pin.IN, Pin.PULL_DOWN),
    Pin(20, Pin.IN, Pin.PULL_DOWN),
    Pin(18, Pin.IN, Pin.PULL_DOWN),
)


def leer_direccion_i2c():
    valor = 0
    for indice, pin in enumerate(DIP_PINS):
        valor |= pin.value() << indice
    return 0x40 + valor


SLAVE_ADDR = leer_direccion_i2c()

# Se conserva el mismo orden de pines del ejemplo funcional main.py.
slave = i2c_slave.I2CSlave(0, SLAVE_ADDR, 23, 22)
slave.init()


# ==================================================
# Protocolo I2C de solo escritura
# ==================================================

#   0x01 -> bloque 1, reles 1-4
#   0x02 -> bloque 2, reles 5-8
#   0x03 -> bloque 3, reles 9-12
#   0x04 -> bloque 4, reles 13-16
#   0xFE -> deshabilitar el mux

CMD_APAGAR = 0xFE
active_block = 0


print("======================================")
print("Prueba I2C con barrido PWM de hardware")
print("Direccion esclavo: 0x{:02X}".format(SLAVE_ADDR))
print("Frecuencia de barrido: {} Hz".format(SCAN_FREQUENCY_HZ))
print("1 -> reles 1, 2, 3 y 4")
print("2 -> reles 5, 6, 7 y 8")
print("3 -> reles 9, 10, 11 y 12")
print("4 -> reles 13, 14, 15 y 16")
print("FE -> deshabilitar mux")
print("Estado inicial: todos los reles apagados")
print("======================================")


while True:
    # Puede esperar porque S0 y S1 siguen conmutando mediante PWM hardware.
    comando = slave.read_command(1000)

    if comando is None:
        continue

    comando &= 0xFF

    if 1 <= comando <= 4:
        active_block = comando
        seleccionar_bloque(active_block)
    elif comando == CMD_APAGAR:
        EN.value(1)
