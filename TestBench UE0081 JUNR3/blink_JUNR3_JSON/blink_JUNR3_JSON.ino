// =========================================================
// Blink Firmware Demo y Test JUNR3
// =========================================================

#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <HardwareSerial.h>
#include "DHT.h"

// ==== DECLARACIÓN DE PINES DIGITALES ====
// GPIO D0 -> RX UART con CH340
// GPIO D1 -> TX UART con CH340
// GPIO D2 -> Libre
// GPIO D3 -> Libre
#define DHT_PIN 4       // GPIO D4 -> Sensor de Temperatura DHT11
#define BUZZER_PIN 5    // GPIO D5 -> Buzzer en Shield
#define IR_PIN 6        // GPIO D6 -> LED Infrarrojo en Shield
#define NEOP_RETURN 7   // GPIO D7 -> Retorno de Neopixel
#define NEOP_PIN 8      // GPIO D8 -> Matriz de Neopixel
#define RGB_RD09 9      // GPIO D09 -> Rojo RGB Shield
#define RGB_GD10 10     // GPIO D10 -> Verde RGB Shield
#define RGB_BD11 11     // GPIO D11 -> Azul RGB Shield
#define LED_D12 12      // GPIO D12 -> LED Rojo en Shield D12
#define LED_BUILTIN 13  // LED interno de la placa

// ==== PINES ANALÓGICOS ====
#define A0_PIN A0        // A0 Salida de datos para A1
#define A1_PIN A1        // A1 Entrada de datos desde A0
#define A2_PIN A2        // A2 para lectura de LM35 en Shield
#define A3_PIN A3        // A3 para lectura de sensor de luz TEMT6000
#define RELAY_PIN A4     // A4 Salida de activación relay
#define RELAY_RETURN A5  // A5 Entrada de datos desde A4

// ==== DECLARACIÓN DE VARIABLES Y OBJETOS ====
#define NUMPIXELS 25
#define DELAYVAL 50

Adafruit_NeoPixel pixels(NUMPIXELS, NEOP_PIN, NEO_GRB + NEO_KHZ800);
#define DHTTYPE DHT11  // Tipo de sensor
DHT dht(DHT_PIN, DHTTYPE);

String JSON_entrada;                  ///< Buffer para recibir JSON desde PagWeb
StaticJsonDocument<256> receiveJSON;  ///< Documento JSON para parsear datos recibidos

String JSON_lectura;               ///< Buffer para transmitir JSON de respuesta
StaticJsonDocument<256> sendJSON;  ///< Documento JSON para armar respuestas

// =========================================================
// CONFIGURACIÓN INICIAL (SETUP)
// =========================================================
void setup() {
  Serial.begin(115200);
  delay(100);
  printDebug("Serial inicializado");

  // Configuración de pines de salida
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(RGB_RD09, OUTPUT);
  pinMode(RGB_GD10, OUTPUT);
  pinMode(RGB_BD11, OUTPUT);
  pinMode(LED_D12, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  // Configuración de pines de entrada
  pinMode(IR_PIN, INPUT);
  pinMode(NEOP_RETURN, INPUT);
  pinMode(RELAY_RETURN, INPUT);

  // Estado inicial del relay de demanda de corriente
  //digitalWrite(RELAY_PIN, HIGH);

  // Inicialización de Neopixels
  pixels.begin();
  pixels.setBrightness(4);
  pixels.clear();
  pixels.show();

  // Inicialización del DHT
  dht.begin();
}

// =========================================================
// BUCLE PRINCIPAL (LOOP)
// =========================================================
void loop() {
  if (Serial.available()) {
    JSON_entrada = Serial.readStringUntil('\n');
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);

    if (!error) {
      String Function = receiveJSON["Function"];

      int opc = 0;
      if (Function == "ping") opc = 1;          // {"Function": "ping"}
      else if (Function == "buzzer") opc = 2;   // {"Function": "buzzer"}
      else if (Function == "leds") opc = 3;     // {"Function": "leds"}
      else if (Function == "testAll") opc = 4;  // {"Function": "testAll"}

      else if (Function == "relayOn") opc = 5;   // {"Function": "relayOn"}
      else if (Function == "relayOff") opc = 6;  // {"Function": "relayOff"}

      switch (opc) {
        case 1:
          {
            sendJSON.clear();
            sendJSON["ping"] = "pong";
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }

        case 2:
          {
            for (int i = 0; i < 5; i++) {
              melodyBuzzer();
              delay(50);
            }
            break;
          }

        case 3:
          {
            int delay_ms = 100;

            for (int i = 0; i < 10; i++) {
              digitalWrite(LED_BUILTIN, HIGH);
              digitalWrite(LED_D12, LOW);
              digitalWrite(RGB_RD09, HIGH);
              digitalWrite(RGB_GD10, LOW);
              digitalWrite(RGB_BD11, LOW);
              delay(delay_ms);

              digitalWrite(LED_BUILTIN, LOW);
              digitalWrite(LED_D12, HIGH);
              digitalWrite(RGB_RD09, LOW);
              digitalWrite(RGB_GD10, HIGH);
              digitalWrite(RGB_BD11, LOW);
              delay(delay_ms);

              digitalWrite(LED_BUILTIN, HIGH);
              digitalWrite(LED_D12, LOW);
              digitalWrite(RGB_RD09, LOW);
              digitalWrite(RGB_GD10, LOW);
              digitalWrite(RGB_BD11, HIGH);
              delay(delay_ms);

              digitalWrite(LED_BUILTIN, HIGH);
              digitalWrite(LED_D12, HIGH);
              digitalWrite(RGB_RD09, HIGH);
              digitalWrite(RGB_GD10, LOW);
              digitalWrite(RGB_BD11, HIGH);
              delay(delay_ms);
            }
            break;
          }

        case 4:
          {
            sendJSON.clear();

            bool statusD4 = DHTmeas();
            sendJSON["D4"] = statusD4;

            bool statusD6 = IRLEDreceptor();
            sendJSON["D6"] = statusD6;

            // Validación A0 y A1 con envio de secuencia de datos
            bool statusA01 = testGpios(A1_PIN, A0_PIN);
            sendJSON["A0"] = statusA01;
            sendJSON["A1"] = statusA01;

            // Validación A2 LM35 en Shield (Umbral > 10)
            bool statusA2 = testAnalogPin(A2_PIN, "A2", 10.0);
            sendJSON["A2"] = statusA2;

            // Validación A3 Sensor de Luz TEMT6000 (Umbral > 100)
            bool statusA3 = testAnalogPin(A3_PIN, "A3", 100.0);
            sendJSON["A3"] = statusA3;

            if (statusD4 && statusD6 && statusA01 && statusA2 && statusA3) {
              sendJSON["testAll"] = "OK";
              sendJSON["Result"] = "OK";
            } else {
              sendJSON["testAll"] = "FAIL";
            }

            // Envío del JSON final con todos los resultados del test
            serializeJson(sendJSON, Serial);
            Serial.println();

            melodyBuzzer();
            break;
          }

        case 5:
          digitalWrite(RELAY_PIN, LOW);
          break;

        case 6:
          digitalWrite(RELAY_PIN, HIGH);
          break;

        default:
          printDebug("Opción no disponible");
          break;
      }
    }
  } else {
    demoNeop();
  }
}

// =========================================================
// FUNCIONES DE UTILIDAD Y CORE
// =========================================================

// Interrupción por software para no bloquear el puerto serial
bool smartDelay(unsigned long wait) {
  unsigned long startMillis = millis();
  while (millis() - startMillis < wait) {
    if (Serial.available() > 0) {
      return true;  // ¡Llegó un JSON! Abortar la espera.
    }
    yield();  // Buena práctica para evitar bloqueos internos
  }
  return false;
}

// Función centralizada para enviar mensajes de debug en formato JSON
void printDebug(String str) {
  str.replace("\"", "\\\"");  // Escapa comillas para no romper el JSON
  Serial.println("{\"debug\": \"" + str + "\"}");
}

// =========================================================
// FUNCIONES DE SENSORES Y ACTUADORES
// =========================================================

// Función de reconocimiento y lectura de sensor DHT Temp y Hum D4
bool DHTmeas() {
  printDebug("Lectura Sensor de Temperatura en D4");
  float h = dht.readHumidity();
  float t = dht.readTemperature();  // °C

  if (isnan(h) || isnan(t)) {
    printDebug("Error leyendo el DHT11");
    return false;
  }

  // Se concatena todo en un solo String para enviarlo limpio en el JSON
  printDebug("Temperatura: " + String(t) + " °C | Humedad: " + String(h) + " %");

  return true;
}

// Función de lectura digital en GPIO8 D6
bool IRLEDreceptor() {
  printDebug("Lectura de sensor IR en D6");
  int n = 0;
  int lect = 0;
  for (int i = 0; i < 10; i++) {
    n = digitalRead(IR_PIN);
    lect += n;
  }

  if (lect == 10) {
    printDebug("Lectura HIGH estable en D6: " + String(lect) + "/10");
    return true;
  } else {
    printDebug("Lectura inestable en D6: " + String(lect) + "/10");
    return false;
  }
}

// Función de melodía en Buzzer D5
void melodyBuzzer() {
  printDebug("Ejecución de rutina en Buzzer D5");

#define NOTE_B0 31
#define NOTE_C1 33
#define NOTE_CS1 35
#define NOTE_D1 37
#define NOTE_DS1 39
#define NOTE_E1 41
#define NOTE_F1 44
#define NOTE_FS1 46
#define NOTE_G1 49
#define NOTE_GS1 52
#define NOTE_A1 55
#define NOTE_AS1 58
#define NOTE_B1 62
#define NOTE_C2 65
#define NOTE_CS2 69
#define NOTE_D2 73
#define NOTE_DS2 78
#define NOTE_E2 82
#define NOTE_F2 87
#define NOTE_FS2 93
#define NOTE_G2 98
#define NOTE_GS2 104
#define NOTE_A2 110
#define NOTE_AS2 117
#define NOTE_B2 123
#define NOTE_C3 131
#define NOTE_CS3 139
#define NOTE_D3 147
#define NOTE_DS3 156
#define NOTE_E3 165
#define NOTE_F3 175
#define NOTE_FS3 185
#define NOTE_G3 196
#define NOTE_GS3 208
#define NOTE_A3 220
#define NOTE_AS3 233
#define NOTE_B3 247
#define NOTE_C4 262
#define NOTE_CS4 277
#define NOTE_D4 294
#define NOTE_DS4 311
#define NOTE_E4 330
#define NOTE_F4 349
#define NOTE_FS4 370
#define NOTE_G4 392
#define NOTE_GS4 415
#define NOTE_A4 440
#define NOTE_AS4 466
#define NOTE_B4 494
#define NOTE_C5 523
#define NOTE_CS5 554
#define NOTE_D5 587
#define NOTE_DS5 622
#define NOTE_E5 659
#define NOTE_F5 698
#define NOTE_FS5 740
#define NOTE_G5 784
#define NOTE_GS5 831
#define NOTE_A5 880
#define NOTE_AS5 932
#define NOTE_B5 988
#define NOTE_C6 1047
#define NOTE_CS6 1109
#define NOTE_D6 1175
#define NOTE_DS6 1245
#define NOTE_E6 1319
#define NOTE_F6 1397
#define NOTE_FS6 1480
#define NOTE_G6 1568
#define NOTE_GS6 1661
#define NOTE_A6 1760
#define NOTE_AS6 1865
#define NOTE_B6 1976
#define NOTE_C7 2093
#define NOTE_CS7 2217
#define NOTE_D7 2349
#define NOTE_DS7 2489
#define NOTE_E7 2637
#define NOTE_F7 2794
#define NOTE_FS7 2960
#define NOTE_G7 3136
#define NOTE_GS7 3322
#define NOTE_A7 3520
#define NOTE_AS7 3729
#define NOTE_B7 3951
#define NOTE_C8 4186
#define NOTE_CS8 4435
#define NOTE_D8 4699
#define NOTE_DS8 4978

#define REST 0

  int melody[] = {
    NOTE_G6, NOTE_A6, NOTE_B6,
    NOTE_G6, NOTE_A6, NOTE_B6,
    NOTE_C7
  };

  int durations[] = {
    12, 12, 12,
    12, 12, 12,
    4
  };

  for (int thisNote = 0; thisNote < sizeof(melody) / sizeof(int); thisNote++) {
    int noteDuration = 1000 / durations[thisNote];
    if (melody[thisNote] != REST) {
      tone(BUZZER_PIN, melody[thisNote], noteDuration);
    }
    delay(noteDuration * 1.30);
    noTone(BUZZER_PIN);
  }

  delay(1000);  // Pausa antes de repetir
}

// =========================================================
// FUNCIONES DE PRUEBA DE PINES (TESTBENCH)
// =========================================================

/*
 * testGpios(uint8_t gpioA, uint8_t gpioB): Prueba integridad bidireccional
 * Realiza pruebas en ambas direcciones: A->B y B->A
 */
bool testGpios(uint8_t gpioA, uint8_t gpioB) {
  bool resultAB = testSequence(gpioA, gpioB);
  if (!resultAB) return false;

  delay(10);  // Pausa entre pruebas

  bool resultBA = testSequence(gpioB, gpioA);
  if (!resultBA) return false;

  return true;  // Ambas pruebas correctas
}

/*
 * testSequence(uint8_t gpioOut, uint8_t gpioIn): Prueba secuencia de bit
 */
bool testSequence(uint8_t gpioOut, uint8_t gpioIn) {
  pinMode(gpioOut, OUTPUT);
  pinMode(gpioIn, INPUT);  // Configuración con resistencia

  uint8_t testPattern[] = { 1, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1, 1 };

  for (int i = 0; i < sizeof(testPattern); i++) {
    digitalWrite(gpioOut, testPattern[i]);  // Envía bit
    delay(10);                              // Espera de estabilización

    int readValue = digitalRead(gpioIn);  // Lee bit

    if (readValue != testPattern[i]) {
      return false;
    }
  }
  return true;  // Todos los bits coincidieron correctamente
}

/*
 * testAnalogPin: Lee un pin analógico 10 veces, promedia y evalúa contra un umbral.
 * * Parámetros:
 * - pin: El pin físico o constante a leer (ej. A2_PIN)
 * - pinName: El nombre en texto para el debug (ej. "A2")
 * - threshold: El valor mínimo que debe superar el promedio para ser true
 */
bool testAnalogPin(uint8_t pin, String pinName, float threshold) {
  float lect = 0;

  for (int i = 0; i < 10; i++) {
    lect += analogRead(pin);
  }

  float avg = lect / 10.0;
  printDebug("El promedio de lecturas en " + pinName + ": " + String(avg));

  // Como la evaluación (avg > threshold) ya da como resultado true o false,
  // podemos retornarlo directamente y nos ahorramos el if/else
  return (avg > threshold);
}

// =========================================================
// RUTINAS DE LUCES NEOPIXEL (DEMO)
// =========================================================

void demoNeop() {
  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(LED_D12, LOW);
  digitalWrite(RGB_RD09, HIGH);
  digitalWrite(RGB_GD10, LOW);
  digitalWrite(RGB_BD11, LOW);

  rainbowCycle(10);
  if (Serial.available()) return;  // Revisa si el ciclo se abortó

  if (smartDelay(500)) return;  // Pausa inteligente de 500ms

  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(LED_D12, HIGH);
  digitalWrite(RGB_RD09, LOW);
  digitalWrite(RGB_GD10, HIGH);
  digitalWrite(RGB_BD11, LOW);

  colorWipe(pixels.Color(255, 0, 0), DELAYVAL);
  if (Serial.available()) return;

  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(LED_D12, LOW);
  digitalWrite(RGB_RD09, LOW);
  digitalWrite(RGB_GD10, LOW);
  digitalWrite(RGB_BD11, HIGH);

  colorWipe(pixels.Color(0, 255, 0), DELAYVAL);
  if (Serial.available()) return;

  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(LED_D12, HIGH);
  digitalWrite(RGB_RD09, HIGH);
  digitalWrite(RGB_GD10, LOW);
  digitalWrite(RGB_BD11, LOW);

  colorWipe(pixels.Color(0, 0, 255), DELAYVAL);
  if (Serial.available()) return;

  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(LED_D12, LOW);
  digitalWrite(RGB_RD09, LOW);
  digitalWrite(RGB_GD10, HIGH);
  digitalWrite(RGB_BD11, LOW);

  if (smartDelay(500)) return;

  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(LED_D12, LOW);
  digitalWrite(RGB_RD09, LOW);
  digitalWrite(RGB_GD10, HIGH);
  digitalWrite(RGB_BD11, LOW);

  theaterChase(pixels.Color(127, 127, 127), DELAYVAL);
  if (Serial.available()) return;

  if (smartDelay(500)) return;

  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(LED_D12, HIGH);
  digitalWrite(RGB_RD09, LOW);
  digitalWrite(RGB_GD10, LOW);
  digitalWrite(RGB_BD11, HIGH);

  quetzalcoatlEffect(60);
  if (Serial.available()) return;

  if (smartDelay(500)) return;

  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(LED_D12, LOW);
  digitalWrite(RGB_RD09, HIGH);
  digitalWrite(RGB_GD10, LOW);
  digitalWrite(RGB_BD11, LOW);

  unsigned long start = millis();
  while (millis() - start < 3000) {
    confetti(30);
    if (Serial.available()) return;
  }

  if (smartDelay(500)) return;

  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(LED_D12, HIGH);
  digitalWrite(RGB_RD09, LOW);
  digitalWrite(RGB_GD10, HIGH);
  digitalWrite(RGB_BD11, LOW);

  scanner(pixels.Color(255, 0, 255), 50);
  if (Serial.available()) return;

  if (smartDelay(500)) return;

  imageGalleryDemo();
  if (Serial.available()) return;

  if (smartDelay(500)) return;

  pixels.clear();
  pixels.show();

  if (smartDelay(500)) return;
}

void colorWipe(uint32_t color, int wait) {
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, color);
    pixels.show();
    if (smartDelay(wait)) return;
  }
}

void theaterChase(uint32_t color, int wait) {
  for (int a = 0; a < 10; a++) {
    for (int b = 0; b < 3; b++) {
      pixels.clear();
      for (int i = b; i < NUMPIXELS; i += 3) {
        pixels.setPixelColor(i, color);
      }
      pixels.show();
      if (smartDelay(wait)) return;
    }
  }
}

uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else if (WheelPos < 170) {
    WheelPos -= 85;
    return pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  } else {
    WheelPos -= 170;
    return pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
}

void rainbowCycle(int wait) {
  for (int j = 0; j < 256; j++) {
    for (int i = 0; i < NUMPIXELS; i++) {
      pixels.setPixelColor(i, Wheel((i * 256 / NUMPIXELS + j) & 255));
    }
    pixels.show();
    if (smartDelay(wait)) return;
  }
}

void scanner(uint32_t color, int wait) {
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.clear();
    pixels.setPixelColor(i, color);
    pixels.show();
    if (smartDelay(wait)) return;
  }
  for (int i = NUMPIXELS - 2; i > 0; i--) {
    pixels.clear();
    pixels.setPixelColor(i, color);
    pixels.show();
    if (smartDelay(wait)) return;
  }
}

void confetti(int wait) {
  for (int i = 0; i < NUMPIXELS; i++) {
    uint32_t c = pixels.getPixelColor(i);
    uint8_t r = (c >> 16) & 0xFF;
    uint8_t g = (c >> 8) & 0xFF;
    uint8_t b = c & 0xFF;
    pixels.setPixelColor(i, pixels.Color(r * 0.94, g * 0.94, b * 0.94));
  }
  int pos = random(NUMPIXELS);
  pixels.setPixelColor(pos, Wheel(random(0, 255)));
  pixels.show();
  if (smartDelay(wait)) return;
}

void fadeInOut(uint32_t color) {
  uint8_t r = (color >> 16) & 0xFF;
  uint8_t g = (color >> 8) & 0xFF;
  uint8_t b = color & 0xFF;
  for (int bri = 0; bri <= 64; bri++) {
    pixels.setBrightness(bri);
    colorWipe(pixels.Color(r, g, b), 5);
    if (Serial.available()) return;
  }
  for (int bri = 64; bri >= 0; bri--) {
    pixels.setBrightness(bri);
    colorWipe(pixels.Color(r, g, b), 5);
    if (Serial.available()) return;
  }
}

uint32_t aztecPalette[] = {
  pixels.Color(34, 139, 34),
  pixels.Color(218, 165, 32),
  pixels.Color(178, 34, 34),
  pixels.Color(255, 215, 0),
  pixels.Color(0, 100, 0)
};
const int SNAKE_LEN = 7;

void quetzalcoatlEffect(int wait) {
  int head = 0;
  int paletteSize = sizeof(aztecPalette) / sizeof(aztecPalette[0]);
  for (int step = 0; step < NUMPIXELS + SNAKE_LEN; step++) {
    pixels.clear();
    for (int i = 0; i < SNAKE_LEN; i++) {
      int idx = (head - i + NUMPIXELS) % NUMPIXELS;
      uint32_t color = aztecPalette[i % paletteSize];
      pixels.setPixelColor(idx, color);
    }
    pixels.show();
    if (smartDelay(wait)) return;
    head = (head + 1) % NUMPIXELS;
  }
}

void drawImage(byte image[5], uint32_t color, int wait = 0) {
  pixels.clear();
  for (int y = 0; y < 5; y++) {
    for (int x = 0; x < 5; x++) {
      bool on = bitRead(image[y], 4 - x);
      int idx = y * 5 + x;
      if (on) {
        pixels.setPixelColor(idx, color);
      }
    }
  }
  pixels.show();
  if (smartDelay(wait)) return;
}

byte smiley[5] = {
  0b00000,
  0b01010,
  0b00000,
  0b10001,
  0b01110
};

byte heart[5] = {
  0b01010,
  0b11111,
  0b11111,
  0b01110,
  0b00100
};

byte arrow[5] = {
  0b00100,
  0b00010,
  0b11111,
  0b00010,
  0b00100
};

void imageGalleryDemo() {
  drawImage(smiley, pixels.Color(255, 255, 0), 1000);
  drawImage(heart, pixels.Color(255, 0, 0), 1000);
  drawImage(arrow, pixels.Color(0, 255, 0), 1000);
}