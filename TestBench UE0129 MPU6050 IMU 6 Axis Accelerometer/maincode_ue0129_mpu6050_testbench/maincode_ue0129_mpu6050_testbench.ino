/*
 * Firmware testbench MPU6050 en Pulsar C6 (ESP32-C6)
 * Control dual por USB Serial nativo o UART1 (PagWeb) mediante JSON delimitado por '\n'



{"Function":"ping"}
{"Function":"i2c_scan"}
{"Function":"init_i2c"}
{"Function":"read_i2c"}


 */

// ==== LIBRERIAS ====
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <ArduinoJson.h>

// ==== DECLARACIÓN DE GPIOS ====
#define SDA_PIN 6
#define SCL_PIN 7
#define RX2 15
#define TX2 19

// ==== MACROS Y CONSTANTES GLOBALES ====
#define I2C_FREQ 400000
#define ADDR_MPU 0x68
#define DEFAULT_STREAM_MS 200
#define MIN_STREAM_MS 20

// ==== OBJETOS Y ESTADO ====
Adafruit_MPU6050 mpu;
HardwareSerial PagWeb(1);

StaticJsonDocument<512> receiveJSON;
StaticJsonDocument<512> sendJSON;
Stream *activePort = &Serial;

bool mpuReady = false;
bool isStreaming = false;
unsigned long streamIntervalMs = DEFAULT_STREAM_MS;
unsigned long lastStreamMs = 0;

// ==== UTILIDADES ====
void sendResponse(JsonDocument &doc) {
  if (activePort == nullptr) return;
  serializeJson(doc, *activePort);
  activePort->println();
}

void sendError(const char *func, const char *msg) {
  sendJSON.clear();
  sendJSON["Function"] = func;
  sendJSON["status"] = "error";
  sendJSON["msg"] = msg;
  sendResponse(sendJSON);
}

bool isMpuConnected() {
  Wire.beginTransmission(ADDR_MPU);
  return (Wire.endTransmission() == 0);
}

float round3(float val) {
  return roundf(val * 1000.0f) / 1000.0f;
}

const char *accelRangeToStr(mpu6050_accel_range_t range) {
  switch (range) {
    case MPU6050_RANGE_2_G: return "+-2G";
    case MPU6050_RANGE_4_G: return "+-4G";
    case MPU6050_RANGE_8_G: return "+-8G";
    case MPU6050_RANGE_16_G: return "+-16G";
    default: return "unknown";
  }
}

const char *gyroRangeToStr(mpu6050_gyro_range_t range) {
  switch (range) {
    case MPU6050_RANGE_250_DEG: return "+-250 deg/s";
    case MPU6050_RANGE_500_DEG: return "+-500 deg/s";
    case MPU6050_RANGE_1000_DEG: return "+-1000 deg/s";
    case MPU6050_RANGE_2000_DEG: return "+-2000 deg/s";
    default: return "unknown";
  }
}

const char *filterBwToStr(mpu6050_bandwidth_t bw) {
  switch (bw) {
    case MPU6050_BAND_260_HZ: return "260 Hz";
    case MPU6050_BAND_184_HZ: return "184 Hz";
    case MPU6050_BAND_94_HZ: return "94 Hz";
    case MPU6050_BAND_44_HZ: return "44 Hz";
    case MPU6050_BAND_21_HZ: return "21 Hz";
    case MPU6050_BAND_10_HZ: return "10 Hz";
    case MPU6050_BAND_5_HZ: return "5 Hz";
    default: return "unknown";
  }
}

// ==== FUNCIONALIDADES DEL TESTBENCH ====
void handlePing() {
  sendJSON.clear();
  sendJSON["ping"] = "pong";
  sendJSON["status"] = "ok";
  sendResponse(sendJSON);
}

void handleI2cScan() {
  sendJSON.clear();
  sendJSON["Function"] = "i2c_scan";
  sendJSON["status"] = "ok";

  JsonArray devices = sendJSON.createNestedArray("devices");
  uint8_t count = 0;
  bool mpuFound = false;

  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      char hexAddr[5];
      snprintf(hexAddr, sizeof(hexAddr), "0x%02X", addr);
      devices.add(hexAddr);
      count++;
      if (addr == ADDR_MPU) {
        mpuFound = true;
      }
    }
  }

  sendJSON["count"] = count;
  sendJSON["mpu_found"] = mpuFound;
  sendResponse(sendJSON);
}

void handleInitMpu() {
  isStreaming = false;
  mpuReady = false;

  if (!mpu.begin(ADDR_MPU, &Wire)) {
    sendError("init_i2c", "MPU6050 not found");
    return;
  }

  Wire.setClock(I2C_FREQ);
  mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  mpuReady = true;

  sendJSON.clear();
  sendJSON["Function"] = "init_i2c";
  sendJSON["status"] = "ok";
  sendJSON["addr"] = "0x68";
  sendJSON["accel_range"] = accelRangeToStr(mpu.getAccelerometerRange());
  sendJSON["gyro_range"] = gyroRangeToStr(mpu.getGyroRange());
  sendJSON["filter_bw"] = filterBwToStr(mpu.getFilterBandwidth());
  sendResponse(sendJSON);
}

void publishMpuTelemetry() {
  if (!mpuReady) {
    isStreaming = false;
    sendError("read_i2c", "MPU6050 not initialized");
    return;
  }

  // Verifica presencia física antes de leer (evita lecturas congeladas si falla el contacto)
  if (!isMpuConnected()) {
    isStreaming = false;
    mpuReady = false;
    sendError("read_i2c", "I2C communication lost");
    return;
  }

  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  sendJSON.clear();
  sendJSON["Function"] = "read_i2c";
  sendJSON["status"] = "ok";

  JsonObject accel = sendJSON.createNestedObject("acc");
  accel["x"] = round3(a.acceleration.x);
  accel["y"] = round3(a.acceleration.y);
  accel["z"] = round3(a.acceleration.z);

  JsonObject gyro = sendJSON.createNestedObject("gyr");
  gyro["x"] = round3(g.gyro.x);
  gyro["y"] = round3(g.gyro.y);
  gyro["z"] = round3(g.gyro.z);

  sendJSON["temp_c"] = round3(temp.temperature);
  sendResponse(sendJSON);
}

void processCommand(const String &rawLine) {
  receiveJSON.clear();
  DeserializationError error = deserializeJson(receiveJSON, rawLine);

  if (error) {
    sendError("parse", error.c_str());
    return;
  }

  const char *func = receiveJSON["Function"] | "";

  if (strcmp(func, "ping") == 0) {
    handlePing();
  } else if (strcmp(func, "i2c_scan") == 0) {
    handleI2cScan();
  } else if (strcmp(func, "init_i2c") == 0) {
    handleInitMpu();
  } else if (strcmp(func, "read_i2c") == 0) {
    bool streamReq = receiveJSON["stream"] | false;
    if (streamReq) {
      unsigned long reqInterval = receiveJSON["interval_ms"] | DEFAULT_STREAM_MS;
      streamIntervalMs = (reqInterval < MIN_STREAM_MS) ? MIN_STREAM_MS : reqInterval;
      isStreaming = true;
      lastStreamMs = 0;  // Fuerza primera lectura inmediata
    } else {
      isStreaming = false;
      publishMpuTelemetry();
    }
  } else if (strcmp(func, "stop_read") == 0) {
    isStreaming = false;
    sendJSON.clear();
    sendJSON["Function"] = "stop_read";
    sendJSON["status"] = "ok";
    sendResponse(sendJSON);
  } else {
    sendError(func, "Unknown function");
  }
}

// ==== SETUP Y LOOP ====
void setup() {
  Serial.begin(115200);
  Serial.setTimeout(100);

  PagWeb.begin(115200, SERIAL_8N1, RX2, TX2);
  PagWeb.setTimeout(100);

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(I2C_FREQ);
  Wire.setTimeOut(50);  // Previene bloqueo del bus si un DUT mantiene SDA/SCL en bajo
}

void loop() {
  if (Serial.available() || PagWeb.available()) {
    activePort = Serial.available() ? static_cast<Stream *>(&Serial)
                                    : static_cast<Stream *>(&PagWeb);

    String line = activePort->readStringUntil('\n');
    line.trim();

    if (line.length() > 0) {
      processCommand(line);
    }
  }

  if (isStreaming && (millis() - lastStreamMs >= streamIntervalMs)) {
    lastStreamMs = millis();
    publishMpuTelemetry();
  }
}