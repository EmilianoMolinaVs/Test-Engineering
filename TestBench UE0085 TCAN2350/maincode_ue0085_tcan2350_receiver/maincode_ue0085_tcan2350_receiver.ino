/*
 * ==============================================================================
 * FIRMWARE ESCLAVO (VALIDADOR)
 * ==============================================================================
 * Dispositivo: ESP32-C6 / ESP32-H2
 * Función: Escucha el bus CAN buscando comandos específicos. Al recibir 100 
 *          mensajes "ping", retorna un mensaje "OK" por el bus CAN para 
 *          confirmar el éxito de la prueba.
 * ==============================================================================
 */

#include <DevLab_TCAN1051HVD.h>
#include <ArduinoJson.h>

// ==== DECLARACIÓN DE PINES ====
#define CAN_TX_PIN GPIO_NUM_7
#define CAN_RX_PIN GPIO_NUM_6

DevLab_TCAN1051HVD can(CAN_TX_PIN, CAN_RX_PIN);

// ==== CREACIÓN DE OBJETOS JSON ====
StaticJsonDocument<128> logJSON; // Solo lo usamos para reportar su estado interno

// ==== CONSTANTES GLOBALES ====
String dataInput = "";
int countData = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  if (!can.begin()) {
    logJSON.clear();
    logJSON["state"] = "FAIL";
    logJSON["module"] = "CAN";
    logJSON["message"] = "Error al iniciar controlador CAN en Esclavo";
    serializeJson(logJSON, Serial);
    Serial.println();
    
    while (true) delay(1000); // Detenemos la ejecución
  }

  logJSON.clear();
  logJSON["state"] = "OK";
  logJSON["module"] = "SYSTEM";
  logJSON["message"] = "Dispositivo ESCLAVO iniciado y esperando tramas...";
  serializeJson(logJSON, Serial);
  Serial.println();
}

void loop() {
  TCAN1051_Frame frame;

  // ==== 1. ESCUCHA ACTIVA DEL BUS CAN ====
  if (can.receive(frame, 100)) {
    
    // Reconstruimos la cadena de texto recibida
    for (int i = 0; i < frame.length; i++) {
      dataInput += (char)frame.data[i];
    }

    // Validamos el contenido del mensaje
    if (dataInput == "ping") {
      countData++;
    }

    // ==== 2. EVALUACIÓN DE LA PRUEBA (100 MENSAJES) ====
    if (countData == 100) {
      
      // Log local informativo
      logJSON.clear();
      logJSON["state"] = "INFO";
      logJSON["action"] = "RX_BURST_COMPLETE";
      logJSON["message"] = "100 paquetes recibidos, enviando confirmacion";
      serializeJson(logJSON, Serial);
      Serial.println();

      countData = 0; // Reiniciamos el contador para la siguiente prueba

      // Preparamos la respuesta de validación para el maestro
      const char* text = "OK";
      uint8_t data[2] = { 0 };
      memcpy(data, text, 2);

      delay(1000); // Pequeña pausa antes de responder (opcional)
      
      // Enviamos el "OK" por el bus (¡DLC corregido a 2!)
      if (can.send(0x100, data, 2, 0, 0, 1000)) { 
        logJSON.clear();
        logJSON["state"] = "OK";
        logJSON["action"] = "TX_CONFIRMATION";
        logJSON["message"] = "Confirmacion enviada al Maestro";
        serializeJson(logJSON, Serial);
        Serial.println();
      } else {
        logJSON.clear();
        logJSON["state"] = "FAIL";
        logJSON["action"] = "TX_CONFIRMATION";
        logJSON["message"] = "Fallo al enviar confirmacion por el bus";
        serializeJson(logJSON, Serial);
        Serial.println();
      }
    }

    // Limpiamos el buffer para procesar el siguiente mensaje entrante
    dataInput = "";
  }
}