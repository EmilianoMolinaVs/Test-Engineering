
/*
 ================================================================================
                    FIRMWARE DE PRUEBA - TOUCHDOT UE0072
 ================================================================================
 Descripción: Firmware de prueba para validación de GPIOs internos y externos,
              comunicación serie JSON, control de LED neopixel y pantalla OLED.
 
 Funcionalidades:
   - Comunicación serie con protocolo JSON
   - Prueba de integridad de GPIOs internos (pares D15-D17, D19-D27, etc)
   - Prueba de integridad de GPIOs externos (pares PIN_1-PIN_2, etc)
   - Control de LED RGB Neopixel
   - Visualización en pantalla OLED 128x64
   - Identificación de dispositivo por dirección MAC
 
 Comandos JSON soportados:
   1. {"Function":"ping"}     - Prueba de conexión
   2. {"Function":"mac"}      - Obtener dirección MAC
   3. {"Function":"gpioIn"}   - Prueba de GPIOs internos
   4. {"Function":"gpioOut"}  - Prueba de GPIOs externos
 
 Autor: [Nombre del equipo]
 Fecha: 2026-03-03
 Versión: 1.0
 ================================================================================
*/

// ======================== LIBRERÍAS ========================
#include <Wire.h>               // Comunicación I2C para OLED
#include <HardwareSerial.h>     // UART para comunicación serie
#include <ArduinoJson.h>        // Procesamiento de JSON
#include <Arduino.h>            // Funciones base de Arduino
#include <Adafruit_SSD1306.h>   // Control de pantalla OLED
#include <Adafruit_NeoPixel.h>  // Control de LED Neopixel RGB

#ifdef __AVR__
#include <avr/power.h>  // Optimización de potencia para Trinket
#endif

// ======================== CONFIGURACIÓN DE PINES ========================
// Pines I2C para pantalla OLED
#define PIN_SCL 6  // Pin de reloj (Serial Clock) para OLED
#define PIN_SDA 5  // Pin de datos (Serial Data) para OLED

// Pin del LED Neopixel RGB
#define PIN_NEOPIXEL 45  // GPIO para control del neopixel

// Pines de entrada (botón)
#define BUTTON_PIN 0  // Botón de control

// Pines GPIO externos PCB (Conectores exteriores)
#define PIN_1 1  // Grupo Externo 1
#define PIN_2 2
#define PIN_3 3  // Grupo Externo 2
#define PIN_4 4
#define PIN_8 8  // Grupo Externo 3
#define PIN_9 9
#define PIN_10 10  // Grupo Externo 4
#define PIN_11 11
#define PIN_17 17  // Grupo Externo 5
#define PIN_18 18

// Pines GPIO internos PCB (Conectores internos)
#define PIN_D15 D15  // Grupo Interno 1
#define PIN_D17 D17
#define PIN_D19 D19  // Grupo Interno 2
#define PIN_D27 D27
#define PIN_D16 D16  // Grupo Interno 3
#define PIN_D18 D18
#define PIN_D20 D20  // Grupo Interno 4
#define PIN_D28 D28

// ======================== CONSTANTES DE PANTALLA OLED ========================
#define OLED_RESET -1     // Pin de reset (-1 si comparte reset del MCU)
#define SCREEN_WIDTH 128  // Ancho de pantalla OLED en píxeles
#define SCREEN_HEIGHT 64  // Alto de pantalla OLED en píxeles

// ======================== OBJETOS GLOBALES ========================
Adafruit_NeoPixel pixels(1, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);           // LED RGB
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);  // OLED

// ======================== VARIABLES GLOBALES ========================
String status_OLED;  // Estado de conexión con pantalla OLED ("OK" / "FAIL")

// Variables para manejo de comunicación JSON
String JSON_entrada;                  // JSON recibido desde interfaz web
StaticJsonDocument<200> receiveJSON;  // Buffer para deserialización de entrada

String JSON_salida;                // JSON a enviar hacia interfaz web
StaticJsonDocument<200> sendJSON;  // Buffer para serialización de salida

// Tabla de colores para el LED Neopixel (formato GRB)
uint32_t colors[] = {
  pixels.Color(128, 0, 0),      // Rojo
  pixels.Color(0, 128, 0),      // Verde
  pixels.Color(0, 0, 128),      // Azul
  pixels.Color(128, 128, 0),    // Amarillo
  pixels.Color(0, 128, 128),    // Cian
  pixels.Color(128, 0, 128),    // Magenta
  pixels.Color(128, 128, 128),  // Blanco
  pixels.Color(64, 64, 0),      // Naranja oscuro
  pixels.Color(0, 64, 64),      // Teal oscuro
  pixels.Color(64, 0, 64),      // Púrpura
  pixels.Color(0, 0, 0)         // Negro (apagado)
};

int colorIndex = 0;  // Índice actual del color activo en el array


// ======================== FUNCIÓN DE INICIALIZACIÓN ========================
/*
 * setup(): Inicializa todos los periféricos del sistema
 * - Comunicación serie a 115200 baud
 * - LED Neopixel RGB
 * - Pantalla OLED por I2C
 * - GPIOs para botón y LED interno
 */
void setup() {

  // Inicialización de comunicación UART serie
  Serial.begin(115200);
  delay(500);  // Espera a que la comunicación se estabilice
  //Serial.println("\n=== Sistema TouchDot Iniciado ===");
  //Serial.println("Serial a 115200 baud...");

  // Inicialización del LED Neopixel RGB
  pixels.begin();
  pixels.clear();  // Apaga todos los LEDs
  pixels.show();
  //Serial.println("✓ Neopixel inicializado");

  // Inicialización de bus I2C para pantalla OLED
  Wire.begin(PIN_SDA, PIN_SCL);
  delay(100);

  // Detección e inicialización de pantalla OLED
  if (!i2cCheckDevice(0x3C)) {
    //Serial.println("✗ ERROR: Pantalla SSD1306 no detectada en I2C");
    status_OLED = "FAIL";
  } else {
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    status_OLED = "OK";
    //Serial.println("✓ Pantalla OLED inicializada en dirección 0x3C");
  }

  // Mostrar mensaje de bienvenida en OLED
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, 0);
  display.println(F("Test de Prueba"));
  display.setCursor(30, 10);
  display.println(F("TouchDot"));
  display.setCursor(10, 25);
  display.println(F("Status OLED: "));
  display.println(status_OLED);
  display.display();

  // Inicialización de GPIOs
  pinMode(LED_BUILTIN, OUTPUT);       // LED interno del MCU
  pinMode(BUTTON_PIN, INPUT_PULLUP);  // Botón con resistencia interna de pull-up

  //Serial.println("✓ GPIOs configurados");
  //Serial.println("=== Sistema listo para pruebas ===\n");
}




// ======================== FUNCIÓN PRINCIPAL ========================
/*
 * loop(): Bucle principal de ejecución
 * - Gestiona entrada del botón físico
 * - Procesa comandos JSON por puerto serie
 * - Ejecuta demostración de LED si no hay comandos
 */
void loop() {

  // Lectura del estado físico del botón
  bool currentButton = digitalRead(BUTTON_PIN);

  // Si el botón está presionado (lógica activa baja), cambiar color del LED
  if (currentButton == LOW) {
    colorIndex = (colorIndex + 1) % (sizeof(colors) / sizeof(colors[0]));
    pixels.setPixelColor(0, colors[colorIndex]);
    pixels.show();
    delay(1000);  // Debounce y visualización
  }

  // ========== PROCESAMIENTO DE COMANDOS JSON ==========
  if (Serial.available()) {

    // Lectura de JSON desde puerto serie (hasta encontrar salto de línea)
    JSON_entrada = Serial.readStringUntil('\n');

    // Intento de deserialización del JSON
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);

    if (!error) {

      // Extracción del campo "Function" del JSON
      String Function = receiveJSON["Function"];

      // Variables de control para seleccionar comando
      int opc = 0;

      // Mapeo de comandos a opciones numeradas
      if (Function == "ping") opc = 1;          // {"Function":"ping"}
      else if (Function == "mac") opc = 2;      // {"Function":"mac"}
      else if (Function == "gpioIn") opc = 3;   // {"Function":"gpioIn"}
      else if (Function == "gpioOut") opc = 4;  // {"Function":"gpioOut"}
      else if (Function == "testAll") opc = 5;  // {"Function":"testAll"}

      // Procesamiento según comando recibido
      switch (opc) {

        // ===== COMANDO 1: PING - Prueba de conexión =====
        case 1:
          {
            sendJSON.clear();
            sendJSON["ping"] = "pong";
            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }

        // ===== COMANDO 2: MAC - Obtener dirección MAC del dispositivo =====
        case 2:
          {
            sendJSON.clear();

            // Lectura del identificador único (MAC) del chip ESP32
            uint64_t chipid = ESP.getEfuseMac();

            // Formateo de MAC en formato legible XX:XX:XX:XX:XX:XX
            char macStr[18];
            sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
                    (uint8_t)(chipid >> 40),
                    (uint8_t)(chipid >> 32),
                    (uint8_t)(chipid >> 24),
                    (uint8_t)(chipid >> 16),
                    (uint8_t)(chipid >> 8),
                    (uint8_t)(chipid));

            sendJSON["mac"] = macStr;

            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }

        // ===== COMANDO 3: GPIO IN - Prueba de GPIOs internos =====
        case 3:
          {
            sendJSON.clear();

            // Prueba de 4 grupos de pines internos
            // Cada grupo consiste en un par de pines que se prueban bidireccionales
            String state1 = testGpios(PIN_D15, PIN_D17);
            sendJSON["Int1"] = state1;

            String state2 = testGpios(PIN_D19, PIN_D27);
            sendJSON["Int2"] = state2;

            String state3 = testGpios(PIN_D16, PIN_D18);
            sendJSON["Int3"] = state3;

            String state4 = testGpios(PIN_D20, PIN_D28);
            sendJSON["Int4"] = state4;

            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }

        // ===== COMANDO 4: GPIO OUT - Prueba de GPIOs externos =====
        case 4:
          {
            sendJSON.clear();

            // Prueba de 5 grupos de pines externos
            String state1 = testGpios(PIN_1, PIN_2);
            sendJSON["Ext1"] = state1;

            String state2 = testGpios(PIN_3, PIN_4);
            sendJSON["Ext2"] = state2;

            String state3 = testGpios(PIN_8, PIN_9);
            sendJSON["Ext3"] = state3;

            String state4 = testGpios(PIN_10, PIN_11);
            sendJSON["Ext4"] = state4;

            String state5 = testGpios(PIN_17, PIN_18);
            sendJSON["Ext5"] = state5;

            serializeJson(sendJSON, Serial);
            Serial.println();
            break;
          }


        case 5:
          {
            sendJSON.clear();

            String state1 = testGpios(PIN_D15, PIN_D17);
            sendJSON["Int1"] = state1;
            String state2 = testGpios(PIN_D19, PIN_D27);
            sendJSON["Int2"] = state2;
            String state3 = testGpios(PIN_D16, PIN_D18);
            sendJSON["Int3"] = state3;
            String state4 = testGpios(PIN_D20, PIN_D28);
            sendJSON["Int4"] = state4;

            String state5 = testGpios(PIN_1, PIN_2);
            sendJSON["Ext1"] = state5;
            String state6 = testGpios(PIN_3, PIN_4);
            sendJSON["Ext2"] = state6;
            String state7 = testGpios(PIN_8, PIN_9);
            sendJSON["Ext3"] = state7;
            String state8 = testGpios(PIN_10, PIN_11);
            sendJSON["Ext4"] = state8;
            String state9 = testGpios(PIN_17, PIN_18);
            sendJSON["Ext5"] = state9;

            serializeJson(sendJSON, Serial);
            Serial.println();
            break;

            break;
          }

        // Comando no reconocido
        default:
          Serial.println("{\"error\": \"Función no soportada\"}");
          break;
      }
    } else {
      // Error en deserialización JSON
      Serial.print("{\"error\": \"JSON inválido: ");
      Serial.print(error.c_str());
      Serial.println("\"}");
    }

  } else {
    // Si no hay datos en puerto serie, ejecutar demostración de LED
    ledDemo();
  }
}

// ======================== FUNCIONES AUXILIARES ========================

/*
 * i2cCheckDevice(uint8_t address): Verifica si existe un dispositivo I2C
 * Parámetros:
 *   - address: Dirección I2C del dispositivo a verificar (ej: 0x3C)
 * Retorna:
 *   - true si el dispositivo responde
 *   - false si no hay respuesta (dispositivo no encontrado)
 */
bool i2cCheckDevice(uint8_t address) {
  Wire.beginTransmission(address);
  byte error = Wire.endTransmission();
  return (error == 0);  // 0 = dispositivo encontrado
}

/*
 * testGpios(uint8_t gpioA, uint8_t gpioB): Prueba integridad bidireccional
 * Realiza pruebas en ambas direcciones: A->B y B->A
 * 
 * Parámetros:
 *   - gpioA: Primer pin GPIO a probar
 *   - gpioB: Segundo pin GPIO a probar
 * Retorna:
 *   - "OK" si ambas pruebas son exitosas
 *   - "Fail" si hay fallos en cualquier dirección
 */
String testGpios(uint8_t gpioA, uint8_t gpioB) {

  // Prueba A -> B
  bool resultAB = testSequence(gpioA, gpioB);
  if (!resultAB) return "Fail";

  delay(10);  // Pausa entre pruebas

  // Prueba B -> A (bidireccional)
  bool resultBA = testSequence(gpioB, gpioA);
  if (!resultBA) return "Fail";

  return "OK";  // Ambas pruebas correctas
}

/*
 * testSequence(uint8_t gpioOut, uint8_t gpioIn): Prueba secuencia de bit
 * Envía un patrón de bits a través de gpioOut y verifica lectura en gpioIn
 * 
 * Parámetros:
 *   - gpioOut: Pin configurado como salida
 *   - gpioIn: Pin configurado como entrada
 * Retorna:
 *   - true si todos los bits coinciden
 *   - false si hay algún error de correspondencia
 */
bool testSequence(uint8_t gpioOut, uint8_t gpioIn) {

  // Configuración de pines
  pinMode(gpioOut, OUTPUT);
  pinMode(gpioIn, INPUT_PULLDOWN);  // Configuración con resistencia pull-down

  // Patrón de prueba: secuencia de bits con patrones de repetición
  uint8_t testPattern[] = { 1, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1, 1 };

  // Envío y verificación de cada bit del patrón
  for (int i = 0; i < sizeof(testPattern); i++) {

    digitalWrite(gpioOut, testPattern[i]);  // Envía bit
    delay(10);                              // Espera de estabilización

    int readValue = digitalRead(gpioIn);  // Lee bit

    // Si la lectura no coincide con lo enviado, fallido
    if (readValue != testPattern[i]) {
      return false;
    }
  }

  return true;  // Todos los bits coincidieron correctamente
}

/*
 * ledDemo(): Demostración continua del LED RGB
 * Alterna colores cada 100ms entre rojo, verde, azul y magenta
 * Se ejecuta cuando no hay comandos JSON pendientes (idle)
 */
void ledDemo() {
  int tm_delay = 100;  // Intervalo de tiempo entre transiciones

  // Rojo
  digitalWrite(LED_BUILTIN, HIGH);
  pixels.setPixelColor(0, pixels.Color(20, 0, 0));
  pixels.show();
  delay(tm_delay);

  // Verde
  digitalWrite(LED_BUILTIN, LOW);
  pixels.setPixelColor(0, pixels.Color(0, 20, 0));
  pixels.show();
  delay(tm_delay);

  // Azul
  digitalWrite(LED_BUILTIN, HIGH);
  pixels.setPixelColor(0, pixels.Color(0, 0, 20));
  pixels.show();
  delay(tm_delay);

  // Magenta
  digitalWrite(LED_BUILTIN, LOW);
  pixels.setPixelColor(0, pixels.Color(20, 0, 20));
  pixels.show();
  delay(tm_delay);
}
