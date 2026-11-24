
#include <Arduino.h>
#include <Wire.h>
#include <DevLab_GT365xx.h>

#if defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_RP2350)
#define I2C_BUS Wire1
constexpr int SDA_PIN = 12;
constexpr int SCL_PIN = 13;
#elif defined(ARDUINO_ARCH_ESP32)
#define I2C_BUS Wire
constexpr int SDA_PIN = 6;
constexpr int SCL_PIN = 7;
#elif defined(ARDUINO_ARCH_STM32)
// STM32duino: default I2C pins of the selected board.
#define I2C_BUS Wire
constexpr int SDA_PIN = SDA;
constexpr int SCL_PIN = SCL;
#elif defined(ARDUINO_ARCH_AVR)
// AVR has fixed I2C pins (Uno/Nano: A4/A5, Mega: 20/21, Leonardo: 2/3);
// SDA/SCL come from the board variant and begin() ignores the pin numbers.
#define I2C_BUS Wire
constexpr int SDA_PIN = SDA;
constexpr int SCL_PIN = SCL;
#else
#error "Use an ESP32, RP2040, RP2350, STM32, or AVR master"
#endif

constexpr uint32_t I2C_FREQ = 400000UL;
constexpr uint8_t SENSOR_ADDRESS = 0x26U;
constexpr uint32_t READ_INTERVAL_MS = 250U;

DevLab_GT365xx sensor(I2C_BUS, SENSOR_ADDRESS, I2C_FREQ);

void setup() {
  Serial.begin(9600);

  sensor.beginRecovered(SDA_PIN, SCL_PIN);
  if (!sensor.busReady()) {
    Serial.println("ERROR: I2C bus is blocked");
    return;
  }

  if (!sensor.isConnected()) {
    Serial.println("ERROR: expected GT36537 DDP device ID 0x0106 at 0x26");
    return;
  }

  sensor.printInfo();
  Serial.println("GT36537 ready: PA2 ADC0 reads the LDR divider");

  // addr=0x26 id=0x0106 type=gt36537-light protocol=1.0 firmware=1.0 hardware=1.0 caps=0x199 match=yes
}

void loop() {
  static uint32_t lastRead = 0U;
  if (millis() - lastRead < READ_INTERVAL_MS) {
    return;
  }
  lastRead = millis();

  uint16_t raw = 0U;
  if (sensor.readRaw(raw)) {
    float percent = (raw * 100.0f) / 4095.0f;
#if defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_ARCH_STM32)
    // AVR Serial has no printf(), and newlib-nano/avr-libc printf lack %f.
    Serial.print("adc0=");
    Serial.print(raw);
    Serial.print(" percent=");
    Serial.println(percent, 1);
#else
    Serial.printf("adc0=%u percent=%.1f\n", raw, percent);
#endif
  } else {
    Serial.println("ERROR: light read failed");
  }
}
