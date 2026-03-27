#include <Arduino.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>

#define RX1 4 
#define TX1 5 

HardwareSerial ITECH(1); 

// --- Variables para LIST (Simulada por Software) ---
struct Step { float current; float duration; };
Step softwareList[30];
int totalSteps = 0;
bool isListReady = false;

// --- Variables para DYNAMic (Simulada por Software) ---
float dynA, dynB;
unsigned long dynTimeA, dynTimeB;
unsigned long lastDynMillis = 0;
bool dynState = false; 
bool runDynamic = false;

int numSamples = 1; // Ajustado a 1 muestra según tu solicitud

void sendSCPI(String cmd) {
  ITECH.println(cmd);
  delay(45); 
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
    } else {
      muestras.add("0.0;0.0;0.0");
    }
    if(numSamples > 1) delay(50); 
  }

  res["CONF"] = "ReadDone";
  res["response"] = true;
  serializeJson(res, Serial);
  Serial.println();
}

void configLoad(String funcion, JsonVariant res, JsonVariant range) {
  sendSCPI("SYST:REM");
  sendSCPI("INP 0");
  sendSCPI("*CLS");

  // Tomamos range[1] que es la Corriente (ignora el [0] de Voltaje)
  float currentRange = range[1].as<float>(); 
  if(currentRange <= 0) currentRange = 30.0; // Fail-safe a 30A

  if (funcion == "LIST") {
    runDynamic = false;
    totalSteps = res[0] | 0;
    float stepCurrent = res[1] | 0.0;
    float stepDuration = res[2] | 0.0;

    for (int i = 0; i < totalSteps && i < 30; i++) {
      softwareList[i].current = (i + 1) * stepCurrent;
      softwareList[i].duration = stepDuration;
    }
    sendSCPI("FUNC CURRent");
    sendSCPI("CURRent:RANGe " + String(currentRange));
    isListReady = true;
    runDynamic = false;
  } 
  else if (funcion == "DYN") {
    isListReady = false;
    dynA = res[0];
    dynTimeA = (unsigned long)((float)res[1] * 1000);
    dynB = res[2];
    dynTimeB = (unsigned long)((float)res[3] * 1000);
    
    sendSCPI("FUNC CURRent");
    sendSCPI("CURRent:RANGe " + String(currentRange));
    runDynamic = true; 
    dynState = false; 
    lastDynMillis = millis();
  }
}

void runSoftwareList() {
  sendSCPI("INP 1");
  for (int i = 0; i < totalSteps; i++) {
    sendSCPI("CURRent " + String(softwareList[i].current, 3));
    delay(softwareList[i].duration * 1000);
  }
  isListReady = false;
  sendSCPI("INP 0"); // Apagar al finalizar rampa
  Serial.println("{\"DEBUG\":\"LIST finalizado\"}");
}

void setup() {
  Serial.begin(115200);
  ITECH.begin(9600, SERIAL_8N1, RX1, TX1);
  Serial.println("{\"System\":\"Ready\"}");
}

void loop() {
  // Manejo de oscilación DYN (No bloqueante)
  if (runDynamic) {
    unsigned long currentMillis = millis();
    unsigned long interval = dynState ? dynTimeB : dynTimeA;

    if (currentMillis - lastDynMillis >= interval) {
      lastDynMillis = currentMillis;
      dynState = !dynState;
      float val = dynState ? dynB : dynA;
      sendSCPI("CURRent " + String(val, 3));
    }
  }

  if (Serial.available()) {
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, Serial);
    
    if (!error) {
      String funcion = doc["Funcion"] | "";
      String start = doc["Start"] | "";

      // 1. Si es configuración (LIST o DYN)
      if (funcion == "LIST" || funcion == "DYN") {
        configLoad(funcion, doc["Config"]["Resolution"], doc["Config"]["Range"]);
        Serial.print("{\"CONF\":\""); Serial.print(funcion); Serial.println("\",\"response\":true}");
        
        // Ejecución inmediata si el JSON de configuración trae CFG_ON
        if (start == "CFG_ON") {
          if (funcion == "LIST") runSoftwareList();
          else if (funcion == "DYN") sendSCPI("INP 1");
        }
      } 
      // 2. Si es comando de lectura
      else if (start == "Read") {
        takeMeasurements();
      }
      // 3. Si es solo control (Other)
      else if (start == "CFG_ON") {
        if (isListReady) runSoftwareList();
        else sendSCPI("INP 1");
        Serial.println("{\"CONF\":\"CFG_ON\",\"response\":true}");
      }
      else if (start == "CFG_OFF") {
        sendSCPI("INP 0");
        runDynamic = false;
        isListReady = false;
        Serial.println("{\"CONF\":\"CFG_OFF\",\"response\":true}");
      }
    }
  }
}