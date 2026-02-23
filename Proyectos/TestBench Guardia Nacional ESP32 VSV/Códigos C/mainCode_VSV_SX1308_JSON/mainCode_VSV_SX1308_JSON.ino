/**
 * ============================================================================
 * TESTBENCH GUARDIA NACIONAL - ESP32 VSV (SX1308)
 * Código Final Integrado entre PagWeb y Pulsar ESP32-C6
 * 
 * Descripción:
 *   Este código controla un sistema de prueba con comunicación UART JSON
 *   entre una interfaz web (PagWeb) y el ESP32. Maneja:
 *   - Comunicación bidireccional vía JSON
 *   - Control de relés para fuente de alimentación
 *   - Botón de arranque manual
 * ============================================================================
 */

// --- LIBRERÍAS NECESARIAS ---
#include <HardwareSerial.h>  // UART por hardware
#include <ArduinoJson.h>     // Procesamiento de JSON

// --- DEFINICIÓN DE PINES ---
#define RX2 D4        // Pin RX para UART2 (GPIO15)
#define TX2 D5        // Pin TX para UART2 (GPIO19)
#define RELAYA 8      // Relé A - Control de fuente de alimentación
#define RELAYB 9      // Relé B - Control de fuente de alimentación
#define RUN_BUTTON 4  // Botón de arranque manual

// --- OBJETOS DE COMUNICACIÓN ---
/** @brief Objeto UART para comunicación con la página web */
HardwareSerial PagWeb(1);  // UART2 configurado como PagWeb

// --- VARIABLES GLOBALES - JSON ---
/** @brief String que recibe los datos JSON crudos desde PagWeb */
String JSON_entrada;

/** @brief Documento JSON para deserializar datos de entrada (capacidad: 200 bytes) */
StaticJsonDocument<200> receiveJSON;

/** @brief String que envía los datos JSON hacia PagWeb */
String JSON_salida;

/** @brief Documento JSON para serializar datos de salida (capacidad: 200 bytes) */
StaticJsonDocument<200> sendJSON;

/**
 * @brief Inicialización del sistema
 * 
 * Configura:
 *   - Puertos seriales (115200 baud)
 *   - Pines de relés como salida
 *   - Botón de arranque como entrada
 *   - Estado inicial de relés (apagados)
 */
void setup() {
  // Inicializar puerto serial principal (depuración)
  Serial.begin(115200);

  // Inicializar UART2 para comunicación con PagWeb
  PagWeb.begin(115200, SERIAL_8N1, RX2, TX2);
  Serial.println("Inicialización serial...");

  // Configurar pines como salida (relés)
  pinMode(RELAYA, OUTPUT);
  pinMode(RELAYB, OUTPUT);

  // Configurar pin como entrada (botón)
  pinMode(RUN_BUTTON, INPUT);

  // Estado inicial: todos los relés apagados (LOW = relés desactivados)
  digitalWrite(RELAYA, LOW);
  digitalWrite(RELAYB, LOW);
}



/**
 * @brief Bucle principal de ejecución
 * 
 * Funcionalidades:
 *   1. Detecta presión del botón de arranque y envía confirmación JSON
 *   2. Lee y procesa comandos JSON desde PagWeb
 *   3. Ejecuta acciones de control según función JSON recibida
 */
void loop() {

  // ============================================================================
  // SECCIÓN 1: DETECCIÓN DE BOTÓN DE ARRANQUE
  // ============================================================================

  // Detectar si el botón está presionado (lectura inicial)
  if (digitalRead(RUN_BUTTON) == HIGH) {
    delay(200);        // Anti-rebote: esperar 200ms para estabilizar
    sendJSON.clear();  // Limpiar documento JSON previo

    // Verificar nuevamente el estado del botón (double-check para anti-rebote)
    if (digitalRead(RUN_BUTTON) == LOW) {
      Serial.println("Arranque por botonera");

      // Crear respuesta JSON de confirmación
      sendJSON["Run"] = "OK";  // Indicar que se recibió comando de arranque

      // Enviar JSON serializado a la página web
      serializeJson(sendJSON, PagWeb);
      PagWeb.println();  // Agregar salto de línea como delimitador
    }
  }

  // ============================================================================
  // SECCIÓN 2: RECEPCIÓN Y PROCESAMIENTO DE COMANDOS JSON DESDE PAGWEB
  // ============================================================================

  if (PagWeb.available()) {
    // Leer STRING completo hasta encontrar salto de línea ('\n')
    JSON_entrada = PagWeb.readStringUntil('\n');

    // Deserializar el JSON recibido con detección de errores
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);

    // Validar que no haya errores de parseo JSON
    if (!error) {
      // Extraer el campo "Function" del JSON (clave de ejecución)
      String Function = receiveJSON["Function"];

      // Variable para seleccionar acción mediante switch
      int opc = 0;

      // ====================================================================
      // SECCIÓN 3: MAPEO DE FUNCIONES JSON A OPCIONES
      // ====================================================================

      // Mapear funciones JSON a valores numéricos para switch
      if (Function == "PowerOn") {
        opc = 1;  // Activar fuente de alimentación
      } else if (Function == "PowerOff") {
        opc = 2;  // Desactivar fuente de alimentación
      }

      // ====================================================================
      // SECCIÓN 4: EJECUCIÓN DE COMANDOS POR SWITCH
      // ====================================================================

      switch (opc) {

        // Caso 1: Encender fuente de alimentación (PowerOn)
        case 1:
          digitalWrite(RELAYA, LOW);  // Activar relé A
          digitalWrite(RELAYB, LOW);  // Activar relé B
          Serial.println("Fuente de alimentación: ENCENDIDA");
          break;

        // Caso 2: Apagar fuente de alimentación (PowerOff)
        case 2:
          digitalWrite(RELAYA, HIGH);  // Desactivar relé A
          digitalWrite(RELAYB, HIGH);  // Desactivar relé B
          Serial.println("Fuente de alimentación: APAGADA");
          break;

        // Caso por defecto: comando no reconocido
        default:
          Serial.println("Comando no reconocido");
          break;
      }

    } else {
      // Manejar error de deserialización JSON
      Serial.print("Error de JSON: ");
      Serial.println(error.c_str());
    }

  } else {
    // No hay datos disponibles en el puerto serial (falso contacto/ espera)
    // Serial.println("Esperando datos...");  // Descomentar si se desea depuración
  }
}
