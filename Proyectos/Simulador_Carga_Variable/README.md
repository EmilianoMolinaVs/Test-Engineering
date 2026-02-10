# Simulador de Carga Variable

## Descripción

Firmware para la **Pulsar C6** que simula el comportamiento de una carga variable, devolviendo valores de voltaje, corriente y potencia en formato JSON. Este simulador está diseñado para ser utilizado en sincronización con las aplicaciones web (PagWeb) del TestBench.

## Características

- **Comunicación Serial**: Opera a través de puerto serial a 115200 baudios
- **Comando JSON**: Recibe comandos estructurados en JSON
- **Valores Simulados**: Genera lecturas de:
  - Voltaje (V)
  - Corriente (A)
  - Potencia (W)
- **Formato de Salida**: JSON con valores en formato de cadena separados por punto y coma
- **Sincronización Web**: Compatible con las interfaces web para visualización en tiempo real

## Hardware Requerido

- **MCU**: Pulsar C6 (ESP32-C6)
- **Interfaz**: Puerto Serial USB

## Dependencias

- **ArduinoJson**: Librería para manejo de JSON en Arduino

```bash
# Instalación en Arduino IDE
Sketch → Include Library → Manage Libraries → Buscar "ArduinoJson" → Instalar
```

## Protocolo de Comunicación

### Comando de Entrada (JSON)

```json
{
    "Funcion": "Other", 
    "Config": {
        "Resolution": [], 
        "Range": []}, 
        "Start": "Read"
}
```

### Respuesta (JSON)

```json
{
  "medicion": "5.00;2.00;10.00"
}
```

Donde el formato de `medicion` es: `voltaje;corriente;potencia`

## Cómo Usar

1. **Cargar el Firmware**: Subir el código a la Pulsar C6 usando Arduino IDE
2. **Conectar la MCU**: Conectar la Pulsar C6 al computador vía USB
3. **Enviar Comandos**: Desde la aplicación web o terminal serial, enviar:
   ```
   {"Funcion": "Other", "Config": {"Resolution": [], "Range": []}, "Start": "Read"}
   ```
4. **Recibir Datos**: El firmware responderá con los valores simulados de carga

## Valores Actuales (Simulados)

- **Voltaje**: 5.00 V
- **Corriente**: 2.00 A
- **Potencia**: 10.00 W

> Nota: Los valores están hardcodeados en el firmware. Para cambiar los valores simulados, modificar las variables en la sección de comentario `// ===== Lecturas simuladas =====`

## Integración con PagWeb

Este simulador se comunica con las páginas web operador para mostrar en tiempo real:
- Gráficos de consumo de carga
- Valores instantáneos de voltaje y corriente
- Cálculos de potencia

## Estructura del Código

| Componente | Función |
|-----------|----------|
| `setup()` | Inicializa puerto serial a 115200 baudios |
| `loop()` | Lee comandos JSON, valida y ejecuta |
| Simulación | Genera valores ficticios de carga |
| Serialización | Devuelve respuesta en formato JSON |

## Troubleshooting

| Problema | Solución |
|----------|----------|
| "JSON inválido" | Verificar que el JSON sea válido y esté completo |
| No hay respuesta | Confirmar conexión USB y velocidad de puerto (115200) |
| Valores no cambian | Los valores simulados son fijos; modificar el código para randomizar |

## Notas de Desarrollo

- Actualmente los valores son estáticos. Para un verdadero simulador, considerar:
  - Usar `random()` para variabilidad
  - Simular carga variable con patrones
  - Incluir opciones para cambiar valores vía JSON
  
## Licencia

Proyecto internal - TestBench UE Engineering

---

**Última actualización**: Febrero 2026
