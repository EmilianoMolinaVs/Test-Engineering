#include <Arduino.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>

#define RX1 4 
#define TX1 5 

HardwareSerial ITECH(1); 

// --- Variables para LIST ---
struct Step { float current; float duration; };
Step softwareList[30];
int totalSteps = 0;
bool isListReady = false;

// --- Variables para DYNAMic ---
float dynA, dynB;
unsigned long dynTimeA, dynTimeB;
unsigned long lastDynMillis = 0;
bool dynState = false; 
bool runDynamic = false;

int numSamples = 1; // Cantidad de muestras a devolver

void sendSCPI(String cmd) {
  ITECH.println(cmd);
  delay(40); 
}

// --- NUEVA FUNCIÓN DE MEDICIÓN ---
void takeMeasurements() {
  DynamicJsonDocument res(2048);
  JsonArray muestras = res.createNestedArray("muestras");

  for (int i = 0; i < numSamples; i++) {
    ITECH.println("MEAS:VOLT?;CURR?;POW?");
    
    unsigned long t = millis();
    // Espera a que la carga responda con un timeout de 300ms
    while(!ITECH.available() && (millis() - t < 300)); 

    if (ITECH.available()) {
      String raw = ITECH.readStringUntil('\n');
      raw.trim();
      raw.replace("\t", ";"); // Cambia tabulador por punto y coma para tu web
      muestras.add(raw);
    } else {
      muestras.add("0.0;0.0;0.0"); // Si falla, agrega ceros
    }
    delay(50); // Pequeño respiro entre lecturas
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

  if (funcion == "LIST") {
    runDynamic = false;
    totalSteps = res[0] | 0;
    for (int i = 0; i < totalSteps && i < 30; i++) {
      softwareList[i].current = (i + 1) * (float)res[1];
      softwareList[i].duration = res[2];
    }
    sendSCPI("FUNC CURRent");
    sendSCPI("CURRent:RANGe " + String(range[0].as<float>()));
    isListReady = true;
    Serial.println("{\"DEBUG\":\"Software LIST configurado\"}");
  } 
  else if (funcion == "DYNAMic") {
    isListReady = false;
    dynA = res[0];
    dynTimeA = (unsigned long)((float)res[1] * 1000);
    dynB = res[2];
    dynTimeB = (unsigned long)((float)res[3] * 1000);
    
    sendSCPI("FUNC CURRent");
    sendSCPI("CURRent:RANGe " + String(range[0].as<float>()));
    runDynamic = true; 
    dynState = false; 
    Serial.println("{\"DEBUG\":\"Software DYNAMic configurado\"}");
  }
}

void runSoftwareList() {
  sendSCPI("INP 1");
  for (int i = 0; i < totalSteps; i++) {
    sendSCPI("CURRent " + String(softwareList[i].current, 3));
    delay(softwareList[i].duration * 1000);
  }
  isListReady = false;
  Serial.println("{\"DEBUG\":\"LIST finalizado\"}");
}

void setup() {
  Serial.begin(115200);
  ITECH.begin(9600, SERIAL_8N1, RX1, TX1);
  Serial.println("{\"System\":\"Ready\"}");
}

void loop() {
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

      if (funcion != "Other" && funcion != "") {
        configLoad(funcion, doc["Config"]["Resolution"], doc["Config"]["Range"]);
        Serial.print("{\"CONF\":\""); Serial.print(funcion); Serial.println("\",\"response\":true}");
      } 
      // --- AQUÍ ESTABA EL ERROR: AGREGAMOS EL COMANDO READ ---
      else if (start == "Read") {
        takeMeasurements();
      }
      // ------------------------------------------------------
      else if (start == "CFG_ON") {
        if (isListReady) runSoftwareList();
        else if (runDynamic) { sendSCPI("INP 1"); lastDynMillis = millis(); }
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