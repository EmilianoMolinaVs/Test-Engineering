#include <Arduino.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>

// Pines para la IT8511A (Serial 1)
#define RX1 4 
#define TX1 5 

// Objetos Serial
HardwareSerial ITECH(1); 

// Variables de Control
bool flagFinish = false;
bool responseStatus = false;
int numMues = 10;
String lastConf = "None";

// Tareas de FreeRTOS (Cores)
TaskHandle_t TaskCore1;
TaskHandle_t TaskCore2;

// --- FUNCIONES DE COMANDO SCPI ---

void configLoad(String funcion, JsonArray resolution, JsonArray range) {
  ITECH.println("SYST:REM");
  delay(50);

  if (funcion == "LIST") {
    int steps = resolution[0];
    float currStep = resolution[1];
    float timeStep = resolution[2];

    ITECH.println("FUNC LIST");
    ITECH.print("CURR:RANG "); ITECH.println(range[0].as<float>());
    ITECH.print("LIST:STEP "); ITECH.println(steps);
    
    for (int i = 0; i < steps; i++) {
      float currentVal = (i + 1) * currStep;
      ITECH.print("LIST:CURR "); ITECH.print(i + 1); ITECH.print(", "); ITECH.println(currentVal, 3);
      ITECH.print("LIST:WID "); ITECH.print(i + 1); ITECH.print(", "); ITECH.println(timeStep, 3);
      delay(20);
    }
    ITECH.println("LIST:COUNT 0");
    responseStatus = true;
  } 
  else if (funcion == "DYN") {
    ITECH.println("FUNC DYN");
    ITECH.print("CURR:RANG "); ITECH.println(range[0].as<float>());
    ITECH.print("DYN:ALEV "); ITECH.println(resolution[0].as<float>(), 3);
    ITECH.print("DYN:AWID "); ITECH.println(resolution[1].as<float>(), 3);
    ITECH.print("DYN:BLEV "); ITECH.println(resolution[2].as<float>(), 3);
    ITECH.print("DYN:BWID "); ITECH.println(resolution[3].as<float>(), 3);
    responseStatus = true;
  }
  else if (funcion == "RST") {
    ITECH.println("*RST");
    responseStatus = true;
  }
}

void startLoad(String startCmd) {
  if (startCmd == "CFG_ON") {
    ITECH.println("SYST:REM");
    delay(50);
    ITECH.println("INP 1");
  } else if (startCmd == "CFG_OFF") {
    ITECH.println("INP 0");
  }
}

// --- CORE 0: LECTURA DE JSON (PC -> ESP32) ---
void TaskJSONReader(void * pvParameters) {
  for (;;) {
    if (Serial.available()) {
      StaticJsonDocument<512> doc;
      DeserializationError error = deserializeJson(doc, Serial);

      if (!error) {
        String funcion = doc["Funcion"] | "";
        String start = doc["Start"] | "";

        if (funcion != "Other" && funcion != "") {
          lastConf = funcion;
          configLoad(funcion, doc["Config"]["Resolution"], doc["Config"]["Range"]);
          
          // Respuesta JSON inmediata
          StaticJsonDocument<256> res;
          res["CONF"] = lastConf;
          res["response"] = responseStatus;
          res["muestras"] = (char*)NULL;
          serializeJson(res, Serial);
          Serial.println();
        } 
        else if (start == "Read") {
          flagFinish = true; // Dispara el CORE 1
        } 
        else {
          startLoad(start);
          StaticJsonDocument<256> res;
          res["CONF"] = start;
          res["response"] = true;
          serializeJson(res, Serial);
          Serial.println();
        }
      }
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

// --- CORE 1: LECTURA DE MEDICIONES (ITECH -> PC) ---
void TaskMeasure(void * pvParameters) {
  for (;;) {
    if (flagFinish) {
      StaticJsonDocument<1024> res;
      JsonArray muestras = res.createNestedArray("muestras");

      for (int i = 0; i < numMues; i++) {
        ITECH.println("MEAS:VOLT?;CURR?;POW?");
        delay(150); 

        if (ITECH.available()) {
          String data = ITECH.readStringUntil('\n');
          data.trim();
          data.replace("\t", ";"); // Formato voltaje;corriente;potencia
          muestras.add(data);
        } else {
          muestras.add("0.0;0.0;0.0");
        }
      }

      res["CONF"] = (char*)NULL;
      res["response"] = true;
      serializeJson(res, Serial);
      Serial.println();

      flagFinish = false;
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void setup() {
  Serial.begin(115200); // PC
  ITECH.begin(9600, SERIAL_8N1, RX1, TX1); // Carga IT8511A

  // Crear Tarea en Core 0 (Manejo de JSON y Comandos)
  xTaskCreatePinnedToCore(TaskJSONReader, "JSONTask", 10000, NULL, 1, &TaskCore1, 0);

  // Crear Tarea en Core 1 (Mediciones de alta prioridad)
  xTaskCreatePinnedToCore(TaskMeasure, "MeasureTask", 10000, NULL, 1, &TaskCore2, 1);

  Serial.println("{\"System\":\"Ready\", \"Mode\":\"DualCore\"}");
}

void loop() {
  // El loop se queda vacío porque estamos usando Tasks de FreeRTOS
  vTaskDelete(NULL); 
}