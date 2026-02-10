# UE0089 - Cargador de Baterías (documentación)

## Resumen

Este README documenta el código principal de integración JSON utilizado para las pruebas de **UE0089 Cargador de Baterías** (archivo principal: `Códigos ARDUINO/MainCode Integración JSON/Integracion_JSON/Integracion_JSON.ino`) y la implementación de **MicroPython Carga Variable** (`MicroPython Carga Variable/main.py`).

El `MainCode Integración JSON` es el sketch que se vincula a la `PagWeb` (interfaz de pruebas) y realiza las rutinas de medición y control del cargador mediante comandos JSON por UART2. El MicroPython es el controlador de la carga variable que recibe JSON para configurar y ejecutar perfiles, y devuelve mediciones en JSON.

## MainCode Integración JSON (archivo principal)

Ruta: `Códigos ARDUINO/MainCode Integración JSON/Integracion_JSON/Integracion_JSON.ino`

### Propósito

- Ejecutar rutinas de prueba del cargador (cortocircuito, lectura nominal) controladas por la interfaz de pruebas.
- Medir corriente/voltaje con `INA219` y controlar relés para aplicar condiciones de prueba.
- Enviar los resultados a la `PagWeb` por UART2 en formato JSON.

### Pines y hardware (según sketch)

- UART2 (PagWeb): `RX2 = D4`, `TX2 = D5` (115200 bps)
- I2C (INA219): `SDA = GPIO6`, `SCL = GPIO7`
- Relevadores de cortocircuito: `RELAY1 = 14`, `RELAY2 = 0`
- Relevadores de fuente / conmutación: `RELAYA = 8`, `RELAYB = 9`

(Ajustar pines si su hardware difiere.)

### Comandos JSON aceptados

El sketch espera una línea con JSON terminada en `\n` por UART2. Campo principal: `Function`.

Soportados:
- `{"Function":"Cortocircuito"}` → Ejecuta prueba de cortocircuito:
  - Activa `RELAY1` y `RELAY2` (low activo), espera 1.2s, mide corriente, apaga relés.
  - Respuesta JSON enviada contiene `LecturaCorto` y opcionalmente `Result":"OK"` si cumple condición.

- `{"Function":"Lectura Nom"}` → Lectura nominal de corriente:
  - Mide corriente (función `medirValores()`), envía `{"Lectura":"X.XXX A","Result":"OK"}` si la lectura > 2.5 A.

Ejemplo de petición y respuesta (cortocircuito):

Petición:

```json
{"Function":"Cortocircuito"}
```

Respuesta (ejemplo):

```json
{"LecturaCorto":"0.523 A","Result":"OK"}
```

Ejemplo de petición y respuesta (lectura nominal):

Petición:

```json
{"Function":"Lectura Nom"}
```

Respuesta (ejemplo):

```json
{"Lectura":"3.142 A","Result":"OK"}
```

### Medición INA219

La función `medirValores()` lee `shunt_mV` y `bus_V` desde `ina219`, aplica offset y calcula corriente usando la resistencia shunt `R_SHUNT = 0.05` ohm. El valor retornado es la corriente en amperios.

### Consideraciones de integración

- El sketch envía JSON por `PagWeb` y termina la línea con `\n` (importante para que la interfaz delimite mensajes).
- Las rutinas de prueba son síncronas: la `PagWeb` debe esperar la respuesta antes de enviar otra petición.
- Logs y errores se imprimen por `Serial` USB (115200) para depuración.

## MicroPython - Carga Variable (controlador de carga)

Ruta: `MicroPython Carga Variable/main.py`

### Propósito

Controla una carga variable que puede configurarse por la `PagWeb` mediante JSON; además realiza lecturas del analizador de potencia (vía UART a un módulo de potencia) y devuelve los resultados en JSON. Este código es la versión que recibe JSON y también devuelve datos en JSON.

### Resumen de funcionalidades

- Recibe JSON por `UART2` con los campos `Funcion`, `Config`, `Start`.
- Soporta modos de configuración `LIST` y `DYN`, comandos de idioma `LANG`, `RST`, y control `Other` con `Start` (`CFG_ON`, `CFG_OFF`, `Read`).
- Envía respuestas JSON de confirmación y envía las mediciones (lista de muestras) cuando se solicitan.
- Implementa lectura concurrente: un hilo (`CORE2`) realiza lecturas del dispositivo de potencia vía `uart1`, otro hilo (`CORE1`) recibe comandos y envía respuestas.

### JSON de entrada soportado

Formato general (por línea, con `\n`):

- Configurar modo LIST:

```json
{"Funcion":"LIST","Config":{"Resolution":[10,0.2,0.5,"STEP1"],"Range":[50,3]},"Start":"CFG_ON"}
```

- Configurar modo DYN:

```json
{"Funcion":"DYN","Config":{"Resolution":[0.350,0.20,0.350,0.20],"Range":[50,3]},"Start":"CFG_ON"}
```

- Configurar idioma:

```json
{"Funcion":"LANG","Config":{"Resolution":[],"Range":[]},"Start":"CFG_ON"}
```

- Reset del sistema:

```json
{"Funcion":"RST","Config":{"Resolution":[],"Range":[]},"Start":"CFG_ON"}
```

- Control/activar/desactivar carga:

```json
{"Funcion":"Other","Config":{"Resolution":[],"Range":[]},"Start":"CFG_ON"}

{"Funcion":"Other","Config":{"Resolution":[],"Range":[]},"Start":"CFG_OFF"}
```

- Iniciar lectura de datos:

```json
{"Funcion":"Other","Config":{"Resolution":[],"Range":[]},"Start":"Read"}
```

### JSON de salida (respuestas)

- Respuesta de configuración:

```json
{"CONF":"DYN","response":true,"muestras":null}
```

- Respuesta de control (por ejemplo activar/desactivar):

```json
{"CONF":"CFG_ON","muestras":null}
```

- Datos de medición (cuando se solicita `Read`): devuelve `muestras` como lista de cadenas `"voltaje;corriente;potencia"`:

```json
{"CONF":null,"muestras":["3.45;1.23;4.25","3.46;1.24;4.28"],"response":true}
```

### Ejemplos de uso (flujo completo)

1. Configurar modo DYN:

```json
{"Funcion":"DYN","Config":{"Resolution":[0.350,5.20,0.150,1.20],"Range":[50,3]},"Start":"CFG_ON"}
```

2. Iniciar lectura de N muestras (previamente configurado):

```json
{"Funcion":"Other","Config":{},"Start":"Read"}
```

Respuesta (ejemplo):

```json
{"CONF":null,"muestras":["3.45;1.23;4.25","3.46;1.24;4.28"],"response":true}
```

3. Apagar carga:

```json
{"Funcion":"Other","Config":{},"Start":"CFG_OFF"}
```

### Notas técnicas

- `main.py` usa `uart1` para comunicarse con el hardware de carga (baud 9600) y `uart2` para comunicarse con la `PagWeb` (baud 9600). Asegúrese de configurar los pines apropiados si su hardware difiere.
- El código espera líneas JSON completas terminadas en `\n`. En UART2 existe una rutina `readSerialUART2()` que acumula bytes hasta recibir `\n`.
- `CORE2` realiza la lectura de muestras y rellena `muestras` que luego son enviadas por `CORE1`.
- `DEBUG` controla impresiones por consola; las respuestas JSON siempre se envían aunque `DEBUG` esté `False`.

## Recomendaciones y buenas prácticas

- Siempre terminar los JSON con `\n` y esperar la respuesta antes de enviar el siguiente comando.
- Si cambia el modo de comunicación o se presentan lecturas inconsistentes, reinicie la Pulsar o cierre y vuelva a abrir el monitor serie (especialmente importante cuando se cambia entre protocolos o se reinicia el periférico de potencia).
- Ajustar `StaticJsonDocument` en el sketch si se planea añadir más campos al JSON.

## Archivos relevantes

- `Códigos ARDUINO/MainCode Integración JSON/Integracion_JSON/Integracion_JSON.ino` — MainCode de integración JSON (UE0089)
- `MicroPython Carga Variable/main.py` — Controlador de carga variable (recepción y respuesta JSON)
- `MicroPython Carga Variable/CS Código Pulsar Recibiendo JSON UART.txt` — notas/ejemplos de uso

---

**Última actualización**: Febrero 2026
**Autor**: Documentación automática basada en el código fuente
