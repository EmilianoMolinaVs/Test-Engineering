#include <Arduino.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>

// Pines para la carga (Serial 1)
#define RX1 4 
#define TX1 5 

HardwareSerial ITECH(1); 

// --- Variables Globales ---
struct Step {
  float current;
  float duration;
};

Step softwareList[30]; 
int totalSteps = 0;
bool isListReady = false;
int numSamples = 10; // Default de muestras para el comando "Read"

// --- Funciones de Comunicación ---

void sendSCPI(String cmd) {
  ITECH.println(cmd);
  // Pequeño respiro para que la carga procese el comando
  delay(50); 
}

void configLoad(String funcion, JsonVariant res, JsonVariant range) {
  sendSCPI("SYST:REM");
  sendSCPI("INP 0");
  sendSCPI("*CLS");

  if (funcion == "LIST") {
    totalSteps = res[0] | 0;
    float stepAmps = res[1] | 0.1;
    float stepSecs = res[2] | 1.0;

    // Guardamos la escalera en memoria del ESP32
    for (int i = 0; i < totalSteps && i < 30; i++) {
      softwareList[i].current = (i + 1) * stepAmps;
      softwareList[i].duration = stepSecs;
    }
    
    sendSCPI("FUNC CURRent");
    String rangeCmd = "CURRent:RANGe " + String(range[0].as<float>());
    sendSCPI(rangeCmd);
    
    isListReady = true;
    Serial.println("{\"DEBUG\":\"Lista preparada por Software (Modo CC)\"}");
  } 
  else if (funcion == "DYNAMic") {
    // Usamos los nombres largos que confirmaste
    String rangeCmd = "CURRent:RANGe " + String(range[0].as<float>());
    sendSCPI(rangeCmd);
    sendSCPI("FUNC DYNAMic");
    
    sendSCPI("DYNAMic:ALEVel " + String(res[0].as<float>(), 3));
    sendSCPI("DYNAMic:AWIDth " + String(res[1].as<float>(), 3));
    sendSCPI("DYNAMic:BLEVel " + String(res[2].as<float>(), 3));
    sendSCPI("DYNAMic:BWIDth " + String(res[3].as<float>(), 3));
    Serial.println("{\"DEBUG\":\"Modo DYNAMic configurado\"}");
  }
}

void runSoftwareList() {
  Serial.println("{\"DEBUG\":\"Iniciando secuencia LIST...\"}");
  sendSCPI("INP 1");
  for (int i = 0; i < totalSteps; i++) {
    sendSCPI("CURRent " + String(softwareList[i].current, 3));
    // Esperamos el tiempo definido para este paso
    delay(softwareList[i].duration * 1000);
  }
  // isListReady = false; // Descomenta si quieres que solo corra una vez
}

void takeMeasurements() {
  DynamicJsonDocument res(2048);
  JsonArray muestras = res.createNestedArray("muestras");

  for (int i = 0; i < numSamples; i++) {
    ITECH.println("MEAS:VOLT?;CURR?;POW?");
    
    unsigned long t = millis();
    while(!ITECH.available() && (millis() - t < 300)); 

    if (ITECH.available()) {
      String raw = ITECH.readStringUntil('\n');
      raw.trim();
      raw.replace("\t", ";"); 
      muestras.add(raw);
    }
    delay(100); 
  }

  res["CONF"] = "ReadDone";
  res["response"] = true;
  serializeJson(res, Serial);
  Serial.println();
}

// --- Setup y Loop ---

void setup() {
  Serial.begin(115200);
  ITECH.begin(9600, SERIAL_8N1, RX1, TX1);
  Serial.println("{\"System\":\"Ready\", \"Hardware\":\"IT8511A_TTL\"}");
}

void loop() {
  if (Serial.available()) {
    DynamicJsonDocument doc(1024);
    DeserializationError err = deserializeJson(doc, Serial);

    if (!err) {
      String funcion = doc["Funcion"] | "";
      String start = doc["Start"] | "";

      if (funcion != "Other" && funcion != "") {
        configLoad(funcion, doc["Config"]["Resolution"], doc["Config"]["Range"]);
        Serial.print("{\"CONF\":\""); Serial.print(funcion); Serial.println("\",\"response\":true}");
      } 
      else if (start == "Read") {
        takeMeasurements();
      } 
      else if (start == "CFG_ON") {
        if (isListReady) runSoftwareList();
        else sendSCPI("INP 1");
        Serial.println("{\"CONF\":\"CFG_ON\",\"response\":true}");
      }
      else if (start == "CFG_OFF") {
        sendSCPI("INP 0");
        isListReady = false;
        Serial.println("{\"CONF\":\"CFG_OFF\",\"response\":true}");
      }
    }
  }
  delay(1);
}