/*
  Firmware para el TestBench UE0116: DevLab I2C MPR121QR2 Capacitive Touch Sensor

  Este firmware inicializa el sensor de toque MPR121QR2 en modo hibrido:
  - Electrodos E0..E3 se usan como entradas tactiles.
  - Electrodos E4..E11 se usan como salidas GPIO digitales para pruebas.

  La comunicacion con la interfaz de pruebas PagWeb se realiza por UART2
  usando mensajes JSON de entrada/salida.
*/

#include <Arduino.h>
#include <Wire.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>

// ======= PINS =======
static constexpr uint8_t SDA_PIN = 6;
static constexpr uint8_t SCL_PIN = 7;
static constexpr uint8_t RX2_PIN = 15;  // UART2 RX para PagWeb
static constexpr uint8_t TX2_PIN = 19;  // UART2 TX para PagWeb

static constexpr uint8_t ELECT4_PIN = 20;
static constexpr uint8_t ELECT5_PIN = 21;
static constexpr uint8_t ELECT6_PIN = 18;
static constexpr uint8_t ELECT7_PIN = 2;
static constexpr uint8_t ELECT8_PIN = 14;
static constexpr uint8_t ELECT9_PIN = 0;
static constexpr uint8_t ELECTA_PIN = 1;
static constexpr uint8_t ELECTB_PIN = 3;

static const uint8_t electrPorts[] = {
  ELECT4_PIN,
  ELECT5_PIN,
  ELECT6_PIN,
  ELECT7_PIN,
  ELECT8_PIN,
  ELECT9_PIN,
  ELECTA_PIN,
  ELECTB_PIN,
};

// ======= I2C / MPR121 =======
static constexpr uint8_t MPR_ADDR = 0x5A;
static constexpr uint8_t REG_ELECTRODE_CONFIG = 0x5E;
static constexpr uint8_t REG_GPIO_CONTROL_0 = 0x73;
static constexpr uint8_t REG_GPIO_CONTROL_1 = 0x74;
static constexpr uint8_t REG_GPIO_DIRECTION = 0x76;
static constexpr uint8_t REG_GPIO_ENABLE = 0x77;
static constexpr uint8_t REG_GPIO_SET = 0x78;
static constexpr uint8_t REG_GPIO_CLEAR = 0x79;
static constexpr uint8_t REG_GPIO_DATA = 0x75;
static constexpr uint8_t REG_TOUCH_STATUS_L = 0x00;

static constexpr uint8_t TOUCH_THRESHOLD = 0x20;
static constexpr uint8_t RELEASE_THRESHOLD = 0x10;
static constexpr uint8_t NUM_TOUCH_ELECTRODES = 4;
static constexpr uint8_t GPIO_MASK_ALL = 0xFF;
static const uint8_t SWEEP_ORDER[8] = { 4, 5, 6, 7, 8, 9, 10, 11 };

static constexpr size_t JSON_BUFFER_SIZE = 256;

// ======= Globals =======
HardwareSerial PagWeb(1);
String JSON_entrada;
StaticJsonDocument<JSON_BUFFER_SIZE> receiveJSON;
StaticJsonDocument<JSON_BUFFER_SIZE> sendJSON;
uint8_t lastTouchBits = 0;

// ======= Funciones auxiliares =======
void writeReg(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(MPR_ADDR);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
  delayMicroseconds(100);
}

uint8_t readReg(uint8_t reg) {
  Wire.beginTransmission(MPR_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(MPR_ADDR, (uint8_t)1);
  if (Wire.available() < 1) {
    return 0;
  }
  return Wire.read();
}

void sendJsonToPagWeb() {
  serializeJson(sendJSON, PagWeb);
  PagWeb.println();
}

void serialDebug(const String &message) {
  String escaped = message;
  escaped.replace("\"", "\\\"");
  Serial.println("{\"debug\": \"" + escaped + "\"}");
}

void pagwebDebug(const String &message) {
  String escaped = message;
  escaped.replace("\"", "\\\"");
  PagWeb.println("{\"debug\": \"" + escaped + "\"}");
}

bool gpioStatus(uint8_t port) {
  delay(50);
  int sum = 0;
  for (int i = 0; i < 5; ++i) {
    if (digitalRead(port) == HIGH) {
      sum++;
    }
  }
  return (sum == 5);
}

// ======= MPR121 Initialization =======
void mpr121InitTouchAndGpio() {
  writeReg(REG_ELECTRODE_CONFIG, 0x00);
  delay(10);

  writeReg(0x2B, 0x01);
  writeReg(0x2C, 0x01);
  writeReg(0x2D, 0x00);
  writeReg(0x2E, 0x00);
  writeReg(0x2F, 0x01);
  writeReg(0x30, 0x01);
  writeReg(0x31, 0xFF);
  writeReg(0x32, 0x02);

  for (uint8_t i = 0; i < NUM_TOUCH_ELECTRODES; ++i) {
    writeReg(0x41 + (i * 2), TOUCH_THRESHOLD);
    writeReg(0x42 + (i * 2), RELEASE_THRESHOLD);
  }

  writeReg(REG_GPIO_CONTROL_0, 0x00);
  writeReg(REG_GPIO_CONTROL_1, 0x00);
  writeReg(REG_GPIO_DIRECTION, GPIO_MASK_ALL);
  writeReg(REG_GPIO_ENABLE, GPIO_MASK_ALL);
  writeReg(REG_GPIO_CLEAR, GPIO_MASK_ALL);

  writeReg(REG_ELECTRODE_CONFIG, 0x84);
  delay(20);
}

uint16_t readTouchStatus() {
  Wire.beginTransmission(MPR_ADDR);
  Wire.write(REG_TOUCH_STATUS_L);
  Wire.endTransmission(false);
  Wire.requestFrom(MPR_ADDR, (uint8_t)2);
  if (Wire.available() < 2) {
    return 0;
  }
  uint8_t lsb = Wire.read();
  uint8_t msb = Wire.read();
  return (uint16_t)lsb | ((uint16_t)msb << 8);
}

void reportTouchEdges(uint16_t touchBits) {
  sendJSON.clear();
  for (uint8_t i = 0; i < NUM_TOUCH_ELECTRODES; ++i) {
    uint16_t bit = (uint16_t)(1u << i);
    bool previous = (lastTouchBits & bit) != 0;
    bool current = (touchBits & bit) != 0;

    if (!previous && current) {
      sendJSON["touch"] = "E" + String(i);
      Serial.print("TOUCH E");
      Serial.println(i);
    }
    if (previous && !current) {
      sendJSON["release"] = "E" + String(i);
      Serial.print("RELEASE E");
      Serial.println(i);
    }
  }
  if (sendJSON.size() > 0) {
    sendJsonToPagWeb();
  }
  lastTouchBits = touchBits;
}

void setOutputsRaw(uint8_t rawMask) {
  writeReg(REG_GPIO_CLEAR, GPIO_MASK_ALL);
  writeReg(REG_GPIO_SET, rawMask);
}

void handleDigitalScan() {
  sendJSON.clear();
  int totalOk = 0;
  for (uint8_t i = 0; i < sizeof(electrPorts); ++i) {
    uint8_t electrode = SWEEP_ORDER[i];
    uint8_t mask = (uint8_t)(1u << (electrode - 4));
    setOutputsRaw(mask);
    delay(100);

    if (gpioStatus(electrPorts[i])) {
      sendJSON["port" + String(i)] = "OK";
      totalOk++;
    } else {
      sendJSON["port" + String(i)] = "FAIL";
    }

    uint8_t data = readReg(REG_GPIO_DATA);
    Serial.print("ON ELE");
    Serial.print((int)electrode);
    Serial.print(" | GPIO_DATA=0x");
    Serial.println(data, HEX);
    delay(100);
  }

  if (totalOk == 8) {
    sendJSON["Result"] = "OK";
  }
  sendJsonToPagWeb();
}

void handlePagWebCommand() {
  JSON_entrada = PagWeb.readStringUntil('\n');
  DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);
  if (error) {
    serialDebug("JSON parse error");
    return;
  }

  String functionName = receiveJSON["Function"];
  if (functionName == "ping") {
    sendJSON.clear();
    sendJSON["ping"] = "pong";
    sendJsonToPagWeb();
  } else if (functionName == "init") {
    Wire.begin(SDA_PIN, SCL_PIN);
    mpr121InitTouchAndGpio();
    pagwebDebug("GPIOS initialized");
  } else if (functionName == "digitalScan") {
    handleDigitalScan();
  } else if (functionName == "restart") {
    ESP.restart();
  }
}

void initGpioOnly() {
  writeReg(REG_ELECTRODE_CONFIG, 0x00);
  delay(10);
  writeReg(REG_GPIO_CONTROL_0, 0x00);
  writeReg(REG_GPIO_CONTROL_1, 0x00);
  writeReg(REG_GPIO_DIRECTION, GPIO_MASK_ALL);
  writeReg(REG_GPIO_ENABLE, GPIO_MASK_ALL);
  setOutputsRaw(0x00);
}

void setup() {
  Serial.begin(115200);
  delay(100);
  serialDebug("Serial initialized");

  PagWeb.begin(115200, SERIAL_8N1, RX2_PIN, TX2_PIN);
  pagwebDebug("UART PagWeb initialized");

  Wire.begin(SDA_PIN, SCL_PIN);
  mpr121InitTouchAndGpio();

  pinMode(ELECT4_PIN, INPUT);
  pinMode(ELECT5_PIN, INPUT);
  pinMode(ELECT6_PIN, INPUT);
  pinMode(ELECT7_PIN, INPUT);
  pinMode(ELECT8_PIN, INPUT);
  pinMode(ELECT9_PIN, INPUT);
  pinMode(ELECTA_PIN, INPUT);
  pinMode(ELECTB_PIN, INPUT);
}

void loop() {
  if (PagWeb.available()) {
    handlePagWebCommand();
  } else {
    uint16_t touchStatus = readTouchStatus();
    uint16_t touchBits = (uint16_t)(touchStatus & 0x0FFF);
    reportTouchEdges(touchBits);
    delay(50);
  }
}
