/* 
===== CÓDIGO DE INTEGRACIÓN CONTROL UE0093 I2C LockNode JSON ====
*/

// --- BIBLIOTECAS ---
#include <Wire.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include <Arduino.h>
#include <MapController.h>

// ==== Declaración de pines
#define RX2 D4  // GPIO15 como RX
#define TX2 D5  // GPIO19 como TX

#define I2C_SDA 22  // Lectura del módulo por I2C
#define I2C_SCL 23

#define RelayNO 4  // Lectura de conmutación en relevador
#define RelayNC 5

#define switchGPIO 3

// ==== Inicialización de objetos
HardwareSerial PagWeb(1);  // Objeto para UART2 en PULSAR como PagWeb

// MapController class with full functionality (MapCRUD alias available for command compatibility)
MapController deviceManager;

// ==== Variables de inicialización JSON
String JSON_entrada;  // Variable que recibe al JSON en crudo de PagWeb
StaticJsonDocument<200> receiveJSON;

String JSON_lectura;  // Variable que envía el JSON de datos
StaticJsonDocument<200> sendJSON;

// Global variables for demo Manager
unsigned long lastTestTime = 0;
bool demoMode = false;
int demoStep = 0;

// Function declarations
void showBanner();
void showMenu();
void processCommand(String cmd);
void runDemo();
void printDeviceStatus();


void setup() {

  // Iniciar UART0 (USB) para depuración
  Serial.begin(115200);
  Serial.println("UART0 listo (USB)");
  delay(1000);

  // Iniciar UART2 en los pines seleccionados
  PagWeb.begin(115200, SERIAL_8N1, RX2, TX2);
  Serial.println("UART2 iniciado en RX=9, TX=8");

  // Declaración de pines de entrada de relevador
  pinMode(RelayNO, INPUT);
  pinMode(RelayNC, INPUT);

  pinMode(switchGPIO, OUTPUT);
  digitalWrite(switchGPIO, HIGH);

  //showBanner();

  deviceManager.begin(Wire, I2C_SDA, I2C_SCL, 20000);  // ESP32: SDA=21, SCL=22, 100kHz
  deviceManager.initializeI2C(1);                      // print=1 to show initialization

  // Initialize device mapping system
  deviceManager.initializeMapping(1);

  // Perform initial scan
  //Serial.println(">> Realizando scan inicial automatico...");
  //deviceManager.scanDevices(1);
}


void loop() {
  /*
  // Handle continuous testing if active
  if (deviceManager.isContinuousTestActive()) {
    deviceManager.runContinuousTest();
    delay(deviceManager.getTestInterval());
  }

  // Run demo mode if active
  if (demoMode) {
    runDemo();
  }
  */

  // Process user commands
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toLowerCase();

    if (cmd.length() > 0) {
      Serial.printf("CMD: %s\n", cmd.c_str());
      processCommand(cmd);
      Serial.print("\nMapController> ");
    }
  }


  if (PagWeb.available()) {

    JSON_entrada = PagWeb.readStringUntil('\n');                              // Leer hasta newline (JSON en crudo)
    DeserializationError error = deserializeJson(receiveJSON, JSON_entrada);  // Deserializa el JSON y guarda la información en datosJSON

    if (!error) {

      String Function = receiveJSON["Function"];   // Function es la variable de interés del JSON
      String addressStr = receiveJSON["Address"];  // "0x40"
      uint8_t address = (uint8_t)strtol(addressStr.c_str(), NULL, 16);

      int opc = 0;

      if (Function == "scan") opc = 1;            // {"Function":"scan"}
      else if (Function == "TestBuz") opc = 2;    //{"Function":"TestBuz", "Address": "0X42"}
      else if (Function == "TestRelay") opc = 3;  //{"Function":"TestRelay", "Address": "0X42"}
      else if (Function == "TestNeo") opc = 4;
      else if (Function == "TestAll") opc = 5;

      switch (opc) {

        case 1:  // Escaneo de dirección I2C
          {
            sendJSON.clear();                                              // Limpia cualquier dato previo
            String resultado = deviceManager.scanDevices(1);               // Todo el String en crudo
            String addrStr = scanDirection(resultado);                     // Dirección 0x
            uint8_t address = (uint8_t)strtol(addrStr.c_str(), NULL, 16);  // Dirección uint

            sendJSON["Ad"] = addrStr;
            sendJSON["status"] = "OK";
            serializeJson(sendJSON, PagWeb);  // Envío de datos por JSON a la PagWeb
            PagWeb.println();
            break;
          }

        case 2:  // Ejecución de prueba única de buzzer
          {
            deviceManager.cmdPWM25(address, 1);
            delay(300);
            deviceManager.cmdPWM50(address, 1);
            delay(300);
            deviceManager.cmdPWM75(address, 1);
            delay(300);
            deviceManager.cmdPWM100(address, 1);
            delay(300);
            deviceManager.cmdPWM75(address, 1);
            delay(300);
            deviceManager.cmdPWM50(address, 1);
            delay(300);
            deviceManager.cmdPWM25(address, 1);
            delay(300);
            deviceManager.cmdPWMOff(address, 1);
            break;
          }

        case 3:  // Ejecución de prueba única de Relay
          {
            deviceManager.cmdRelay(address, true, 1);  // Accionamiento
            delay(100);
            deviceManager.cmdRelay(address, false, 1);
            delay(100);
            deviceManager.cmdRelay(address, true, 1);  // Accionamiento
            delay(100);
            deviceManager.cmdRelay(address, false, 1);
            delay(100);
            deviceManager.cmdRelay(address, true, 1);  // Accionamiento
            delay(100);
            deviceManager.cmdRelay(address, false, 1);
            delay(100);
            break;
          }


        case 4:  // Ejecución de prueba única de Neopixel
          {
            deviceManager.cmdNeoRed(address, 1);
            delay(1000);
            deviceManager.cmdNeoBlue(address, 1);
            delay(1000);
            deviceManager.cmdNeoGreen(address, 1);
            delay(1000);
            deviceManager.cmdNeoWhite(address, 1);
            delay(1000);
            deviceManager.cmdNeoOff(address, 1);
            Serial.end();
            delay(1000);
            Serial.begin(115200);
            break;
          }

        case 5:  // Ejecución de Test Completo
          {
            // Bloque de Buzzer
            deviceManager.cmdPWM25(address, 1);
            delay(300);
            deviceManager.cmdPWM50(address, 1);
            delay(300);
            deviceManager.cmdPWM75(address, 1);
            delay(300);
            deviceManager.cmdPWM100(address, 1);
            delay(300);
            deviceManager.cmdPWM75(address, 1);
            delay(300);
            deviceManager.cmdPWM50(address, 1);
            delay(300);
            deviceManager.cmdPWM25(address, 1);
            delay(300);
            deviceManager.cmdPWMOff(address, 1);

            // Bloque de Neopixel
            deviceManager.cmdNeoRed(address, 1);
            delay(1000);
            deviceManager.cmdNeoBlue(address, 1);
            delay(1000);
            deviceManager.cmdNeoGreen(address, 1);
            delay(1000);
            deviceManager.cmdNeoWhite(address, 1);
            delay(1000);
            deviceManager.cmdNeoOff(address, 1);

            Serial.end();
            delay(1000);
            Serial.begin(115200);

            // Bloque de Relay
            delay(300);
            String status1 = "Fail";
            deviceManager.cmdRelay(address, false, 1);  // Accionamiento
            delay(200);
            float relayNC_init = analogRead(RelayNC);
            float relayNO_init = analogRead(RelayNO);

            // Normalizar
            //int relayNC_init = normalizeAnalog(relayNC_init_raw);
            //int relayNO_init = normalizeAnalog(relayNO_init_raw);

            Serial.println("Valor init NC: " + String(relayNC_init));
            Serial.println("Valor init NO: " + String(relayNO_init));
            Serial.println("");

            deviceManager.cmdRelay(address, true, 1);  // Accionamiento
            delay(200);

            float relayNC_fin = analogRead(RelayNC);
            float relayNO_fin = analogRead(RelayNO);

            // Normalizar
            //int relayNC_fin = normalizeAnalog(relayNC_fin_raw);
            //int relayNO_fin = normalizeAnalog(relayNO_fin_raw);

            Serial.println("Valor fin NC: " + String(relayNC_fin));
            Serial.println("Valor fin NO: " + String(relayNO_fin));
            deviceManager.cmdRelay(address, false, 1);  // Accionamiento

            if (fabs(relayNC_init - relayNC_fin) > 300 && fabs(relayNO_init - relayNO_fin) > 300) {
              status1 = "OK";
            }
            sendJSON["status_relay"] = status1;

            // Bloque de GPIO ADC
            String status2 = "Fail";
            digitalWrite(switchGPIO, HIGH);

            String resultado1 = deviceManager.cmdReadPA4(address, 1);  // Lectura inicial HIGH
            int pos1 = resultado1.lastIndexOf('(');
            int valor1 = resultado1.substring(pos1 + 1, pos1 + 2).toInt();  // valor = 1

            digitalWrite(switchGPIO, LOW);
            delay(500);

            String resultado2 = deviceManager.cmdReadPA4(address, 1);  // Lectura final LOW
            int pos2 = resultado2.lastIndexOf('(');
            int valor2 = resultado2.substring(pos2 + 1, pos2 + 2).toInt();  // valor = 1
            digitalWrite(switchGPIO, HIGH);

            if (valor1 != valor2) {
              status2 = "OK";
            }

            sendJSON["status_gpio"] = status2;
            serializeJson(sendJSON, PagWeb);  // Envío de datos por JSON a la PagWeb
            PagWeb.println();
            break;
          }
      }
    }
  }
  delay(10);
}


/*
int normalizeAnalog(float value) {
  return (value > 200) ? 1 : 0;
}
*/


String scanDirection(String resultado) {
  String address = "";

  int okPos = resultado.indexOf("[OK]");
  if (okPos != -1) {
    // Buscar "0x" SOLO después de la línea [OK]
    int dirPos = resultado.indexOf("0x", okPos);
    if (dirPos != -1) {
      int fin = resultado.indexOf(' ', dirPos);
      address = resultado.substring(dirPos, fin);
    }
  }
  Serial.println("Direccion encontrada: " + address);

  return address;
}



void showBanner() {
  Serial.println("\n===========================================");
  Serial.println("    MAP CONTROLLER - Complete Example     ");
  Serial.println("   I2C Device Manager & Testing System   ");
  Serial.println("  Compatible with PY32F003 v6.0.0_lite  ");
  Serial.println("===========================================");
  Serial.println("Features:");
  Serial.println("✓ Device scanning & compatibility check");
  Serial.println("✓ Continuous testing with statistics");
  Serial.println("✓ I2C address management");
  Serial.println("✓ PWM/Buzzer control + Musical tones 🎵");
  Serial.println("✓ System alerts (SUCCESS/OK/WARN/ALERT) 🔔");
  Serial.println("✓ NeoPixel control");
  Serial.println("✓ Sensor reading (PA4, ADC)");
  Serial.println("✓ Device information & UID");
  Serial.println("✓ Security: Relay commands 0xA0-0xAF ⚠️");
  Serial.println("===========================================\n");
}

void showMenu() {
  Serial.println("\nCOMANDOS DISPONIBLES:");
  Serial.println("===============================================");
  Serial.println("=== GESTION BASICA ===");
  Serial.println("'scan' / 's'        = Escanear dispositivos I2C");
  Serial.println("'list' / 'l'        = Ver dispositivos encontrados");
  Serial.println("'ping 0x20'         = Ping a dispositivo especifico");
  Serial.println("'stats'             = Ver estadisticas detalladas");
  Serial.println("'menu' / 'm'        = Mostrar este menu");
  Serial.println("");
  Serial.println("=== DEMO & TESTING ===");
  Serial.println("'demo'              = Iniciar modo demo automatico");
  Serial.println("'stop-demo'         = Detener modo demo");
  Serial.println("'test-start pa4'    = Test continuo lectura PA4");
  Serial.println("'test-start toggle' = Test continuo toggle");
  Serial.println("'test-start neo'    = Test continuo NeoPixels");
  Serial.println("'test-start pwm'    = Test continuo PWM/buzzer");
  Serial.println("'test-start music'  = 🎵 Test escala musical (v6.0.0)");
  Serial.println("'test-start alerts' = 🔔 Test alertas sistema (v6.0.0)");
  Serial.println("'test-start gradual'= 📈 Test PWM gradual (v6.0.0)");
  Serial.println("'test-stop'         = Detener test continuo");
  Serial.println("'interval 1000'     = Cambiar intervalo test (ms)");
  Serial.println("");
  Serial.println("=== CONTROL DE DISPOSITIVOS ===");
  Serial.println("'relay 0x20 on'     = Encender rele");
  Serial.println("'relay 0x20 off'    = Apagar rele");
  Serial.println("'toggle 0x20'       = Pulso de rele (toggle)");
  Serial.println("'neo 0x20 red'      = NeoPixels rojos");
  Serial.println("'neo 0x20 white'    = NeoPixels blancos");
  Serial.println("'neo 0x20 off'      = Apagar NeoPixels");
  Serial.println("'pwm 0x20 25'       = PWM 25% (200Hz)");
  Serial.println("'pwm 0x20 100'      = PWM 100% (2kHz)");
  Serial.println("'pwm 0x20 off'      = PWM silencio");
  Serial.println("'tone 0x20 do'      = 🎵 Tocar nota DO (261Hz)");
  Serial.println("'tone 0x20 la'      = 🎵 Tocar nota LA (440Hz)");
  Serial.println("'alert 0x20 ok'     = 🔔 Alerta OK (1000Hz)");
  Serial.println("'alert 0x20 warn'   = ⚠️ Alerta WARNING (1200Hz)");
  Serial.println("");
  Serial.println("=== SENSORES ===");
  Serial.println("'pa4 0x20'          = Leer entrada digital PA4");
  Serial.println("'adc 0x20'          = Leer ADC PA0");
  Serial.println("");
  Serial.println("=== INFORMACION ===");
  Serial.println("'info 0x20'         = Informacion del dispositivo");
  Serial.println("'uid 0x20'          = UID unico del dispositivo");
  Serial.println("'test-all 0x20'     = Test completo de comandos");
  Serial.println("'test-both 0x20'    = Test rápido (solo toggle+pa4)");
  Serial.println("'test-both 0x20'    = Test rápido (solo toggle + pa4)");
  Serial.println("");
  Serial.println("=== GESTION I2C ===");
  Serial.println("'change 0x20 0x30'  = Cambiar direccion I2C");
  Serial.println("'factory 0x20'      = Factory reset a UID");
  Serial.println("'reset 0x20'        = Reiniciar dispositivo");
  Serial.println("'status 0x20'       = Estado I2C (Flash/UID)");
  Serial.println("===============================================");
  Serial.println("Ejemplos:");
  Serial.println("  scan               (escanear bus)");
  Serial.println("  demo               (modo demostracion)");
  Serial.println("  test-start toggle  (test automatico)");
  Serial.println("  relay 0x20 on      (encender rele)");
  Serial.println("  neo 0x20 blue      (NeoPixels azules)");
  Serial.println("===============================================");
}

void processCommand(String cmd) {
  // Basic management commands
  if (cmd == "scan" || cmd == "s") {
    deviceManager.scanDevices(1);

  } else if (cmd == "list" || cmd == "l") {
    deviceManager.listDevices(1);

  } else if (cmd == "menu" || cmd == "m") {
    showMenu();

  } else if (cmd == "stats") {
    deviceManager.showStatistics(1);

  } else if (cmd.startsWith("ping ")) {
    String addr_str = cmd.substring(5);
    uint8_t address = deviceManager.parseAddress(addr_str);
    if (address > 0) {
      deviceManager.pingDevice(address, 1);
    }

    // Demo and testing commands
  } else if (cmd == "demo") {
    demoMode = true;
    demoStep = 0;
    Serial.println("[OK] Modo demo iniciado. Usa 'stop-demo' para detener.");

  } else if (cmd == "stop-demo") {
    demoMode = false;
    Serial.println("[OK] Modo demo detenido.");

  } else if (cmd == "test-start pa4") {
    deviceManager.startContinuousTest(MapController::TEST_MODE_PA4_ONLY);
    Serial.println("[OK] Test continuo PA4 iniciado");

  } else if (cmd == "test-start toggle") {
    deviceManager.startContinuousTest(MapController::TEST_MODE_TOGGLE_ONLY);
    Serial.println("[OK] Test continuo toggle iniciado");

  } else if (cmd == "test-start neo") {
    deviceManager.startContinuousTest(MapController::TEST_MODE_NEO_CYCLE);
    Serial.println("[OK] Test continuo NeoPixels iniciado");

  } else if (cmd == "test-start pwm") {
    deviceManager.startContinuousTest(MapController::TEST_MODE_PWM_TONES);
    Serial.println("[OK] Test continuo PWM iniciado");

  } else if (cmd == "test-start music") {
    deviceManager.startContinuousTest(MapController::TEST_MODE_MUSICAL_SCALE);
    Serial.println("[OK] 🎵 Test escala musical DO-SI iniciado (v6.0.0_lite)");

  } else if (cmd == "test-start alerts") {
    deviceManager.startContinuousTest(MapController::TEST_MODE_ALERTS);
    Serial.println("[OK] 🔔 Test alertas sistema iniciado (v6.0.0_lite)");

  } else if (cmd == "test-start gradual") {
    deviceManager.startContinuousTest(MapController::TEST_MODE_PWM_GRADUAL);
    Serial.println("[OK] 📈 Test PWM gradual 100Hz-1000Hz iniciado (v6.0.0_lite)");

  } else if (cmd == "test-stop") {
    deviceManager.stopContinuousTest();
    Serial.println("[OK] Test continuo detenido");

  } else if (cmd.startsWith("interval ")) {
    String interval_str = cmd.substring(9);
    uint32_t new_interval = interval_str.toInt();
    deviceManager.setTestInterval(new_interval);
    Serial.printf("[OK] Intervalo cambiado a %d ms\n", new_interval);

    // Device control commands
  } else if (cmd.startsWith("relay ")) {
    parseRelayCommand(cmd);

  } else if (cmd.startsWith("toggle ")) {
    String addr_str = cmd.substring(7);
    uint8_t address = deviceManager.parseAddress(addr_str);
    if (address > 0) {
      deviceManager.cmdToggle(address, 1);
    }

  } else if (cmd.startsWith("neo ")) {
    parseNeoCommand(cmd);

  } else if (cmd.startsWith("pwm ")) {
    parsePWMCommand(cmd);

  } else if (cmd.startsWith("tone ")) {
    parseToneCommand(cmd);

  } else if (cmd.startsWith("alert ")) {
    parseAlertCommand(cmd);

    // Sensor commands
  } else if (cmd.startsWith("pa4 ")) {
    String addr_str = cmd.substring(4);
    uint8_t address = deviceManager.parseAddress(addr_str);
    if (address > 0) {
      deviceManager.cmdReadPA4(address, 1);
    }

  } else if (cmd.startsWith("adc ")) {
    String addr_str = cmd.substring(4);
    uint8_t address = deviceManager.parseAddress(addr_str);
    if (address > 0) {
      deviceManager.cmdReadADC(address, 1);
    }

    // Information commands
  } else if (cmd.startsWith("info ")) {
    String addr_str = cmd.substring(5);
    uint8_t address = deviceManager.parseAddress(addr_str);
    if (address > 0) {
      deviceManager.showDeviceInfo(address, 1);
    }

  } else if (cmd.startsWith("uid ")) {
    String addr_str = cmd.substring(4);
    uint8_t address = deviceManager.parseAddress(addr_str);
    if (address > 0) {
      deviceManager.showDeviceUID(address, 1);
    }

  } else if (cmd.startsWith("test-all ")) {
    String addr_str = cmd.substring(9);
    uint8_t address = deviceManager.parseAddress(addr_str);
    if (address > 0) {
      deviceManager.testAllCommands(address, 1);
    }

  } else if (cmd.startsWith("test-both ")) {
    String addr_str = cmd.substring(10);
    uint8_t address = deviceManager.parseAddress(addr_str);
    if (address > 0) {
      deviceManager.testBothCommands(address, 1);
    }

  } else if (cmd.startsWith("test-both ")) {
    String addr_str = cmd.substring(10);
    uint8_t address = deviceManager.parseAddress(addr_str);
    if (address > 0) {
      deviceManager.testBothCommands(address, 1);
    }

    // I2C management commands
  } else if (cmd.startsWith("change ")) {
    parseChangeCommand(cmd);

  } else if (cmd.startsWith("factory ")) {
    String addr_str = cmd.substring(8);
    uint8_t address = deviceManager.parseAddress(addr_str);
    if (address > 0) {
      deviceManager.factoryReset(address, 1);
    }

  } else if (cmd.startsWith("reset ")) {
    String addr_str = cmd.substring(6);
    uint8_t address = deviceManager.parseAddress(addr_str);
    if (address > 0) {
      deviceManager.resetDevice(address, 1);
    }

  } else if (cmd.startsWith("status ")) {
    String addr_str = cmd.substring(7);
    uint8_t address = deviceManager.parseAddress(addr_str);
    if (address > 0) {
      deviceManager.checkI2CStatus(address, 1);
    }

  } else if (cmd.length() > 0) {
    Serial.println("ERROR: Comando no reconocido. Usa 'menu' para ver comandos disponibles.");
  }
}

void parseRelayCommand(String cmd) {
  // Parse: "relay 0x20 on/off"
  int first_space = cmd.indexOf(' ');
  int second_space = cmd.indexOf(' ', first_space + 1);

  if (first_space == -1 || second_space == -1) {
    Serial.println("ERROR: Formato: relay <address> on/off");
    return;
  }

  String addr_str = cmd.substring(first_space + 1, second_space);
  String action = cmd.substring(second_space + 1);
  action.toLowerCase();

  uint8_t address = deviceManager.parseAddress(addr_str);
  if (address == 0) return;

  if (action == "on") {
    deviceManager.cmdRelay(address, true, 1);
  } else if (action == "off") {
    deviceManager.cmdRelay(address, false, 1);
  } else {
    Serial.println("ERROR: Usar 'on' o 'off'");
  }
}

void parseNeoCommand(String cmd) {
  // Parse: "neo 0x20 red/green/blue/white/off"
  int first_space = cmd.indexOf(' ');
  int second_space = cmd.indexOf(' ', first_space + 1);

  if (first_space == -1 || second_space == -1) {
    Serial.println("ERROR: Formato: neo <address> red/green/blue/white/off");
    return;
  }

  String addr_str = cmd.substring(first_space + 1, second_space);
  String color = cmd.substring(second_space + 1);
  color.toLowerCase();

  uint8_t address = deviceManager.parseAddress(addr_str);
  if (address == 0) return;

  if (color == "red") {
    deviceManager.cmdNeoRed(address, 1);
  } else if (color == "green") {
    deviceManager.cmdNeoGreen(address, 1);
  } else if (color == "blue") {
    deviceManager.cmdNeoBlue(address, 1);
  } else if (color == "white") {
    deviceManager.cmdNeoWhite(address, 1);
  } else if (color == "off") {
    deviceManager.cmdNeoOff(address, 1);
  } else {
    Serial.println("ERROR: Usar red/green/blue/white/off");
  }
}

void parsePWMCommand(String cmd) {
  // Parse: "pwm 0x20 off/25/50/75/100"
  int first_space = cmd.indexOf(' ');
  int second_space = cmd.indexOf(' ', first_space + 1);

  if (first_space == -1 || second_space == -1) {
    Serial.println("ERROR: Formato: pwm <address> off/25/50/75/100");
    return;
  }

  String addr_str = cmd.substring(first_space + 1, second_space);
  String level = cmd.substring(second_space + 1);
  level.toLowerCase();

  uint8_t address = deviceManager.parseAddress(addr_str);
  if (address == 0) return;

  if (level == "off" || level == "0") {
    deviceManager.cmdPWMOff(address, 1);
  } else if (level == "25") {
    deviceManager.cmdPWM25(address, 1);
  } else if (level == "50") {
    deviceManager.cmdPWM50(address, 1);
  } else if (level == "75") {
    deviceManager.cmdPWM75(address, 1);
  } else if (level == "100") {
    deviceManager.cmdPWM100(address, 1);
  } else {
    Serial.println("ERROR: Usar off/25/50/75/100");
  }
}

void parseToneCommand(String cmd) {
  // Parse: "tone 0x20 do/re/mi/fa/sol/la/si"
  int first_space = cmd.indexOf(' ');
  int second_space = cmd.indexOf(' ', first_space + 1);

  if (first_space == -1 || second_space == -1) {
    Serial.println("ERROR: Formato: tone <address> do/re/mi/fa/sol/la/si");
    return;
  }

  String addr_str = cmd.substring(first_space + 1, second_space);
  String note = cmd.substring(second_space + 1);
  note.toLowerCase();

  uint8_t address = deviceManager.parseAddress(addr_str);
  if (address == 0) return;

  if (note == "do") {
    deviceManager.cmdToneDo(address, 1);
  } else if (note == "re") {
    deviceManager.cmdToneRe(address, 1);
  } else if (note == "mi") {
    deviceManager.cmdToneMi(address, 1);
  } else if (note == "fa") {
    deviceManager.cmdToneFa(address, 1);
  } else if (note == "sol") {
    deviceManager.cmdToneSol(address, 1);
  } else if (note == "la") {
    deviceManager.cmdToneLa(address, 1);
  } else if (note == "si") {
    deviceManager.cmdToneSi(address, 1);
  } else {
    Serial.println("ERROR: Usar do/re/mi/fa/sol/la/si");
  }
}

void parseAlertCommand(String cmd) {
  // Parse: "alert 0x20 success/ok/warn/critical"
  int first_space = cmd.indexOf(' ');
  int second_space = cmd.indexOf(' ', first_space + 1);

  if (first_space == -1 || second_space == -1) {
    Serial.println("ERROR: Formato: alert <address> success/ok/warn/critical");
    return;
  }

  String addr_str = cmd.substring(first_space + 1, second_space);
  String alert = cmd.substring(second_space + 1);
  alert.toLowerCase();

  uint8_t address = deviceManager.parseAddress(addr_str);
  if (address == 0) return;

  if (alert == "success") {
    deviceManager.cmdSuccess(address, 1);
  } else if (alert == "ok") {
    deviceManager.cmdOk(address, 1);
  } else if (alert == "warn" || alert == "warning") {
    deviceManager.cmdWarning(address, 1);
  } else if (alert == "critical" || alert == "alert") {
    deviceManager.cmdAlert(address, 1);
  } else {
    Serial.println("ERROR: Usar success/ok/warn/critical");
  }
}

void parseChangeCommand(String cmd) {
  // Parse: "change 0x20 0x30"
  int first_space = cmd.indexOf(' ');
  int second_space = cmd.indexOf(' ', first_space + 1);

  if (first_space == -1 || second_space == -1) {
    Serial.println("ERROR: Formato: change <old_address> <new_address>");
    return;
  }

  String old_str = cmd.substring(first_space + 1, second_space);
  String new_str = cmd.substring(second_space + 1);

  uint8_t old_addr = deviceManager.parseAddress(old_str);
  uint8_t new_addr = deviceManager.parseAddress(new_str);

  if (old_addr > 0 && new_addr > 0) {
    deviceManager.changeI2CAddress(old_addr, new_addr, 1);
  }
}

void runDemo() {
  static unsigned long lastDemoTime = 0;

  if (millis() - lastDemoTime < 3000) return;  // 3 second intervals
  lastDemoTime = millis();

  if (deviceManager.getDeviceCount() == 0) {
    Serial.println("[DEMO] No hay dispositivos para demostrar");
    demoMode = false;
    return;
  }

  // Get first compatible device
  const MapController::DeviceInfo* devices = deviceManager.getDevices();
  uint8_t demo_addr = 0;

  for (int i = 0; i < deviceManager.getDeviceCount(); i++) {
    if (devices[i].is_compatible) {
      demo_addr = devices[i].address;
      break;
    }
  }

  if (demo_addr == 0) {
    Serial.println("[DEMO] No hay dispositivos compatibles para demostrar");
    demoMode = false;
    return;
  }

  Serial.printf("[DEMO] Paso %d - Dispositivo 0x%02X\n", demoStep + 1, demo_addr);

  switch (demoStep % 8) {
    case 0:
      Serial.println("[DEMO] NeoPixels rojos");
      deviceManager.cmdNeoRed(demo_addr, 1);
      break;
    case 1:
      Serial.println("[DEMO] NeoPixels verdes");
      deviceManager.cmdNeoGreen(demo_addr, 1);
      break;
    case 2:
      Serial.println("[DEMO] NeoPixels azules");
      deviceManager.cmdNeoBlue(demo_addr, 1);
      break;
    case 3:
      Serial.println("[DEMO] NeoPixels blancos");
      deviceManager.cmdNeoWhite(demo_addr, 1);
      break;
    case 4:
      Serial.println("[DEMO] NeoPixels OFF");
      deviceManager.cmdNeoOff(demo_addr, 1);
      break;
    case 5:
      Serial.println("[DEMO] Toggle rele");
      deviceManager.cmdToggle(demo_addr, 1);
      break;
    case 6:
      Serial.println("[DEMO] Lectura PA4");
      deviceManager.cmdReadPA4(demo_addr, 1);
      break;
    case 7:
      Serial.println("[DEMO] Lectura ADC");
      deviceManager.cmdReadADC(demo_addr, 1);
      break;
  }

  demoStep++;
}



void showHelp() {
  Serial.println("\n╔══════════════════════════════════════════════════════════════╗");
  Serial.println("║                    FAST COMMANDS (≤2 chars)                 ║");
  Serial.println("╚══════════════════════════════════════════════════════════════╝");
  Serial.println("⚡ SYSTEM FAST:");
  Serial.println("  sc = scan       m = map         am = automap    h = help");
  Serial.println("  c = clear       r = reboot");
  Serial.println("⚡ ADC FAST:");
  Serial.println("  a1 = adc 1      a2 = adc 2      a3 = adc 3      a4 = adc 4");
  Serial.println("  a5 = adc 5      aa = adc all");
  Serial.println("⚡ RELAY FAST:");
  Serial.println("  f1 = fire 1     f2 = fire 2     f3 = fire 3     f4 = fire 4");
  Serial.println("  f5 = fire 5     ro = relay off all");
  Serial.println("⚡ NEOPIXEL FAST:");
  Serial.println("  nr = red all    ng = green all  nb = blue all   no = off all");
  Serial.println("⚡ FLASH DEBUG FAST (PRIORITY):");
  Serial.println("  fs <id> <data> = flash save     fr <id> = flash read");
  Serial.println("  Example: fs 1 25, fs 1 0x25, fr 1");
  Serial.println("⚡ ARRAY CONTROL FAST (NEW):");
  Serial.println("  sa <n> = sense array n          sh <n> = show array n");
  Serial.println("  fa <n> = fill array n (TEST!)   all = show all arrays");
  Serial.println("  Example: sa 0, sh 1, fa 2, all (n = 0-3)");
  Serial.println("⚡ I2C MANAGEMENT FAST:");
  Serial.println("  si <id> <addr> = set i2c        rf <id> = reset factory");
  Serial.println("  is <id> = i2c status            dr <id> = debug reset");
  Serial.println("  Example: si 1 25, rf 1, is 1, dr 1");

  Serial.println("\n╔══════════════════════════════════════════════════════════════╗");
  Serial.println("║                    STANDARD COMMANDS                        ║");
  Serial.println("╚══════════════════════════════════════════════════════════════╝");
  Serial.println("SYSTEM:");
  Serial.println("  scan, s         - Scan for I2C devices");
  Serial.println("  reboot          - Reboot the ESP32");
  Serial.println("  help, h, ?      - Show this help");
  Serial.println("  clear, cls      - Clear screen");
  Serial.println("\nMAPPING (CRUD):");
  Serial.println("  map, ls         - (List) Show current ID-address mapping");
  Serial.println("  automap         - (Create) Auto-assign scanned devices to free IDs");
  Serial.println("  assign <id> <addr> - (Create/Update) Assign an address to an ID");
  Serial.println("  mv <from> <to>  - (Update) Move/Swap a device from one ID to another");
  Serial.println("  rm <id>         - (Delete) Remove a device from an ID");
  Serial.println("  clear map       - (Delete) Clear all mappings from memory and storage");
  Serial.println("  save            - Save current mapping to persistent storage");
  Serial.println("  load            - Load mapping from persistent storage");
  Serial.println("\nSENSE & RELAY CONTROL (uses ID mapping):");
  Serial.println("  sense           - Read PA4 state from all mapped devices");
  Serial.println("  mask            - Get 4-byte bitmask of PA4 states for mapped devices");
  Serial.println("  sense_array <n> - Get detailed mask for array n (0-3: IDs 1-8, 9-16, 17-24, 25-32)");
  Serial.println("  show_array <n>  - Show detailed view of array n with bit positions");
  Serial.println("  fill_array <n>  - Test fill array n (briefly activates relays - USE WITH CARE!)");
  Serial.println("  show_all        - Overview of all 4 arrays with their current masks");
  Serial.println("  f <id>          - Send momentary relay pulse to a device by ID");
  Serial.println("  t <id>          - Toggle relay state on a device by ID");
  Serial.println("  roff            - Turn off all mapped relays");
  Serial.println("  shut            - Shutdown all mapped devices (LEDs and relays off)");
  Serial.println("\nNEOPIXEL CONTROL (uses ID mapping):");
  Serial.println("  neo red [id]    - Set NeoPixel(s) to red");
  Serial.println("  neo green [id]  - Set NeoPixel(s) to green");
  Serial.println("  neo blue [id]   - Set NeoPixel(s) to blue");
  Serial.println("  neo off [id]    - Turn off NeoPixel(s)");
  Serial.println("  red, green, blue, off - Quick color commands for all");
  Serial.println("\nADC READING (uses ID mapping - 12-bit, 3.3V reference):");
  Serial.println("  adc <id>        - Read ADC value from specific device by ID");
  Serial.println("  adcall, aa      - Read ADC values from all mapped devices");
  Serial.println("  Note: ADC is 12-bit (0-4095) with 3.3V reference, automatically masked");
  Serial.println("\nDEBUG FLASH COMMANDS (NEW):");
  Serial.println("  flash_save <id> <data> - Save data byte to device flash (hex or decimal)");
  Serial.println("  flash_read <id>        - Read data byte from device flash");
  Serial.println("  Examples: flash_save 1 0x25, flash_save 1 37, flash_read 1");
  Serial.println("\nI2C ADDRESS MANAGEMENT (NEW):");
  Serial.println("  set_i2c <id> <addr>    - Change device I2C address (0x08-0x77)");
  Serial.println("  reset_factory <id>     - Reset device to factory I2C address");
  Serial.println("  i2c_status <id>        - Get device I2C configuration status");
  Serial.println("  Examples: set_i2c 1 0x25, reset_factory 1, i2c_status 1");
  Serial.println("  -----Address changes require device restart (~2s)");
  Serial.println("\nDIAGNOSTIC COMMANDS:");
  Serial.println("  testcmds        - Test all command values (0x00-0xFF) to find supported ones");
  Serial.println("════════════════════════════════════════════════════════════════");
}
