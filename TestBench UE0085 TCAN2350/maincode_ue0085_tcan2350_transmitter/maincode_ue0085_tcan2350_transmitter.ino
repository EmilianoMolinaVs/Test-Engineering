/*
 * ==============================================================================
 * FIRMWARE MAIN (ORQUESTADOR)
 * ==============================================================================
 * Dispositivo: ESP32-C6 / ESP32-H2
 * Función: Espera comandos JSON por puerto Serial, ejecuta rutinas de prueba 
 *          (como ráfagas CAN) y reporta los resultados de vuelta en formato JSON.
 * ==============================================================================
 */

#include <DevLab_TCAN1051HVD.h>
#include <ArduinoJson.h>

// ==== DECLARACIÓN DE PINES ====
#define CAN_TX_PIN GPIO_NUM_6
#define CAN_RX_PIN GPIO_NUM_7

DevLab_TCAN1051HVD can(CAN_TX_PIN, CAN_RX_PIN);

// ==== CREACIÓN DE OBJETOS JSON ====
StaticJsonDocument<128> receiveJSON;
StaticJsonDocument<128> sendJSON;

// ==== CONSTANTES GLOBALES ====
int contador = 0;
String dataInput = "";

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Intentamos iniciar el controlador CAN
  if (!can.begin()) {
    sendJSON.clear();
    sendJSON["state"] = "FAIL";
    sendJSON["module"] = "CAN";
    sendJSON["message"] = "Error al iniciar controlador CAN";
    serializeJson(sendJSON, Serial);
    Serial.println();

    while (true) delay(1000);  // Detenemos la ejecución si el hardware falla
  }

  // Log de inicio exitoso
  sendJSON.clear();
  sendJSON["state"] = "OK";
  sendJSON["module"] = "SYSTEM";
  sendJSON["message"] = "Dispositivo MAIN iniciado correctamente";
  serializeJson(sendJSON, Serial);
  Serial.println();
}

void loop() {
  TCAN1051_Frame frame;

  // ==== 1. PROCESAMIENTO DE COMANDOS SERIALES (JSON) ====
  if (Serial.available()) {
    String inJSON = Serial.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, inJSON);

    if (error) {
      // Log de error de sintaxis JSON recibida
      sendJSON.clear();
      sendJSON["state"] = "FAIL";
      sendJSON["error"] = error.c_str();
      sendJSON["message"] = "JSON invalido recibido";
      serializeJson(sendJSON, Serial);
      Serial.println();
    } else {
      // Leemos la función solicitada
      String Function = receiveJSON["Function"];
      int opc = 0;

      if (Function == "ping") opc = 1;          // {"Function":"ping"}
      else if (Function == "testCAN") opc = 2;  // {"Function":"testCAN"}

      switch (opc) {

        // --- CASO 1: COMANDO PING ---
        case 1:
          {
            sendJSON.clear();
            sendJSON["state"] = "OK";
            sendJSON["ping"] = "pong";
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }

        // --- CASO 2: PRUEBA DE ESTRÉS CAN (RÁFAGA) ---
        case 2:
          {
            sendJSON.clear();

            // Log de inicio de prueba
            sendJSON["state"] = "INFO";
            sendJSON["message"] = "Iniciando rafaga de 100 mensajes CAN";
            serializeJson(sendJSON, Serial);
            Serial.println();

            const char* text = "ping";
            uint8_t data[4] = { 0 };
            memcpy(data, text, 4);

            bool success = true;

            // Ciclo de envío de ráfaga
            for (int i = 0; i < 100; i++) {
              if (!can.send(0x100, data, 4, 0, 0, 1000)) {
                success = false;
                break;  // Rompemos si hay fallo físico en el envío
              }
              contador++;
              delay(20);
            }

            sendJSON.clear();
            if (success) {
              sendJSON["state"] = "OK";
              sendJSON["action"] = "TX_BURST";
              sendJSON["sent_count"] = contador;
            } else {
              sendJSON["state"] = "FAIL";
              sendJSON["action"] = "TX_BURST";
              sendJSON["message"] = "Fallo fisico al enviar mensaje CAN";
              sendJSON["sent_count"] = contador;
            }
            serializeJson(sendJSON, Serial);
            Serial.println();

            contador = 0;
            break;
          }

        // --- CASO DEFAULT: COMANDO DESCONOCIDO ---
        default:
          sendJSON.clear();
          sendJSON["state"] = "FAIL";
          sendJSON["error"] = "invalid option";
          serializeJson(sendJSON, Serial);
          Serial.println();
          break;
      }
    }
  }

  // ==== 2. LECTURA DE RESPUESTAS CAN DEL BUS ====
  if (can.receive(frame, 100)) {
    // Reconstruimos el string recibido
    for (int i = 0; i < frame.length; i++) {
      dataInput += (char)frame.data[i];
    }

    // Evaluamos si el esclavo confirmó el éxito de la prueba
    if (dataInput == "OK") {
      sendJSON.clear();
      sendJSON["Result"] = "OK";
      sendJSON["action"] = "RX_VALIDATION";
      sendJSON["message"] = "Validacion exitosa confirmada por Esclavo";
      serializeJson(sendJSON, Serial);
      Serial.println();
    }

    // Limpiamos el buffer para la siguiente lectura (¡Bug corregido!)
    dataInput = "";
  }
}