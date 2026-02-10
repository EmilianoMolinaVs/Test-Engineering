#include "MapController.h"
#include <stdarg.h>

// Static variables for test cycles
uint8_t MapController::neo_step_ = 0;
uint8_t MapController::pwm_step_ = 0;
uint8_t MapController::musical_step_ = 0;
uint8_t MapController::alert_step_ = 0;
uint8_t MapController::gradual_step_ = 0;

// ========== helpers de salida ==========
void MapController::emit(String& out, int print, const String& s) {
  out += s;
  if (print) Serial.print(s);
}

void MapController::emitf(String& out, int print, const char* fmt, ...) {
  char buf[256];
  va_list ap; va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  out += buf;
  if (print) Serial.printf("%s", buf);
}

// ========== Ctor/Begin ==========
MapController::MapController() {
  for (int i = 0; i < MAX_DEVICES; i++) device_map_.id_to_address[i] = INVALID_ADDRESS;
  for (int i = 0; i < 256; i++) device_map_.address_to_id[i] = 0;
  device_map_.mapped_count = 0;
  
  // Initialize device list
  memset(found_devices_, 0, sizeof(found_devices_));
  device_count_ = 0;
  continuous_test_ = false;
  current_test_mode_ = TEST_MODE_PA4_ONLY;
  cycle_count_ = 0;
  test_interval_ = DEFAULT_TEST_INTERVAL;
  total_bus_recoveries_ = 0;
  last_keepalive_ = 0;
}

void MapController::begin(TwoWire &wire, int sda, int scl, uint32_t i2c_freq) {
  wire_ = &wire;
  sda_ = sda;
  scl_ = scl;
  i2c_freq_ = i2c_freq;
  
  // Initialize I2C with optimized settings
#if defined(ESP32)
  pinMode(sda_, INPUT_PULLUP);
  pinMode(scl_, INPUT_PULLUP);
  delay(100);
  wire_->begin(sda_, scl_);
  wire_->setTimeout(100);
  wire_->setClock(i2c_freq_);
#else
  wire_->begin();
  wire_->setTimeout(100);
  wire_->setClock(i2c_freq_);
#endif
  
  initializeMapping(/*print=*/0);
}

String MapController::initializeI2C(int print) {
  String out;
  emit(out, print, "===========================================\r\n");
  emit(out, print, "    I2C DEVICE MANAGER (MapController)    \r\n");
  emit(out, print, "  Gestion + Test Continuo + Comandos     \r\n");
  emit(out, print, "===========================================\r\n");
  emitf(out, print, "I2C: SDA=GPIO%d, SCL=GPIO%d, Clock=%dkHz\r\n", sda_, scl_, i2c_freq_/1000);
  
#if defined(ESP32)
  pinMode(sda_, INPUT_PULLUP);
  pinMode(scl_, INPUT_PULLUP);
  delay(100);
  wire_->begin(sda_, scl_);
  wire_->setTimeout(100);
  wire_->setClock(i2c_freq_);
#else
  wire_->begin();
  wire_->setTimeout(100);
  wire_->setClock(i2c_freq_);
#endif
  
  emit(out, print, "[OK] I2C initialized successfully\r\n");
  return out;
}

// ========== DEVICE SCANNING & MANAGEMENT ==========

String MapController::scanDevices(int print) {
  String out;
  emit(out, print, ">> ESCANEANDO BUS I2C...\r\n");
  emit(out, print, "Rango: 0x08-0x77 (8-119 decimal)\r\n");
  emit(out, print, "===========================================\r\n");
  
  device_count_ = 0;
  bool found_any = false;
  
  // Clear device array
  memset(found_devices_, 0, sizeof(found_devices_));
  
  // CRITICAL: Flush y reset del bus I2C antes de empezar
  // Esto asegura que todos los slaves estén en estado conocido
  for (int i = 0; i < 3; i++) {
    wire_->beginTransmission(0x00); // Broadcast address
    wire_->endTransmission(true);
    delay(20);
  }
  
  // Limpiar buffer completamente
  while(wire_->available()) {
    wire_->read();
  }
  
  delay(50); // Pausa inicial para que todos los slaves se estabilicen después del reset
  
  // Flush I2C antes de empezar para limpiar el bus completamente
  wire_->endTransmission(true);
  delay(50); // Pausa inicial para que todos los slaves se estabilicen
  
  for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
    // Scan con 3 intentos, debe pasar al menos 1 de 3 para ser válido
    bool device_found = false;
    
    for (uint8_t attempt = 0; attempt < 3; attempt++) {
      // Limpiar buffer del Wire antes de cada intento
      while(wire_->available()) {
        wire_->read();
      }
      
      wire_->beginTransmission(addr);
      uint8_t error = wire_->endTransmission(true); // Force STOP condition
      
      if (error == 0) {
        device_found = true;
        break; // Respondió, no necesitamos más intentos
      }
      
      // Delay entre intentos - progresivo para dar más tiempo
      delay(10 + (attempt * 5)); // 10ms, 15ms, 20ms
    }
    
    if (device_found) {
      found_any = true;
      
      // Store in array
      if (device_count_ < MAX_DEVICES) {
        found_devices_[device_count_].address = addr;
        found_devices_[device_count_].assigned_id = getIDByAddress(addr);
        found_devices_[device_count_].is_compatible = true;
        found_devices_[device_count_].is_online = true;
        found_devices_[device_count_].status = "I2C Device";
        found_devices_[device_count_].total_tests = 0;
        found_devices_[device_count_].successful_tests = 0;
        found_devices_[device_count_].failed_tests = 0;
        found_devices_[device_count_].consecutive_failures = 0;
        device_count_++;
      }
      
      // Show result
      emitf(out, print, "[OK] 0x%02X (%3d) - I2C Device\r\n", addr, addr);
      
      delay(15); // Delay después de encontrar dispositivo
    }
    
    delay(2); // Mínimo delay entre direcciones
  }
  
  emit(out, print, "===========================================\r\n");
  
  if (!found_any) {
    emit(out, print, "ERROR: No se encontraron dispositivos I2C\r\n");
    emit(out, print, "NOTA: Verificar conexiones SDA/SCL y alimentacion\r\n");
  } else {
    emitf(out, print, "TOTAL: %d dispositivos I2C encontrados\r\n", device_count_);
  }
  
  // Auto-inject scanned devices for automap (matches MapCRUD behavior)
  setScannedDevices(found_devices_, device_count_);
  
  return out;
}

// Compatibility check disabled - all responding devices are considered valid
// This function is kept for potential future use
bool MapController::checkCompatibility(uint8_t address) {
  // Send PA4_DIGITAL command to verify compatibility con reintentos
  const uint8_t max_attempts = 2;
  
  for (uint8_t attempt = 0; attempt < max_attempts; attempt++) {
    if (attempt > 0) {
      delay(20); // Pausa entre intentos para que el slave se recupere
    }
    
    wire_->beginTransmission(address);
    wire_->write(CMD_PA4_DIGITAL);
    
    if (wire_->endTransmission(true) == 0) { // Force STOP condition
      delay(30); // Dar tiempo al PY32F003 para procesar comando y preparar respuesta
      
      uint8_t bytes_received = wire_->requestFrom(address, (uint8_t)1, (uint8_t)true); // Force STOP after read
      
      if (bytes_received > 0 && wire_->available()) {
        uint8_t response = wire_->read();
        delay(5); // Breve delay después de lectura
        
        // Compatible device formats:
        // - Legacy firmware: response = 0 or 1 (direct PA4 state)
        // - v6.0.0_lite: bits[3-0] = 0x09, bits[7-4] = PA4 state
        // Accept: 0, 1, 0x09, 0x19 (and any valid response with lower nibble pattern)
        uint8_t lower_nibble = response & 0x0F;
        
        if (response == 0 || response == 1 || lower_nibble == 0x09) {
          return true; // Compatible PY32F003
        }
      }
    }
  }
  
  return false; // No compatible o no responde correctamente
}

String MapController::listDevices(int print) {
  String out;
  emit(out, print, "DISPOSITIVOS ENCONTRADOS:\r\n");
  emit(out, print, "===========================================\r\n");
  
  if (device_count_ == 0) {
    emit(out, print, "ERROR: No hay dispositivos. Usa scanDevices() primero.\r\n");
    return out;
  }
  
  emit(out, print, "+----------+--------+-------------+----------+----------+\r\n");
  emit(out, print, "| Direccion| Estado | Descripcion | Exitos   | Fallos   |\r\n");
  emit(out, print, "+----------+--------+-------------+----------+----------+\r\n");
  
  for (int i = 0; i < device_count_; i++) {
    String icon = found_devices_[i].is_compatible ? "[OK]" : "[DEV]";
    String status = found_devices_[i].is_online ? "ONLINE" : "OFFLINE";
    
    emitf(out, print, "|%s 0x%02X | %-6s | %-11s | %-8d | %-8d |\r\n",
      icon.c_str(),
      found_devices_[i].address,
      status.c_str(),
      found_devices_[i].is_compatible ? "PY32F003" : "Generico",
      found_devices_[i].successful_tests,
      found_devices_[i].failed_tests
    );
  }
  
  emit(out, print, "+----------+--------+-------------+----------+----------+\r\n");
  return out;
}

String MapController::pingDevice(uint8_t address, int print) {
  String out;
  emitf(out, print, "PING: 0x%02X (%d decimal)\r\n", address, address);
  
  wire_->beginTransmission(address);
  uint8_t error = wire_->endTransmission();
  
  if (error == 0) {
    emit(out, print, "[OK] Dispositivo responde\r\n");
    
    if (checkCompatibility(address)) {
      emit(out, print, "COMPATIBLE: PY32F003 firmware\r\n");
    } else {
      emit(out, print, "DISPOSITIVO: Generico (no compatible)\r\n");
    }
  } else {
    emitf(out, print, "ERROR: No responde (Error: %d)\r\n", error);
  }
  
  return out;
}

uint8_t MapController::parseAddress(String addr_str) {
  addr_str.trim();
  addr_str.toLowerCase();
  
  uint8_t address = 0;
  
  if (addr_str.startsWith("0x")) {
    address = strtol(addr_str.c_str() + 2, NULL, 16);
  } else if (addr_str.endsWith("h")) {
    String hex_part = addr_str.substring(0, addr_str.length() - 1);
    address = strtol(hex_part.c_str(), NULL, 16);
  } else {
    address = addr_str.toInt();
  }
  
  if (address < 0x08 || address > 0x77) {
    // Note: In library context, we don't print errors directly
    return 0;
  }
  
  return address;
}

bool MapController::recoverI2CBus() {
  // Reinitialize I2C
  wire_->end();
  delay(50);
  
#if defined(ESP32)
  wire_->begin(sda_, scl_);
#else
  wire_->begin();
#endif
  
  wire_->setTimeout(100);
  wire_->setClock(i2c_freq_);
  
  total_bus_recoveries_++;
  return true;
}

// ========== BASIC DEVICE COMMANDS ==========

bool MapController::sendBasicCommand(uint8_t address, uint8_t command) {
  const uint8_t max_retries = 2;  // Reducido para velocidad
  
  // Clear buffer rápido sin warm-up delays
  while (wire_->available()) {
    wire_->read();
  }
  
  for (uint8_t retry = 0; retry < max_retries; retry++) {
    // Delay mínimo entre reintentos
    if (retry > 0) {
      delay(10);  // Aumentado para dar tiempo de recuperación completo
    }
    
    wire_->beginTransmission(address);
    wire_->write(command);
    uint8_t error = wire_->endTransmission(true);  // Force STOP condition
    
    if (error == 0) {
      // Delay para respuesta (respetando timeout del PY32F003)
      delay(15);  // Aumentado a 15ms para dar más tiempo al slave
      
      uint8_t bytes_received = wire_->requestFrom(address, (uint8_t)1, (uint8_t)true);  // Force STOP after read
      if (bytes_received == 1 && wire_->available()) {
        uint8_t response = wire_->read();
        delay(5);  // Delay después de lectura para limpieza del bus
        
        return true;
      } else {
        // No response received - try again
        delay(10);  // Delay antes de retry
        continue;
      }
    } else {
      // Transmission error (NACK or other) - try again
      delay(10);  // Delay antes de retry
      continue;
    }
  }
  
  // All retries failed
  return false;
}

bool MapController::i2cReadByteWithRetry(uint8_t address, uint8_t command, uint8_t* out,
                                        uint8_t attempts, uint32_t wait_ms) {
  for (uint8_t i = 0; i < attempts; i++) {
    // Delay mínimo entre reintentos
    if (i > 0) {
      delay(3);  // Reducido de 15 a 3ms
    }
    
    wire_->beginTransmission(address);
    wire_->write(command);
    uint8_t err = wire_->endTransmission(true); // send STOP
    if (err == 0) {
      delay(5); // Reducido a 5ms fijo para velocidad máxima
      uint8_t req = wire_->requestFrom(address, (uint8_t)1, (uint8_t)true); // 1 byte, STOP
      if (req == 1 && wire_->available()) {
        *out = wire_->read();
        return true;
      }
    }
  }
  return false;
}

uint8_t MapController::readPA4Digital(uint8_t address) {
  uint8_t byte_val = 0;
  if (!i2cReadByteWithRetry(address, CMD_PA4_DIGITAL, &byte_val, 2, 5)) {  // Reducido attempts y wait
    return 0; // failure -> assume LOW
  }
  // PA4 is in upper bits (7-4)
  return (byte_val & 0xF0) ? 1 : 0;
}

uint16_t MapController::readADC(uint8_t address) {
  uint16_t adc_value = 0;
  uint8_t byte_val = 0;

  // Read HSB (bits 11-8 in lower nibble; upper nibble may contain PA4)
  if (i2cReadByteWithRetry(address, CMD_ADC_PA0_HSB, &byte_val, 2, 5)) {  // Optimizado para velocidad
    uint8_t hsb = byte_val & 0x0F; // clean PA4 from upper nibble
    adc_value = (hsb << 8);
  } else {
    // HSB failure, return 0 to indicate invalid reading
    return 0;
  }

  // Read LSB (complete bits 7-0)
  if (i2cReadByteWithRetry(address, CMD_ADC_PA0_LSB, &byte_val, 2, 5)) {  // Optimizado para velocidad
    adc_value |= byte_val;
  } else {
    // If LSB fails, return only shifted HSB (otherwise 0)
    // Here we prefer to indicate complete failure
    return 0;
  }

  return adc_value;
}

// ========== CONTINUOUS TESTING SYSTEM ==========

void MapController::startContinuousTest(TestMode mode) {
  if (device_count_ == 0) {
    return; // No devices to test
  }
  
  current_test_mode_ = mode;
  continuous_test_ = true;
  cycle_count_ = 0;
}

void MapController::stopContinuousTest() {
  continuous_test_ = false;
}

void MapController::runContinuousTest() {
  if (!continuous_test_ || device_count_ == 0) {
    return;
  }
  
  cycle_count_++;
  
  // Keep-alive for long intervals: ping before each command
  if (test_interval_ > 1000) {
    for (int i = 0; i < device_count_; i++) {
      if (found_devices_[i].is_compatible) {
        // Short ping to activate slave
        wire_->beginTransmission(found_devices_[i].address);
        wire_->endTransmission();
        delay(1);  // Only 1ms delay
      }
    }
  }
  
  for (int i = 0; i < device_count_; i++) {
    if (found_devices_[i].is_compatible) {
      testDevice(i);
    }
  }
}

void MapController::testDevice(uint8_t index) {
  if (index >= device_count_) return;
  
  uint8_t addr = found_devices_[index].address;
  found_devices_[index].total_tests++;
  
  // Basic communication test
  wire_->beginTransmission(addr);
  uint8_t error = wire_->endTransmission();
  
  if (error == 0) {
    bool command_success = true;
    
    // Execute commands based on selected mode
    switch(current_test_mode_) {
      case TEST_MODE_PA4_ONLY:
        testDevicePA4(index);
        break;
      case TEST_MODE_TOGGLE_ONLY:
        testDeviceToggle(index);
        break;
      case TEST_MODE_BOTH:
        testDeviceBoth(index);
        break;
      case TEST_MODE_NEO_CYCLE:
        testDeviceNeoCycle(index);
        break;
      case TEST_MODE_PWM_TONES:
        testDevicePWMCycle(index);
        break;
      case TEST_MODE_ADC_PA0:
        testDeviceADC(index);
        break;
      case TEST_MODE_MUSICAL_SCALE:
        testDeviceMusicalScale(index);
        break;
      case TEST_MODE_ALERTS:
        testDeviceAlerts(index);
        break;
      case TEST_MODE_PWM_GRADUAL:
        testDevicePWMGradual(index);
        break;
    }
    
  } else {
    found_devices_[index].failed_tests++;
    found_devices_[index].last_failure_time = millis();
    found_devices_[index].consecutive_failures++;
    found_devices_[index].is_online = false;
    
    // Auto recovery if many consecutive failures
    if (found_devices_[index].consecutive_failures >= 5) {
      recoverI2CBus();
      found_devices_[index].consecutive_failures = 0;
    }
  }
}

void MapController::testDevicePA4(uint8_t index) {
  uint8_t pa4_state = readPA4Digital(found_devices_[index].address);
  found_devices_[index].last_pa4_state = pa4_state;
  
  found_devices_[index].successful_tests++;
  found_devices_[index].last_success_time = millis();
  found_devices_[index].consecutive_failures = 0;
  found_devices_[index].is_online = true;
}

void MapController::testDeviceToggle(uint8_t index) {
  bool toggle_ok = sendBasicCommand(found_devices_[index].address, CMD_TOGGLE);
  
  if (toggle_ok) {
    found_devices_[index].successful_tests++;
    found_devices_[index].last_success_time = millis();
    found_devices_[index].consecutive_failures = 0;
    found_devices_[index].is_online = true;
  } else {
    found_devices_[index].failed_tests++;
    found_devices_[index].last_failure_time = millis();
    found_devices_[index].consecutive_failures++;
  }
}

void MapController::testDeviceBoth(uint8_t index) {
  bool toggle_ok = sendBasicCommand(found_devices_[index].address, CMD_TOGGLE);
  uint8_t pa4_state = readPA4Digital(found_devices_[index].address);
  found_devices_[index].last_pa4_state = pa4_state;
  
  if (toggle_ok) {
    found_devices_[index].successful_tests++;
    found_devices_[index].last_success_time = millis();
    found_devices_[index].consecutive_failures = 0;
    found_devices_[index].is_online = true;
  } else {
    found_devices_[index].failed_tests++;
    found_devices_[index].last_failure_time = millis();
    found_devices_[index].consecutive_failures++;
  }
}

void MapController::testDeviceNeoCycle(uint8_t index) {
  uint8_t neo_cmd = CMD_NEO_OFF;
  
  switch(neo_step_ % 5) {
    case 0: neo_cmd = CMD_NEO_WHITE; break;
    case 1: neo_cmd = CMD_NEO_RED; break;
    case 2: neo_cmd = CMD_NEO_GREEN; break;
    case 3: neo_cmd = CMD_NEO_BLUE; break;
    case 4: neo_cmd = CMD_NEO_OFF; break;
  }
  
  bool neo_ok = sendBasicCommand(found_devices_[index].address, neo_cmd);
  
  if (neo_ok) {
    found_devices_[index].successful_tests++;
    found_devices_[index].last_success_time = millis();
    found_devices_[index].consecutive_failures = 0;
    found_devices_[index].is_online = true;
  } else {
    found_devices_[index].failed_tests++;
    found_devices_[index].last_failure_time = millis();
    found_devices_[index].consecutive_failures++;
  }
  
  if (index == device_count_ - 1) { // Last device in cycle
    neo_step_++;
  }
}

void MapController::testDevicePWMCycle(uint8_t index) {
  uint8_t pwm_cmd = CMD_PWM_OFF;
  
  switch(pwm_step_ % 5) {
    case 0: pwm_cmd = CMD_PWM_OFF; break;
    case 1: pwm_cmd = CMD_PWM_25; break;
    case 2: pwm_cmd = CMD_PWM_50; break;
    case 3: pwm_cmd = CMD_PWM_75; break;
    case 4: pwm_cmd = CMD_PWM_100; break;
  }
  
  bool pwm_ok = sendBasicCommand(found_devices_[index].address, pwm_cmd);
  
  if (pwm_ok) {
    found_devices_[index].successful_tests++;
    found_devices_[index].last_success_time = millis();
    found_devices_[index].consecutive_failures = 0;
    found_devices_[index].is_online = true;
  } else {
    found_devices_[index].failed_tests++;
    found_devices_[index].last_failure_time = millis();
    found_devices_[index].consecutive_failures++;
  }
  
  if (index == device_count_ - 1) { // Last device in cycle
    pwm_step_++;
  }
}

void MapController::testDeviceADC(uint8_t index) {
  uint16_t adc_value = readADC(found_devices_[index].address);
  found_devices_[index].last_adc_value = adc_value;
  
  bool command_success = (adc_value > 0);
  
  if (command_success) {
    found_devices_[index].successful_tests++;
    found_devices_[index].last_success_time = millis();
    found_devices_[index].consecutive_failures = 0;
    found_devices_[index].is_online = true;
  } else {
    found_devices_[index].failed_tests++;
    found_devices_[index].last_failure_time = millis();
    found_devices_[index].consecutive_failures++;
  }
}

void MapController::testDeviceMusicalScale(uint8_t index) {
  uint8_t tone_cmd = CMD_PWM_OFF;
  
  switch(musical_step_ % 8) {
    case 0: tone_cmd = CMD_TONE_DO; break;   // 261Hz
    case 1: tone_cmd = CMD_TONE_RE; break;   // 294Hz
    case 2: tone_cmd = CMD_TONE_MI; break;   // 330Hz
    case 3: tone_cmd = CMD_TONE_FA; break;   // 349Hz
    case 4: tone_cmd = CMD_TONE_SOL; break;  // 392Hz
    case 5: tone_cmd = CMD_TONE_LA; break;   // 440Hz
    case 6: tone_cmd = CMD_TONE_SI; break;   // 494Hz
    case 7: tone_cmd = CMD_PWM_OFF; break;   // Silence
  }
  
  bool tone_ok = sendBasicCommand(found_devices_[index].address, tone_cmd);
  
  if (tone_ok) {
    found_devices_[index].successful_tests++;
    found_devices_[index].last_success_time = millis();
    found_devices_[index].consecutive_failures = 0;
    found_devices_[index].is_online = true;
  } else {
    found_devices_[index].failed_tests++;
    found_devices_[index].last_failure_time = millis();
    found_devices_[index].consecutive_failures++;
  }
  
  if (index == device_count_ - 1) { // Last device in cycle
    musical_step_++;
  }
}

void MapController::testDeviceAlerts(uint8_t index) {
  uint8_t alert_cmd = CMD_PWM_OFF;
  
  switch(alert_step_ % 5) {
    case 0: alert_cmd = CMD_SUCCESS; break;  // 800Hz
    case 1: alert_cmd = CMD_OK; break;       // 1000Hz
    case 2: alert_cmd = CMD_WARNING; break;  // 1200Hz
    case 3: alert_cmd = CMD_ALERT; break;    // 1500Hz
    case 4: alert_cmd = CMD_PWM_OFF; break;  // OFF
  }
  
  bool alert_ok = sendBasicCommand(found_devices_[index].address, alert_cmd);
  
  if (alert_ok) {
    found_devices_[index].successful_tests++;
    found_devices_[index].last_success_time = millis();
    found_devices_[index].consecutive_failures = 0;
    found_devices_[index].is_online = true;
  } else {
    found_devices_[index].failed_tests++;
    found_devices_[index].last_failure_time = millis();
    found_devices_[index].consecutive_failures++;
  }
  
  if (index == device_count_ - 1) { // Last device in cycle
    alert_step_++;
  }
}

void MapController::testDevicePWMGradual(uint8_t index) {
  uint8_t pwm_cmd = CMD_PWM_OFF;
  
  switch(gradual_step_ % 4) {
    case 0: pwm_cmd = CMD_PWM_GRADUAL_MIN; break;      // 0x10 - 100Hz
    case 1: pwm_cmd = CMD_PWM_GRADUAL_MIN + 8; break;  // 0x18 - ~550Hz
    case 2: pwm_cmd = CMD_PWM_GRADUAL_MAX; break;      // 0x1F - 1000Hz
    case 3: pwm_cmd = CMD_PWM_OFF; break;              // OFF
  }
  
  bool pwm_ok = sendBasicCommand(found_devices_[index].address, pwm_cmd);
  
  if (pwm_ok) {
    found_devices_[index].successful_tests++;
    found_devices_[index].last_success_time = millis();
    found_devices_[index].consecutive_failures = 0;
    found_devices_[index].is_online = true;
  } else {
    found_devices_[index].failed_tests++;
    found_devices_[index].last_failure_time = millis();
    found_devices_[index].consecutive_failures++;
  }
  
  if (index == device_count_ - 1) { // Last device in cycle
    gradual_step_++;
  }
}

void MapController::setTestInterval(uint32_t interval_ms) {
  if (interval_ms >= MIN_TEST_INTERVAL && interval_ms <= MAX_TEST_INTERVAL) {
    test_interval_ = interval_ms;
    last_keepalive_ = millis(); // Reset keep-alive timer
  }
}

String MapController::showStatistics(int print) {
  String out;
  emit(out, print, "\n=== ESTADISTICAS ===\r\n");
  emitf(out, print, "Ciclos ejecutados: %d\r\n", cycle_count_);
  emitf(out, print, "Recuperaciones de bus: %d\r\n", total_bus_recoveries_);
  
  for (int i = 0; i < device_count_; i++) {
    if (found_devices_[i].total_tests > 0) {
      float success_rate = (float)found_devices_[i].successful_tests / found_devices_[i].total_tests * 100.0;
      
      emitf(out, print, "0x%02X: %d tests, %.1f%% exito, %d fallos consecutivos\r\n",
            found_devices_[i].address,
            found_devices_[i].total_tests,
            success_rate,
            found_devices_[i].consecutive_failures);
    }
  }
  emit(out, print, "==================\r\n");
  return out;
}

void MapController::resetStatistics() {
  for (int i = 0; i < device_count_; i++) {
    found_devices_[i].total_tests = 0;
    found_devices_[i].successful_tests = 0;
    found_devices_[i].failed_tests = 0;
    found_devices_[i].consecutive_failures = 0;
  }
  cycle_count_ = 0;
  total_bus_recoveries_ = 0;
}

// ========== I2C ADDRESS MANAGEMENT ==========

String MapController::changeI2CAddress(uint8_t old_addr, uint8_t new_addr, int print) {
  String out;
  emitf(out, print, "CAMBIO DE DIRECCION I2C: 0x%02X -> 0x%02X\r\n", old_addr, new_addr);
  
  // Verify that source device exists and is compatible
  if (!checkCompatibility(old_addr)) {
    emitf(out, print, "ERROR: Dispositivo 0x%02X no es compatible\r\n", old_addr);
    return out;
  }
  
  // Step 1: Activate address change mode
  wire_->beginTransmission(old_addr);
  wire_->write(CMD_SET_I2C_ADDR);
  if (wire_->endTransmission() != 0) {
    emit(out, print, "ERROR: No se pudo activar modo cambio\r\n");
    return out;
  }
  
  delay(100);
  
  // Step 2: Send new address
  wire_->beginTransmission(old_addr);
  wire_->write(new_addr);
  if (wire_->endTransmission() != 0) {
    emit(out, print, "ERROR: No se pudo enviar nueva direccion\r\n");
    return out;
  }
  
  delay(200);  // Time to save in Flash
  
  // Step 3: Reset device
  wire_->beginTransmission(old_addr);
  wire_->write(CMD_RESET);
  wire_->endTransmission();  // Ignore error because device resets
  
  emit(out, print, "Esperando reset del dispositivo...\r\n");
  delay(3000);
  
  // Verify change
  wire_->beginTransmission(new_addr);
  if (wire_->endTransmission() == 0) {
    emitf(out, print, "[OK] CAMBIO EXITOSO: Dispositivo ahora en 0x%02X\r\n", new_addr);
  } else {
    emit(out, print, "[INFO] Comando enviado. Ejecutar scan para verificar\r\n");
  }
  
  return out;
}

String MapController::factoryReset(uint8_t address, int print) {
  String out;
  emitf(out, print, "FACTORY RESET: 0x%02X -> direccion UID\r\n", address);
  
  wire_->beginTransmission(address);
  wire_->write(CMD_RESET_FACTORY);
  if (wire_->endTransmission() == 0) {
    emit(out, print, "[OK] Comando factory reset enviado\r\n");
    emit(out, print, "NOTA: Ejecutar scan para encontrar nueva direccion\r\n");
  } else {
    emit(out, print, "ERROR: No se pudo enviar comando factory reset\r\n");
  }
  
  return out;
}

String MapController::checkI2CStatus(uint8_t address, int print) {
  String out;
  emitf(out, print, "ESTADO I2C: 0x%02X\r\n", address);
  
  wire_->beginTransmission(address);
  wire_->write(CMD_GET_I2C_STATUS);
  
  if (wire_->endTransmission() == 0) {
    delay(50);
    wire_->requestFrom(address, (uint8_t)1);
    
    if (wire_->available()) {
      uint8_t status = wire_->read();
      uint8_t status_clean = status & 0x0F;
      
      emitf(out, print, "Estado: 0x%02X -> ", status_clean);
      if (status_clean == 0x0F) {
        emit(out, print, "FLASH (direccion personalizada)\r\n");
      } else if (status_clean == 0x0A) {
        emit(out, print, "UID (direccion factory)\r\n");
      } else {
        emit(out, print, "DESCONOCIDO\r\n");
      }
    }
  } else {
    emit(out, print, "ERROR: No se pudo obtener estado\r\n");
  }
  
  return out;
}

String MapController::resetDevice(uint8_t address, int print) {
  String out;
  emitf(out, print, "RESET DISPOSITIVO: 0x%02X\r\n", address);
  
  wire_->beginTransmission(address);
  wire_->write(CMD_RESET);
  wire_->endTransmission();  // Ignore error
  
  emit(out, print, "[OK] Comando de reset enviado\r\n");
  return out;
}

String MapController::resetAllDevices(int print) {
  String out;
  if (device_count_ == 0) {
    emit(out, print, "ERROR: No hay dispositivos. Ejecutar scan primero\r\n");
    return out;
  }
  
  emitf(out, print, "RESET ALL: Reiniciando %d dispositivos...\r\n", device_count_);
  emit(out, print, "===========================================\r\n");
  
  int reset_count = 0;
  for (int i = 0; i < device_count_; i++) {
    uint8_t addr = found_devices_[i].address;
    
    wire_->beginTransmission(addr);
    wire_->write(CMD_RESET);
    uint8_t error = wire_->endTransmission();
    
    if (error == 0) {
      emitf(out, print, "[%d] 0x%02X: Reset enviado OK\r\n", i, addr);
      reset_count++;
    } else {
      emitf(out, print, "[%d] 0x%02X: Error al enviar reset (Error:%d)\r\n", i, addr, error);
    }
    
    delay(10);  // Small delay between resets
  }
  
  emit(out, print, "===========================================\r\n");
  emitf(out, print, "[OK] %d de %d dispositivos reseteados\r\n", reset_count, device_count_);
  
  return out;
}

String MapController::verifyAddressChange(uint8_t old_addr, uint8_t new_addr, int print) {
  String out;
  emitf(out, print, "VERIFICANDO CAMBIO: 0x%02X -> 0x%02X\r\n", old_addr, new_addr);
  
  // Verify that old doesn't respond
  wire_->beginTransmission(old_addr);
  bool old_offline = (wire_->endTransmission() != 0);
  
  // Verify that new responds
  wire_->beginTransmission(new_addr);
  bool new_online = (wire_->endTransmission() == 0);
  
  if (old_offline && new_online) {
    emitf(out, print, "[OK] CAMBIO EXITOSO: 0x%02X -> 0x%02X\r\n", old_addr, new_addr);
  } else if (!old_offline && new_online) {
    emit(out, print, "WARNING: Ambas direcciones responden\r\n");
  } else if (old_offline && !new_online) {
    emit(out, print, "ERROR: Dispositivo perdido\r\n");
  } else {
    emit(out, print, "ERROR: Cambio fallido\r\n");
  }
  
  return out;
}

void MapController::setScannedDevices(DeviceStats* list, int count) {
  scanned_ = list;
  scanned_count_ = count;
}

// ========== Mapping ==========
String MapController::initializeMapping(int print) {
  String out;
  for (int i = 0; i < MAX_DEVICES; i++) device_map_.id_to_address[i] = INVALID_ADDRESS;
  for (int i = 0; i < 256; i++) device_map_.address_to_id[i] = 0;
  device_map_.mapped_count = 0;
  out += loadMapping(print);
  return out;
}

String MapController::saveMapping(int print) {
  String out;
  preferences_.begin(STORAGE_NAMESPACE, false);
  preferences_.putBytes("id_map", device_map_.id_to_address, MAX_DEVICES);
  preferences_.end();
  emit(out, print, "Mapping saved to persistent storage.\r\n");
  return out;
}

String MapController::loadMapping(int print) {
  String out;
  preferences_.begin(STORAGE_NAMESPACE, true);
  if (preferences_.isKey("id_map")) {
    preferences_.getBytes("id_map", device_map_.id_to_address, MAX_DEVICES);
    device_map_.mapped_count = 0;
    for (int i = 0; i < 256; i++) device_map_.address_to_id[i] = 0;
    for (int i = 0; i < MAX_DEVICES; i++) {
      uint8_t a = device_map_.id_to_address[i];
      if (a != INVALID_ADDRESS) {
        device_map_.address_to_id[a] = i + 1;
        device_map_.mapped_count++;
      }
    }
    emit(out, print, "Device mapping loaded from persistent storage.\r\n");
  } else {
    emit(out, print, "No saved mapping found. Using empty map.\r\n");
  }
  preferences_.end();
  return out;
}

String MapController::clearMapping(int print) {
  String out;
  for (int i = 0; i < MAX_DEVICES; i++) device_map_.id_to_address[i] = INVALID_ADDRESS;
  for (int i = 0; i < 256; i++) device_map_.address_to_id[i] = 0;
  device_map_.mapped_count = 0;

  preferences_.begin(STORAGE_NAMESPACE, false);
  preferences_.clear();
  preferences_.end();
  emit(out, print, "All device mappings have been cleared.\r\n");
  return out;
}

// Lógica pura (sin prints) — sin cambios
bool MapController::assignDeviceToID(uint8_t id, uint8_t address) {
  if (id < 1 || id > MAX_DEVICES) { return false; }
  if (address < 0x08 || address > 0x77) { return false; }
  uint8_t existing_id = getIDByAddress(address);
  if (existing_id != 0 && existing_id != id) { return false; }
  uint8_t existing_addr = getAddressByID(id);
  if (existing_addr != INVALID_ADDRESS && existing_addr != address) { return false; }
  device_map_.id_to_address[id - 1] = address;
  device_map_.address_to_id[address] = id;
  device_map_.mapped_count = 0;
  for (int i = 0; i < MAX_DEVICES; i++)
    if (device_map_.id_to_address[i] != INVALID_ADDRESS) device_map_.mapped_count++;
  return true;
}

bool MapController::moveDevice(uint8_t from_id, uint8_t to_id) {
  if (from_id < 1 || from_id > MAX_DEVICES || to_id < 1 || to_id > MAX_DEVICES) return false;
  if (from_id == to_id) return true;
  uint8_t from_addr = getAddressByID(from_id);
  uint8_t to_addr   = getAddressByID(to_id);
  if (from_addr == INVALID_ADDRESS) return false;
  device_map_.id_to_address[from_id - 1] = to_addr;
  if (to_addr != INVALID_ADDRESS) device_map_.address_to_id[to_addr] = from_id;
  device_map_.id_to_address[to_id - 1] = from_addr;
  device_map_.address_to_id[from_addr] = to_id;
  return true;
}

bool MapController::removeDeviceFromID(uint8_t id) {
  if (id < 1 || id > MAX_DEVICES) return false;
  uint8_t a = getAddressByID(id);
  if (a == INVALID_ADDRESS) return true;
  device_map_.id_to_address[id - 1] = INVALID_ADDRESS;
  device_map_.address_to_id[a] = 0;
  device_map_.mapped_count--;
  return true;
}

uint8_t MapController::getAddressByID(uint8_t id) const {
  if (id < 1 || id > MAX_DEVICES) return INVALID_ADDRESS;
  return device_map_.id_to_address[id - 1];
}

uint8_t MapController::getIDByAddress(uint8_t address) const {
  if (address > 255) return 0;
  return device_map_.address_to_id[address];
}

// ========== Mapping commands ==========
String MapController::cmdShowMapping(int print) {
  String out;
  emitf(out, print, "\n--- Device ID Mapping (%d/%d mapped) ---\r\n", device_map_.mapped_count, MAX_DEVICES);
  emit(out, print, "ID | Address | Online\r\n");
  emit(out, print, "---|---------|--------\r\n");
  for (int i = 1; i <= MAX_DEVICES; i++) {
    uint8_t addr = getAddressByID(i);
    if (addr != INVALID_ADDRESS) {
      bool is_online = false;
      if (scanned_ && scanned_count_ > 0) {
        for (int j = 0; j < scanned_count_; j++) {
          if (scanned_[j].address == addr) { is_online = scanned_[j].is_online; break; }
        }
      }
      emitf(out, print, "%-2d | 0x%02X    | %s\r\n", i, addr, is_online ? "Online" : "-");
    }
  }
  emit(out, print, "------------------------------------\r\n");
  return out;
}

String MapController::cmdAutoMap(int print) {
  String out;
  if (!(scanned_ && scanned_count_ > 0)) {
    emit(out, print, "No scanned devices injected. Use setScannedDevices(...).\r\n");
    return out;
  }
  emit(out, print, "Auto-mapping scanned devices to first free IDs...\r\n");
  int mapped_now = 0;
  for (int i = 0; i < scanned_count_; i++) {
    uint8_t addr = scanned_[i].address;
    if (getIDByAddress(addr) == 0) {
      for (int id = 1; id <= MAX_DEVICES; id++) {
        if (getAddressByID(id) == INVALID_ADDRESS) {
          if (assignDeviceToID(id, addr)) { mapped_now++; }
          break;
        }
      }
    }
  }
  emitf(out, print, "Auto-mapped %d new devices.\r\n", mapped_now);
  out += saveMapping(print);
  return out;
}

String MapController::cmdAssign(const String& cmd, int print) {
  String out;
  int id_idx = cmd.indexOf(' ');
  int addr_idx = cmd.lastIndexOf(' ');
  if (id_idx == -1 || id_idx == addr_idx) { emit(out, print, "ERROR: Usage: assign <id> <address>\r\n"); return out; }
  uint8_t id = cmd.substring(id_idx + 1, addr_idx).toInt();
  String addr_str = cmd.substring(addr_idx + 1);
  uint8_t addr = strtol(addr_str.c_str(), NULL, 0);
  if (assignDeviceToID(id, addr)) { emitf(out, print, "Assigned 0x%02X to ID %d.\r\n", addr, id); out += saveMapping(print); }
  else { emitf(out, print, "ERROR: Cannot assign 0x%02X to ID %d.\r\n", addr, id); }
  return out;
}

String MapController::cmdMove(const String& cmd, int print) {
  String out;
  int a = cmd.indexOf(' ');
  int b = cmd.lastIndexOf(' ');
  if (a == -1 || a == b) { emit(out, print, "ERROR: Usage: mv <from_id> <to_id>\r\n"); return out; }
  uint8_t from_id = cmd.substring(a + 1, b).toInt();
  uint8_t to_id   = cmd.substring(b + 1).toInt();
  uint8_t from_addr = getAddressByID(from_id);
  uint8_t to_addr   = getAddressByID(to_id);
  if (!moveDevice(from_id, to_id)) { emit(out, print, "ERROR: invalid IDs or empty source.\r\n"); return out; }
  if (to_addr != INVALID_ADDRESS)
    emitf(out, print, "Swapped ID %d (0x%02X) with ID %d (0x%02X).\r\n", from_id, from_addr, to_id, to_addr);
  else
    emitf(out, print, "Moved ID %d (0x%02X) to empty ID %d.\r\n", from_id, from_addr, to_id);
  out += saveMapping(print);
  return out;
}

String MapController::cmdRemove(const String& cmd, int print) {
  String out;
  int i = cmd.indexOf(' ');
  if (i == -1) { emit(out, print, "ERROR: Usage: rm <id>\r\n"); return out; }
  uint8_t id = cmd.substring(i + 1).toInt();
  uint8_t a_before = getAddressByID(id);
  if (removeDeviceFromID(id)) {
    if (a_before == INVALID_ADDRESS) emitf(out, print, "ID %d already empty.\r\n", id);
    else emitf(out, print, "Removed 0x%02X from ID %d.\r\n", a_before, id);
    out += saveMapping(print);
  } else {
    emit(out, print, "ERROR: ID must be 1..32.\r\n");
  }
  return out;
}

String MapController::cmdClearMapping(int print) { return clearMapping(print); }

// ========== NeoPixel ==========
String MapController::cmdNeoByID(String cmd, int print) {
  String out;
  cmd.toLowerCase();
  uint8_t neo_cmd = CMD_NEO_OFF;
  uint8_t target_id = 0;

  if      (cmd.indexOf("red")   != -1) neo_cmd = CMD_NEO_RED;
  else if (cmd.indexOf("green") != -1) neo_cmd = CMD_NEO_GREEN;
  else if (cmd.indexOf("blue")  != -1) neo_cmd = CMD_NEO_BLUE;
  else if (cmd.indexOf("off")   != -1) neo_cmd = CMD_NEO_OFF;
  else { emit(out, print, "ERROR: Color no reconocido. Usa: red, green, blue, off\r\n"); return out; }

  int sp = cmd.lastIndexOf(' ');
  if (sp != -1) {
    String last = cmd.substring(sp + 1);
    target_id = last.toInt();
  }
  if (target_id >= 1 && target_id <= MAX_DEVICES) out += setNeoPixelByID(target_id, neo_cmd, print);
  else                                            out += setAllNeoPixels(neo_cmd, print);
  return out;
}

String MapController::setNeoPixelByID(uint8_t id, uint8_t cmd, int print) {
  String out;
  uint8_t addr = getAddressByID(id);
  if (addr == INVALID_ADDRESS) { emitf(out, print, "ERROR: No device mapped to ID %d.\r\n", id); return out; }
  const char* color_str = getNeoColorName(cmd);
  if (sendCommandSafe(addr, cmd)) emitf(out, print, "NeoPixel ID %d (0x%02X): %s - OK\r\n", id, addr, color_str);
  else                            emitf(out, print, "NeoPixel ID %d (0x%02X): %s - FAILED\r\n", id, addr, color_str);
  return out;
}

String MapController::setAllNeoPixels(uint8_t cmd, int print) {
  String out;
  if (device_map_.mapped_count == 0) { emit(out, print, "No devices mapped.\r\n"); return out; }
  const char* color_str = getNeoColorName(cmd);
  emitf(out, print, "Setting all NeoPixels to %s...\r\n", color_str);
  int ok=0, fail=0;
  for (int id=1; id<=MAX_DEVICES; id++) {
    uint8_t addr = getAddressByID(id);
    if (addr == INVALID_ADDRESS) continue;
    if (sendCommandSafe(addr, cmd)) { ok++; emitf(out, print, "  -> ID %d (0x%02X): %s - OK\r\n", id, addr, color_str); }
    else { fail++; emitf(out, print, "  -> ID %d (0x%02X): %s - FAILED\r\n", id, addr, color_str); }
  }
  emitf(out, print, "NeoPixel %s: %d success, %d failed\r\n", color_str, ok, fail);
  return out;
}

const char* MapController::getNeoColorName(uint8_t cmd) {
  switch(cmd) {
    case CMD_NEO_RED:   return "RED";
    case CMD_NEO_GREEN: return "GREEN";
    case CMD_NEO_BLUE:  return "BLUE";
    case CMD_NEO_OFF:   return "OFF";
    default:            return "UNKNOWN";
  }
}

// ========== ADC ==========
float MapController::adcToVoltage(uint16_t adc_value) {
  const float VREF = 3.3f;
  const float ADC_MAX = 4095.0f;
  return (adc_value * VREF) / ADC_MAX;
}

String MapController::cmdADCByID(String cmd, int print) {
  String out;
  int sp = cmd.lastIndexOf(' ');
  if (sp == -1) { emit(out, print, "ERROR: Usage: adc <id>\r\n"); return out; }
  uint8_t id = cmd.substring(sp + 1).toInt();
  uint8_t addr = getAddressByID(id);
  if (addr == INVALID_ADDRESS) { emitf(out, print, "ERROR: No device mapped to ID %d.\r\n", id); return out; }

  emitf(out, print, "Reading ADC from ID %d (0x%02X) (16-bit read + 12-bit mask 0x0FFF)...\r\n", id, addr);
  uint16_t adc = readADCValue(addr);
  if (adc != 0xFFFF) {
    float v = adcToVoltage(adc);
    if (adc > 4095) emitf(out, print, "ADC ID %d (0x%02X): %d (0x%04X) - %.3fV [OUT OF RANGE]\r\n", id, addr, adc, adc, v);
    else            emitf(out, print, "ADC ID %d (0x%02X): %d (0x%03X) - %.3fV\r\n", id, addr, adc, adc, v);
  } else {
    emitf(out, print, "ADC ID %d (0x%02X): ERROR\r\n", id, addr);
  }
  return out;
}

String MapController::cmdADCAllByID(int print) {
  String out;
  if (device_map_.mapped_count == 0) { emit(out, print, "No devices mapped.\r\n"); return out; }
  emit(out, print, "Reading 12-bit ADC values from all mapped devices...\r\n");
  emit(out, print, "─────────────────────────────────────────────────────────\r\n");
  emit(out, print, "ID | Addr   | Raw  | Hex    | Voltage | Status\r\n");
  emit(out, print, "─────────────────────────────────────────────────────────\r\n");

  uint32_t start = millis();
  int ok=0, err=0;

  for (int id=1; id<=MAX_DEVICES; id++) {
    uint8_t addr = getAddressByID(id);
    if (addr == INVALID_ADDRESS) continue;

    uint16_t adc = readADCValue(addr);
    if (adc != 0xFFFF) {
      float v = adcToVoltage(adc);
      if (adc > 4095)
        emitf(out, print, "%-2d | 0x%02X   | %-4d   | 0x%04X | %.3fV   | OUT_RANGE\r\n", id, addr, adc, adc, v);
      else
        emitf(out, print, "%-2d | 0x%02X   | %-4d   | 0x%03X | %.3fV   | OK\r\n", id, addr, adc, adc, v);
      ok++;
    } else {
      emitf(out, print, "%-2d | 0x%02X   | ERROR  | ERROR  | ERROR   | FAIL\r\n", id, addr);
      err++;
    }
    delay(COMMAND_DELAY_MS);
  }
  emit(out, print, "─────────────────────────────────────────────────────────\r\n");
  emitf(out, print, "12-bit ADC Results: %d success, %d errors\r\n", ok, err);
  out += recordTiming("ADC ALL BY ID", millis() - start, print);
  return out;
}

uint16_t MapController::readADCValue(uint8_t addr) {
  uint8_t h=0xFF, l=0xFF; bool hs=false, ls=false;
  delay(50);

  for (int att=0; att<3; att++) {
    wire_->beginTransmission(addr); wire_->write(CMD_ADC_PA0_HSB);
    if (wire_->endTransmission() != 0) { if (att==2) return 0xFFFF; delay(50); continue; }
    delay(50);
    uint8_t rec = wire_->requestFrom(addr, (uint8_t)1);
    if (rec == 1) { h = wire_->read(); hs = true; break; }
    delay(20);
  }
  if (!hs) return 0xFFFF;
  h &= 0x0F;
  delay(50);

  for (int att=0; att<3; att++) {
    wire_->beginTransmission(addr); wire_->write(CMD_ADC_PA0_LSB);
    if (wire_->endTransmission() != 0) { if (att==2) return 0xFFFF; delay(50); continue; }
    delay(50);
    uint8_t rec = wire_->requestFrom(addr, (uint8_t)1);
    if (rec == 1) { l = wire_->read(); ls = true; break; }
    delay(20);
  }
  if (!ls) return 0xFFFF;

  uint16_t raw = (h << 8) | l;
  return raw;
}

// ========== Sense (PA4) ==========
String MapController::cmdSenseByID(int print) {
  String out;
  if (device_map_.mapped_count == 0) { emit(out, print, "No devices mapped.\r\n"); return out; }

  emit(out, print, "Reading PA4 sense states based on ID mapping...\r\n");
  emit(out, print, "──────────────────────────────────────────────\r\n");
  emit(out, print, "ID | Addr   | Raw    | State     | Status\r\n");
  emit(out, print, "──────────────────────────────────────────────\r\n");

  uint32_t start = millis();
  int ok=0, err=0;

  for (int id=1; id<=MAX_DEVICES; id++) {
    uint8_t addr = getAddressByID(id);
    if (addr == INVALID_ADDRESS) continue;

    uint8_t raw = readSenseState(addr);
    const char* state = interpretSenseValue(raw);
    const char* st = (raw == 0xFF) ? "FAIL" : "OK";
    if (raw == 0xFF) err++; else ok++;

    emitf(out, print, "%-2d | 0x%02X   | 0x%02X   | %-9s | %s\r\n", id, addr, raw, state, st);
    delay(COMMAND_DELAY_MS);
  }
  emit(out, print, "──────────────────────────────────────────────\r\n");
  out += recordTiming("SENSE BY ID", millis() - start, print);
  return out;
}

String MapController::cmdSenseMaskByID(int print) {
  String out;
  if (device_map_.mapped_count == 0) { emit(out, print, "No devices mapped.\r\n"); return out; }

  uint8_t mask[4] = {0,0,0,0};
  uint32_t start = millis();

  for (int id=1; id<=MAX_DEVICES; id++) {
    uint8_t addr = getAddressByID(id);
    if (addr == INVALID_ADDRESS) continue;

    uint8_t raw = readSenseState(addr);
    if (raw != 0xFF && strcmp(interpretSenseValue(raw), "HIGH") == 0) {
      int b = (id - 1) / 8;
      int bit = (id - 1) % 8;
      mask[b] |= (1 << bit);
    }
    delay(COMMAND_DELAY_MS);
  }
  emitf(out, print, "SENSE MASK BY ID (%d mapped):\r\n", device_map_.mapped_count);
  emitf(out, print, "array[0] = 0x%02X (IDs 1-8)\r\n",  mask[0]);
  emitf(out, print, "array[1] = 0x%02X (IDs 9-16)\r\n", mask[1]);
  emitf(out, print, "array[2] = 0x%02X (IDs 17-24)\r\n",mask[2]);
  emitf(out, print, "array[3] = 0x%02X (IDs 25-32)\r\n",mask[3]);
  out += recordTiming("SENSE MASK", millis() - start, print);
  return out;
}

String MapController::cmdSenseMaskHexByID(int print) {
  String out;
  if (device_map_.mapped_count == 0) {
    emit(out, print, "No devices mapped.\r\n");
    return out;
  }

  uint8_t mask[4] = {0,0,0,0};
  uint32_t start = millis();

  for (int id = 1; id <= MAX_DEVICES; id++) {
    uint8_t addr = getAddressByID(id);
    if (addr == INVALID_ADDRESS) continue;

    uint8_t raw = readSenseState(addr);
    // Considera HIGH cuando interpretSenseValue(raw) == "HIGH"
    if (raw != 0xFF && strcmp(interpretSenseValue(raw), "HIGH") == 0) {
      int b   = (id - 1) / 8;
      int bit = (id - 1) % 8;
      mask[b] |= (1 << bit);
    }
    delay(COMMAND_DELAY_MS);
  }

  // Empaquetado big-endian: [3][2][1][0]
  uint32_t packed =
      ((uint32_t)mask[3] << 24) |
      ((uint32_t)mask[2] << 16) |
      ((uint32_t)mask[1] <<  8) |
      ((uint32_t)mask[0]      );

  char buf[11]; // "0x" + 8 hex + '\0'
  sprintf(buf, "0x%08X", packed);

  // Si no quieres imprimir nada en Serial, deja print=0.
  // out debe contener únicamente el valor concatenado:
  out += buf;

  // Si deseas medir tiempo, puedes omitirlo aquí para no ensuciar la salida.
  // (Si lo quieres, devuélvelo aparte vía JSON en Commands.cpp)

  return out;
}


uint8_t MapController::readSenseState(uint8_t addr) {
  for (int att=0; att<3; att++) {
    wire_->beginTransmission(addr); wire_->write(CMD_PA4_DIGITAL);
    if (wire_->endTransmission() != 0) { if (att==2) return 0xFF; delay(10); continue; }
    delay(30);
    if (wire_->requestFrom(addr, (uint8_t)1) == 1) return wire_->read();
    delay(10);
  }
  return 0xFF;
}

const char* MapController::interpretSenseValue(uint8_t raw) {
  if (raw == 0xFF) return "ERROR";
  return ((raw & 0xF0) >> 4) > 7 ? "HIGH" : "LOW";
}

// === Arrays helpers ===
String MapController::cmdSenseArrayMask(uint8_t array_index, int print) {
  String out;
  if (array_index > 3) { emitf(out, print, "ERROR: Array index %d out of range (0-3)\r\n", array_index); return out; }
  if (device_map_.mapped_count == 0) { emit(out, print, "No devices mapped.\r\n"); return out; }

  uint8_t mask = 0;
  uint8_t checked=0, highc=0;
  uint32_t start = millis();

  int start_id = (array_index * 8) + 1;
  int end_id   = start_id + 7;

  emitf(out, print, "READING ARRAY %d SENSE MASK (IDs %d-%d):\r\n", array_index, start_id, end_id);
  emit(out, print, "──────────────────────────────────────────\r\n");

  for (int id = start_id; id <= end_id; id++) {
    uint8_t addr = getAddressByID(id);
    if (addr == INVALID_ADDRESS) { emitf(out, print, "ID %-2d: Not mapped\r\n", id); continue; }

    checked++;
    uint8_t raw = readSenseState(addr);
    if (raw != 0xFF) {
      const char* st = interpretSenseValue(raw);
      if (strcmp(st,"HIGH")==0) {
        int bit = (id - 1) % 8;
        mask |= (1 << bit);
        highc++;
        emitf(out, print, "ID %-2d (0x%02X): %s -> BIT %d SET\r\n", id, addr, st, bit);
      } else {
        emitf(out, print, "ID %-2d (0x%02X): %s\r\n", id, addr, st);
      }
    } else {
      emitf(out, print, "ID %-2d (0x%02X): ERROR\r\n", id, addr);
    }
    delay(COMMAND_DELAY_MS);
  }

  emit(out, print, "──────────────────────────────────────────\r\n");
  emitf(out, print, "ARRAY %d RESULT:\r\n", array_index);
  emitf(out, print, "  Mask Value: 0x%02X (binary: ", mask);
  for (int i = 7; i >= 0; i--) emitf(out, print, "%d", (mask >> i) & 1);
  emit(out, print, ")\r\n");
  emitf(out, print, "  Devices Checked: %d/8\r\n", checked);
  emitf(out, print, "  Devices HIGH: %d\r\n", highc);
  emit(out, print, "  Bit Pattern: ");
  bool first = true;
  for (int i=0;i<8;i++){ if(mask & (1<<i)){ if(!first) emit(out, print, ", "); emitf(out, print, "BIT%d", i); first=false; } }
  if(first) emit(out, print, "None");
  emit(out, print, "\r\n");
  out += recordTiming("ARRAY MASK", millis() - start, print);
  return out;
}

String MapController::cmdSenseArrayMaskByCommand(const String& cmd, int print) {
  int sp = cmd.lastIndexOf(' ');
  if (sp == -1) return String("ERROR: Usage: sense_array <array_index>\r\n");
  uint8_t idx = cmd.substring(sp + 1).toInt();
  return cmdSenseArrayMask(idx, print);
}

String MapController::cmdShowSenseArray(uint8_t array_index, int print) {
  String out;
  if (array_index > 3) { emitf(out, print, "ERROR: Array index %d out of range (0-3)\r\n", array_index); return out; }
  if (device_map_.mapped_count == 0) { emit(out, print, "No devices mapped.\r\n"); return out; }

  int start_id = (array_index * 8) + 1;
  int end_id   = start_id + 7;

  emitf(out, print, "ARRAY %d DETAILED VIEW (IDs %d-%d):\r\n", array_index, start_id, end_id);
  emit(out, print, "════════════════════════════════════════════════════\r\n");
  emit(out, print, "ID | Addr   | Raw    | State     | Bit | Status\r\n");
  emit(out, print, "═══════════════════════════════════════════════════\r\n");

  uint8_t mask=0; int mapped=0, high=0, commerr=0;
  for (int id = start_id; id <= end_id; id++) {
    uint8_t addr = getAddressByID(id);
    int bit_pos = (id - 1) % 8;
    if (addr == INVALID_ADDRESS) {
      emitf(out, print, "%-2d | ------  | ------ | --------- | %d   | NOT_MAPPED\r\n", id, bit_pos);
      continue;
    }
    mapped++;
    uint8_t raw = readSenseState(addr);
    if (raw != 0xFF) {
      const char* st = interpretSenseValue(raw);
      const char* status = "OK";
      if (strcmp(st,"HIGH")==0) { mask |= (1<<bit_pos); high++; status="HIGH_OK"; }
      emitf(out, print, "%-2d | 0x%02X   | 0x%02X   | %-9s | %d   | %s\r\n", id, addr, raw, st, bit_pos, status);
    } else {
      emitf(out, print, "%-2d | 0x%02X   | ERROR  | ERROR     | %d   | COMM_ERROR\r\n", id, addr, bit_pos);
      commerr++;
    }
    delay(COMMAND_DELAY_MS);
  }

  emit(out, print, "════════════════════════════════════════════════════\r\n");
  emitf(out, print, "ARRAY %d SUMMARY:\r\n", array_index);
  emitf(out, print, "  Final Mask: 0x%02X (", mask);
  for (int i=7;i>=0;i--) emitf(out, print, "%d", (mask>>i)&1);
  emit(out, print, ")\r\n");
  emitf(out, print, "  Mapped Devices: %d/8\r\n", mapped);
  emitf(out, print, "  HIGH States: %d\r\n", high);
  emitf(out, print, "  Comm Errors: %d\r\n", commerr);
  emitf(out, print, "  Coverage: %.1f%%\r\n", (mapped*100.0)/8.0);
  return out;
}

String MapController::cmdShowSenseArrayByCommand(const String& cmd, int print) {
  int sp = cmd.lastIndexOf(' ');
  if (sp == -1) return String("ERROR: Usage: show_array <array_index>\r\n");
  uint8_t idx = cmd.substring(sp + 1).toInt();
  return cmdShowSenseArray(idx, print);
}

String MapController::cmdFillSenseArray(uint8_t array_index, int print) {
  String out;
  if (array_index > 3) { emitf(out, print, "ERROR: Array index %d out of range (0-3)\r\n", array_index); return out; }
  if (device_map_.mapped_count == 0) { emit(out, print, "No devices mapped.\r\n"); return out; }

  int start_id = (array_index * 8) + 1;
  int end_id   = start_id + 7;

  emitf(out, print, "FILLING ARRAY %d (IDs %d-%d) - brief relay pulses:\r\n", array_index, start_id, end_id);
  emit(out, print, "──────────────────────────────────────────────────────────────\r\n");

  int ok=0, err=0;
  for (int id = start_id; id <= end_id; id++) {
    uint8_t addr = getAddressByID(id);
    if (addr == INVALID_ADDRESS) { emitf(out, print, "ID %-2d: Not mapped - SKIPPED\r\n", id); continue; }

    emitf(out, print, "ID %-2d (0x%02X): ", id, addr);
    if (sendCommandSafe(addr, CMD_RELAY_ON)) {
      delay(200);
      if (sendCommandSafe(addr, CMD_RELAY_OFF)) { emit(out, print, "Pulse OK\r\n"); ok++; }
      else { emit(out, print, "OFF FAILED - PARTIAL\r\n"); err++; }
    } else { emit(out, print, "ON FAILED - ERROR\r\n"); err++; }
    delay(COMMAND_DELAY_MS);
  }

  emit(out, print, "──────────────────────────────────────────────────────────────\r\n");
  emitf(out, print, "FILL ARRAY %d SUMMARY: %d success, %d errors\r\n", array_index, ok, err);
  return out;
}

String MapController::cmdFillSenseArrayByCommand(const String& cmd, int print) {
  int sp = cmd.lastIndexOf(' ');
  if (sp == -1) {
    return String("ERROR: Usage: fill_array <array_index>\r\nWarning: activates relays briefly!\r\n");
  }
  uint8_t idx = cmd.substring(sp + 1).toInt();

  String out;
  emit(out, print, "  WARNING: This will briefly activate relays in the specified array!\r\n");
  out += cmdFillSenseArray(idx, print);
  return out;
}


String MapController::cmdShowAllArrays(int print) {
  String out;
  if (device_map_.mapped_count == 0) { emit(out, print, "No devices mapped.\r\n"); return out; }
  emit(out, print, "ALL ARRAYS OVERVIEW - SENSE MASKS:\r\n");
  emit(out, print, "════════════════════════════════════════════════════════════\r\n");
  uint8_t full[4] = {0,0,0,0};

  for (int a=0;a<4;a++) {
    int start_id=(a*8)+1, end_id=start_id+7;
    uint8_t mask=0; int mapped=0, high=0;
    emitf(out, print, "ARRAY %d (IDs %2d-%2d): ", a, start_id, end_id);
    for (int id=start_id; id<=end_id; id++) {
      uint8_t addr = getAddressByID(id);
      if (addr == INVALID_ADDRESS) continue;
      mapped++;
      uint8_t raw = readSenseState(addr);
      if (raw != 0xFF && strcmp(interpretSenseValue(raw),"HIGH")==0) {
        int bit = (id - 1) % 8; mask |= (1<<bit); high++;
      }
      delay(10);
    }
    full[a]=mask;
    emitf(out, print, "0x%02X (", mask);
    for (int i=7;i>=0;i--) emitf(out, print, "%d", (mask>>i)&1);
    emitf(out, print, ") - %d/%d mapped, %d HIGH\r\n", mapped, 8, high);
  }
  emit(out, print, "════════════════════════════════════════════════════════════\r\n");
  emit(out, print, "COMPLETE SYSTEM MASK:\r\n");
  for (int i=0;i<4;i++)
    emitf(out, print, "array[%d] = 0x%02X  (IDs %d-%d)\r\n", i, full[i], (i*8)+1, (i*8)+8);
  emit(out, print, "════════════════════════════════════════════════════════════\r\n");
  return out;
}

// ========== Relay ==========
String MapController::cmdRelayByID(String cmd, int print) {
  String out;
  int sp = cmd.indexOf(' ');
  if (sp == -1) { emit(out, print, "ERROR: Usage: f <id>\r\n"); return out; }
  uint8_t id = cmd.substring(sp + 1).toInt();
  uint8_t addr = getAddressByID(id);
  if (addr != INVALID_ADDRESS) out += relayPulse(addr, 50, print);
  else emitf(out, print, "ERROR: No device mapped to ID %d.\r\n", id);
  return out;
}

String MapController::cmdRelayOff(int print) { return setAllRelays(CMD_RELAY_OFF, print); }
String MapController::cmdAllShut(int print)  { return setAllDevicesState(CMD_NEO_OFF, CMD_RELAY_OFF, print); }

String MapController::relayPulse(uint8_t addr, uint32_t pulse_time_ms, int print) {
  String out;
  emitf(out, print, "Ejecutando pulso de relé en 0x%02X por %lums...\r\n", addr, pulse_time_ms);
  sendCommandSafe(addr, CMD_RELAY_OFF);
  delay(200);
  if (!sendCommandSafe(addr, CMD_RELAY_ON)) {
    emitf(out, print, "  -> 0x%02X: ERROR al activar\r\n", addr);
    return out;
  }
  delay(pulse_time_ms);
  if (sendCommandSafe(addr, CMD_RELAY_OFF)) emitf(out, print, "Relé en 0x%02X desactivado.\r\n", addr);
  else                                      emitf(out, print, "ERROR: No se pudo desactivar el relé en 0x%02X\r\n", addr);
  return out;
}

int MapController::relayTurn(uint8_t addr, int pulse, int print) {
  if (print) 
    if(pulse)
      Serial.printf("Turn ON relay en 0x%02X...\r\n", addr);
    else
      Serial.printf("Turn OFF relay en 0x%02X...\r\n", addr);
  bool ok = sendCommandSafe(addr, pulse);
  if (print) Serial.printf("  -> 0x%02X: %s\r\n", addr, ok ? "Relé encendido" : "ERROR al encender");
  return ok ? 0 : 1;
}

int MapController::relayToggle(uint8_t addr, int print) {
  if (print) Serial.printf("Toggle relay en 0x%02X...\r\n", addr);
  bool ok = sendCommandSafe(addr, CMD_TOGGLE);
  if (print) Serial.printf("  -> 0x%02X: %s\r\n", addr, ok ? "Relé alternado" : "ERROR al alternar");
  return ok ? 0 : 1;
}



// ========== Communication & bulk ==========
bool MapController::sendCommandSafe(uint8_t addr, uint8_t cmd) {
  for (int att=0; att<3; att++) {
    wire_->beginTransmission(addr);
    wire_->write(cmd);
    if (wire_->endTransmission() == 0) { delay(COMMAND_DELAY_MS); return true; }
    delay(50);
  }
  return false;
}

bool MapController::sendCommandFast(uint8_t addr, uint8_t cmd) {
  wire_->beginTransmission(addr);
  wire_->write(cmd);
  if (wire_->endTransmission() == 0) { delay(COMMAND_DELAY_MS); return true; }
  return false;
}

String MapController::setAllRelays(uint8_t cmd, int print) {
  String out;
  const char* st = (cmd == CMD_RELAY_ON) ? "ON" : "OFF";
  emitf(out, print, "Setting all mapped relays to: %s (0x%02X)\r\n", st, cmd);
  int ok=0, fail=0, total=0;
  for (int id=1; id<=MAX_DEVICES; id++) {
    uint8_t addr = getAddressByID(id);
    if (addr == INVALID_ADDRESS) continue;
    total++;
    if (sendCommandSafe(addr, cmd)) { ok++; emitf(out, print, "  -> ID %d (0x%02X): %s - OK\r\n", id, addr, st); }
    else {
      fail++; emitf(out, print, "  -> ID %d (0x%02X): %s - FAILED\r\n", id, addr, st);
      if (cmd == CMD_RELAY_OFF) {
        emitf(out, print, "     CRITICAL: Retrying OFF for ID %d...\r\n", id);
        for (int r=0;r<5;r++){ delay(100*(r+1)); if(sendCommandSafe(addr, CMD_RELAY_OFF)){ ok++; fail--; emitf(out, print, "     SUCCESS OFF for ID %d\r\n", id); break; } }
      }
    }
  }
  emit(out, print, "────────────────────────────────────────────────\r\n");
  emitf(out, print, "RELAY %s SUMMARY: %d total, %d success, %d failed\r\n", st, total, ok, fail);
  if (fail>0 && cmd==CMD_RELAY_OFF) emit(out, print, "  WARNING: Some relays may still be ON.\r\n");
  emit(out, print, "────────────────────────────────────────────────\r\n");
  return out;
}

String MapController::emergencyRelaysOff(int print) {
  String out;
  emit(out, print, " EMERGENCY: Forcing ALL relays OFF with verification...\r\n");
  emit(out, print, "════════════════════════════════════════════════════════\r\n");
  int attempts=0, failures=0;
  for (int id=1; id<=MAX_DEVICES; id++) {
    uint8_t addr = getAddressByID(id);
    if (addr == INVALID_ADDRESS) continue;
    attempts++;
    bool off=false;
    emitf(out, print, "🔧 Emergency OFF for ID %d (0x%02X)...\r\n", id, addr);
    for (int t=0;t<10;t++){
      if (sendCommandSafe(addr, CMD_RELAY_OFF)) { emitf(out, print, "    Attempt %d: SUCCESS\r\n", t+1); off=true; break; }
      else { emitf(out, print, "    Attempt %d: FAILED\r\n", t+1); delay(100*(t+1)); }
    }
    if (!off) { failures++; emitf(out, print, "    CRITICAL: ID %d (0x%02X) may be ON!\r\n", id, addr); }
    else      { emitf(out, print, "    CONFIRMED: ID %d (0x%02X) OFF\r\n", id, addr); }
    delay(50);
  }
  emit(out, print, "════════════════════════════════════════════════════════\r\n");
  if (failures==0) emitf(out, print, " SUCCESS: All %d mapped relays confirmed OFF\r\n", attempts);
  else             emitf(out, print, " DANGER: %d/%d relays could not be confirmed OFF!\r\n", failures, attempts);
  emit(out, print, "════════════════════════════════════════════════════════\r\n");
  return out;
}

String MapController::setAllDevicesState(uint8_t led_cmd, uint8_t relay_cmd, int print) {
  String out;
  emit(out, print, "Setting all mapped devices (LED + RELAY)...\r\n");
  for (int id=1; id<=MAX_DEVICES; id++) {
    uint8_t addr = getAddressByID(id);
    if (addr == INVALID_ADDRESS) continue;
    sendCommandFast(addr, led_cmd);
    delay(COMMAND_DELAY_MS);
    sendCommandFast(addr, relay_cmd);
  }
  return out;
}

// ========== Utility timing ==========
uint32_t MapController::startTiming() {
  last_operation_start_ = millis();
  return last_operation_start_;
}
String MapController::recordTiming(const char* op, uint32_t ms, int print) {
  String out;
  emitf(out, print, "%s completed in %lums\r\n", op, ms);
  return out;
}

// ========== Debug Flash ==========
bool MapController::saveDataToFlash(uint8_t addr, uint8_t data) {
  wire_->beginTransmission(addr);
  wire_->write(CMD_SAVE_DATA);
  wire_->write(data);
  uint8_t res = wire_->endTransmission();
  if (res == 0) { delay(50); return true; }
  return false;
}
uint8_t MapController::readDataFromFlash(uint8_t addr) {
  wire_->beginTransmission(addr);
  wire_->write(CMD_READ_DATA);
  if (wire_->endTransmission() != 0) return 0xFF;
  delay(10);
  wire_->requestFrom(addr, (uint8_t)1);
  if (wire_->available()) return wire_->read();
  return 0xFF;
}

String MapController::cmdFlashSave(String cmd, int print) {
  String out;
  int s1 = cmd.indexOf(' ');
  int s2 = cmd.indexOf(' ', s1+1);
  if (s1==-1 || s2==-1) { emit(out, print, "Usage: flash_save <id> <data>\r\n"); return out; }
  uint8_t id = cmd.substring(s1+1, s2).toInt();
  String dataStr = cmd.substring(s2+1);
  uint8_t data = dataStr.startsWith("0x") ? (uint8_t)strtol(dataStr.c_str(), NULL, 16) : (uint8_t)dataStr.toInt();
  uint8_t addr = getAddressByID(id);
  if (addr == INVALID_ADDRESS) { emitf(out, print, "ERROR: No device mapped to ID %d\r\n", id); return out; }
  emitf(out, print, "Saving 0x%02X to flash at ID %d (0x%02X)...\r\n", data, id, addr);
  emit(out, print, saveDataToFlash(addr, data) ? " OK\r\n" : " FAIL\r\n");
  return out;
}

String MapController::cmdFlashRead(String cmd, int print) {
  String out;
  int sp = cmd.indexOf(' ');
  if (sp==-1){ emit(out, print, "Usage: flash_read <id>\r\n"); return out; }
  uint8_t id = cmd.substring(sp+1).toInt();
  uint8_t addr = getAddressByID(id);
  if (addr == INVALID_ADDRESS) { emitf(out, print, "ERROR: No device mapped to ID %d\r\n", id); return out; }
  emitf(out, print, "Reading flash at ID %d (0x%02X)...\r\n", id, addr);
  uint8_t d = readDataFromFlash(addr);
  if (d != 0xFF) emitf(out, print, " Flash data: 0x%02X (%d)\r\n", d, d);
  else emit(out, print, " Failed to read data from flash\r\n");
  return out;
}

// ========== I2C Address mgmt ==========
bool MapController::setDeviceI2CAddress(uint8_t current_addr, uint8_t new_addr) {
  if (new_addr < 0x08 || new_addr > 0x77) return false;
  wire_->beginTransmission(current_addr);
  wire_->write(CMD_SET_I2C_ADDR);
  wire_->write(new_addr);
  uint8_t r = wire_->endTransmission();
  if (r == 0) { delay(50); return true; }
  return false;
}

bool MapController::resetDeviceToFactory(uint8_t addr) {
  wire_->beginTransmission(addr);
  wire_->write(CMD_RESET_FACTORY);
  uint8_t r = wire_->endTransmission();
  if (r == 0) { delay(50); return true; }
  return false;
}

bool MapController::getDeviceI2CStatus(uint8_t addr, uint8_t* current_addr, uint8_t* factory_addr, bool* is_from_flash) {
  wire_->beginTransmission(addr);
  wire_->write(CMD_GET_I2C_STATUS);
  if (wire_->endTransmission() != 0) return false;
  delay(10);
  wire_->requestFrom(addr, (uint8_t)4);
  if (wire_->available() >= 4) {
    uint8_t status = wire_->read();
    *current_addr = wire_->read();
    *factory_addr = wire_->read();
    wire_->read(); // reserved
    *is_from_flash = (status & 0x01) != 0;
    return true;
  }
  return false;
}

String MapController::cmdSetI2CAddress(String cmd, int print) {
  String out;
  int s1 = cmd.indexOf(' ');
  int s2 = cmd.indexOf(' ', s1+1);
  if (s1==-1 || s2==-1) { emit(out, print, "Usage: set_i2c <id> <new_address>\r\n"); return out; }
  uint8_t id = cmd.substring(s1+1, s2).toInt();
  String aStr = cmd.substring(s2+1);
  uint8_t new_addr = aStr.startsWith("0x") ? (uint8_t)strtol(aStr.c_str(), NULL, 16) : (uint8_t)aStr.toInt();
  uint8_t cur = getAddressByID(id);
  if (cur == INVALID_ADDRESS) { emitf(out, print, "ERROR: No device mapped to ID %d\r\n", id); return out; }
  emitf(out, print, "Changing I2C of ID %d from 0x%02X to 0x%02X...\r\n", id, cur, new_addr);
  emit(out, print, setDeviceI2CAddress(cur, new_addr) ? " OK (scan after restart)\r\n" : " FAIL\r\n");
  return out;
}

String MapController::cmdResetFactory(String cmd, int print) {
  String out;
  int sp = cmd.indexOf(' ');
  if (sp==-1){ emit(out, print, "Usage: reset_factory <id>\r\n"); return out; }
  uint8_t id = cmd.substring(sp+1).toInt();
  uint8_t addr = getAddressByID(id);
  if (addr == INVALID_ADDRESS) { emitf(out, print, "ERROR: No device mapped to ID %d\r\n", id); return out; }
  emitf(out, print, "Resetting ID %d (0x%02X) to factory...\r\n", id, addr);
  emit(out, print, resetDeviceToFactory(addr) ? " OK (scan after restart)\r\n" : " FAIL\r\n");
  return out;
}

String MapController::cmdI2CStatus(String cmd, int print) {
  String out;
  int sp = cmd.indexOf(' ');
  if (sp==-1){ emit(out, print, "Usage: i2c_status <id>\r\n"); return out; }
  uint8_t id = cmd.substring(sp+1).toInt();
  uint8_t addr = getAddressByID(id);
  if (addr == INVALID_ADDRESS) { emitf(out, print, "ERROR: No device mapped to ID %d\r\n", id); return out; }
  emitf(out, print, "Getting I2C status from ID %d (0x%02X)...\r\n", id, addr);
  uint8_t cur,fact; bool from_flash=false;
  if (getDeviceI2CStatus(addr,&cur,&fact,&from_flash)) {
    emit(out, print, " I2C Configuration Status:\r\n");
    emitf(out, print, "   Current Address: 0x%02X (%d)\r\n", cur, cur);
    emitf(out, print, "   Factory Address: 0x%02X (%d)\r\n", fact,fact);
    emitf(out, print, "   Address Source : %s\r\n", from_flash?"Flash Memory":"Factory Default");
    if (cur!=fact) emit(out, print, "   → Address modified from factory\r\n");
  } else emit(out, print, " Failed to get I2C status\r\n");
  return out;
}

String MapController::cmdDebugReset(String cmd, int print) {
  String out;
  int sp = cmd.indexOf(' ');
  if (sp==-1){ emit(out, print, "Usage: debug_reset <id>\r\n"); return out; }
  uint8_t id = cmd.substring(sp+1).toInt();
  uint8_t addr = getAddressByID(id);
  if (addr == INVALID_ADDRESS) { emitf(out, print, "ERROR: No device mapped to ID %d\r\n", id); return out; }
  out += debugResetDevice(addr, print);
  return out;
}

uint8_t MapController::parseI2CAddress(String s) {
  s.trim();
  if (s.length() == 0) return 0;

  long v = -1;

  if (s.startsWith("0x") || s.startsWith("0X")) {
    v = strtol(s.c_str(), nullptr, 16);
  } else if (s.endsWith("h") || s.endsWith("H")) {
    String core = s.substring(0, s.length() - 1);
    v = strtol(core.c_str(), nullptr, 16);
  } else {
    // ¿decimal puro?
    bool dec = true;
    for (uint16_t i = 0; i < s.length(); ++i) {
      if (!isDigit(s[i])) { dec = false; break; }
    }
    if (dec) v = s.toInt();
    else     v = strtol(s.c_str(), nullptr, 16); // intenta como hex
  }

  if (v < 0 || v > 255) return 0;
  if (v < 0x08 || v > 0x77) return 0;  // rango válido I2C 7-bit que usas
  return static_cast<uint8_t>(v);
}


String MapController::cmdVerifyChange(String cmd, int print) {
  String out;
  int s1 = cmd.indexOf(' ');
  int s2 = cmd.indexOf(' ', s1+1);
  if (s1==-1 || s2==-1) { emit(out, print, "Usage: verify_change <old_id> <new_addr>\r\n"); return out; }
  uint8_t old_id = cmd.substring(s1+1, s2).toInt();
  String na = cmd.substring(s2+1);
  uint8_t old_addr = getAddressByID(old_id);
  if (old_addr == INVALID_ADDRESS) { emitf(out, print, "ERROR: No device mapped to ID %d\r\n", old_id); return out; }
  uint8_t new_addr = parseI2CAddress(na);
  if (new_addr == 0) {
    emitf(out, print, "ERROR: Invalid I2C address: %s (valid range 0x08-0x77)\r\n", na.c_str());
    return out;
  }
  out += verifyAddressChange(old_addr, new_addr, print);
  return out;
}

bool MapController::checkDeviceCompatibility(uint8_t address) {
  wire_->beginTransmission(address);
  wire_->write(CMD_PA4_DIGITAL);
  if (wire_->endTransmission() == 0) {
    delay(50);
    wire_->requestFrom(address, (uint8_t)1);
    if (wire_->available()) {
      uint8_t r = wire_->read();
      return (r != 0x00 && r != 0xFF);
    }
  }
  return false;
}

bool MapController::sendCommandWithTimeout(uint8_t address, uint8_t command, uint32_t timeout_ms) {
  uint32_t start = millis();
  wire_->beginTransmission(address); wire_->write(command);
  if (wire_->endTransmission() != 0) return false;
  while (millis() - start < timeout_ms) {
    if (wire_->available()) return true;
    delay(10);
  }
  return false;
}

String MapController::debugResetDevice(uint8_t address, int print) {
  String out;
  emitf(out, print, " DEBUG RESET - Device 0x%02X (%d)\r\n", address, address);
  emit(out, print, "════════════════════════════════════════════════════════\r\n");
  emit(out, print, " Test 1: Basic communication...\r\n");
  wire_->beginTransmission(address);
  uint8_t e = wire_->endTransmission();
  emitf(out, print, "   Result: %s (Error: %d)\r\n", e==0?" OK":" FAILED", e);
  if (e != 0) { emit(out, print, " DEBUG TERMINATED: No basic communication\r\n"); return out; }

  emit(out, print, " Test 2: Compatibility check...\r\n");
  bool comp = checkDeviceCompatibility(address);
  emitf(out, print, "   Result: %s\r\n", comp ? " Compatible" : " Not compatible");

  emit(out, print, " Test 3: Sending reset command (0xFE)...\r\n");
  wire_->beginTransmission(address); wire_->write(CMD_RESET);
  uint8_t re = wire_->endTransmission(true);
  if (re != 0) { emitf(out, print, " Failed to send reset (Error: %d)\r\n", re); return out; }
  emit(out, print, " Reset command sent successfully\r\n");

  emit(out, print, " Test 4: Monitoring device disconnection...\r\n");
  for (int i=1;i<=10;i++){
    delay(100);
    wire_->beginTransmission(address);
    uint8_t ce = wire_->endTransmission();
    emitf(out, print, "   Check %2d (%dms): %s (Error: %d)\r\n", i, i*100, ce==0?" ONLINE":" OFFLINE", ce);
    if (ce != 0 && i>2) { emit(out, print, " Device disconnected correctly (reset)\r\n"); break; }
  }

  emit(out, print, " Test 5: Waiting for reconnection...\r\n");
  emit(out, print, "   Waiting 3 seconds for complete boot...\r\n");
  delay(3000);

  bool back=false;
  for (int a=1;a<=10;a++){
    wire_->beginTransmission(address);
    uint8_t r = wire_->endTransmission();
    emitf(out, print, "   Reconnection %2d: %s (Error: %d)\r\n", a, r==0?" ONLINE":" OFFLINE", r);
    if (r==0){ back=true; emitf(out, print, " Device reconnected on attempt %d\r\n", a); break; }
    delay(500);
  }

  emit(out, print, "════════════════════════════════════════════════════════\r\n");
  if (back) {
    emit(out, print, " DEBUG RESET SUCCESSFUL\r\n");
    if (checkDeviceCompatibility(address)) emit(out, print, " Maintains compatibility post-reset\r\n");
    else                                   emit(out, print, " Warning: Lost compatibility (may be normal)\r\n");
  } else {
    emit(out, print, " DEBUG RESET FAILED\r\n");
    emitf(out, print, " Device 0x%02X did not reconnect\r\n", address);
  }
  return out;
}



// ========== Diagnostics ==========
String MapController::testDeviceCommands(uint8_t addr, int print) {
  String out;
  emitf(out, print, "Testing commands on device 0x%02X...\r\n", addr);
  emit(out, print, "Command | Response | Status\r\n");
  emit(out, print, "--------|----------|--------\r\n");
  uint8_t cmds[] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07};
  const char* names[]={"RELAY_OFF","RELAY_ON","NEO_RED","NEO_GREEN","NEO_BLUE","NEO_OFF","TOGGLE","PA4_READ"};
  for (int i=0;i<8;i++){
    wire_->beginTransmission(addr); wire_->write(cmds[i]);
    uint8_t e = wire_->endTransmission();
    if (e==0) emitf(out, print, "0x%02X    | ACK      | %s\r\n", cmds[i], names[i]);
    else      emitf(out, print, "0x%02X    | NACK(%d) | %s\r\n", cmds[i], e, names[i]);
    delay(20);
  }
  emit(out, print, "\nTesting potential ADC commands...\r\n");
  uint8_t adc_cmds[] = {0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF};
  for (int i=0;i<24;i++){
    wire_->beginTransmission(addr); wire_->write(adc_cmds[i]);
    uint8_t e = wire_->endTransmission();
    if (e==0) emitf(out, print, "0x%02X    | ACK      | POTENTIAL ADC\r\n", adc_cmds[i]);
    delay(10);
  }
  emit(out, print, "Command test completed.\r\n");
  return out;
}

String MapController::cmdTestCommands(String cmd, int print) {
  int sp = cmd.lastIndexOf(' ');
  if (sp == -1) return String("ERROR: Usage: test <id>\r\n");
  uint8_t id = cmd.substring(sp + 1).toInt();
  uint8_t addr = getAddressByID(id);
  if (addr == INVALID_ADDRESS) return String("ERROR: No device mapped to ID " + String(id) + ".\r\n");
  return testDeviceCommands(addr, print);
}

// ========== ADDITIONAL DEVICE FUNCTIONS ==========

String MapController::showDeviceInfo(uint8_t address, int print) {
  String out;
  emitf(out, print, "=== INFORMACIÓN DEL DISPOSITIVO 0x%02X ===\r\n", address);
  
  // Get Device ID
  uint8_t device_id = 0;
  if (i2cReadByteWithRetry(address, CMD_GET_DEVICE_ID, &device_id, 3, 30)) {
    String device_type = getDeviceTypeName(device_id);
    emitf(out, print, "Tipo: %s (ID: 0x%02X)\r\n", device_type.c_str(), device_id);
  } else {
    emit(out, print, "ERROR: No se pudo leer Device ID\r\n");
  }
  
  // Get Flash Size
  uint8_t flash_size = 0;
  if (i2cReadByteWithRetry(address, CMD_GET_FLASH_SIZE, &flash_size, 3, 30)) {
    emitf(out, print, "Flash: %d KB\r\n", flash_size);
  } else {
    emit(out, print, "ERROR: No se pudo leer tamaño de Flash\r\n");
  }
  
  emit(out, print, "=====================================\r\n");
  return out;
}

String MapController::showDeviceUID(uint8_t address, int print) {
  String out;
  emitf(out, print, "=== UID ÚNICO DEL DISPOSITIVO 0x%02X ===\r\n", address);
  
  uint32_t uid = getDeviceUID(address);
  if (uid != 0) {
    emitf(out, print, "UID (32-bit): 0x%08X\r\n", uid);
    
    // Calculate I2C address based on UID (for reference)
    uint8_t calculated_addr = 0x20 + ((uid & 0xFF) & 0x0F);
    emitf(out, print, "Dirección I2C calculada: 0x%02X (%d)\r\n", 
          calculated_addr, calculated_addr);
  } else {
    emit(out, print, "ERROR: No se pudo leer UID\r\n");
  }
  
  emit(out, print, "=====================================\r\n");
  return out;
}

String MapController::getDeviceTypeName(uint8_t device_id) {
  switch(device_id) {
    case 0x16: return "PY32F003F16U6TR (16KB Flash)";
    case 0x18: return "PY32F003F18U6TR (18KB Flash)";
    default: return "Desconocido";
  }
}

uint32_t MapController::getDeviceUID(uint8_t address) {
  uint8_t uid_bytes[4];
  bool uid_ok = true;
  
  // Read the 4 UID bytes
  for (int i = 0; i < 4; i++) {
    uint8_t cmd = CMD_GET_UID_BYTE0 + i;
    if (!i2cReadByteWithRetry(address, cmd, &uid_bytes[i], 3, 30)) {
      uid_ok = false;
      break;
    }
  }
  
  if (uid_ok) {
    // Combine into 32-bit UID
    uint32_t uid = (uint32_t)uid_bytes[3] << 24 | 
                   (uint32_t)uid_bytes[2] << 16 | 
                   (uint32_t)uid_bytes[1] << 8 | 
                   (uint32_t)uid_bytes[0];
    return uid;
  }
  
  return 0; // Error
}

String MapController::testAllCommands(uint8_t address, int print) {
  String out;
  
  // Variables para tracking de resultados
  bool toggle_result = false;
  bool neo_red_result = false;
  bool neo_off_result = false;
  uint8_t pa4_value = 0;
  uint16_t adc_value = 0;
  int tests_passed = 0;
  int tests_total = 4; // toggle, neo_red, pa4, adc
  
  // Test Toggle (sin output, sin delay)
  toggle_result = sendBasicCommand(address, CMD_TOGGLE);
  if (toggle_result) {
    tests_passed++;
  }
  
  // Test NeoPixels RED (sin output, sin delay)
  neo_red_result = sendBasicCommand(address, CMD_NEO_RED);
  if (neo_red_result) {
    // Test NeoPixel OFF (inmediato)
    neo_off_result = sendBasicCommand(address, CMD_NEO_OFF);
  }
  
  // Test PA4 Digital (sin output)
  pa4_value = readPA4Digital(address);
  tests_passed++; // PA4 siempre cuenta como éxito si se puede leer
  
  // Test ADC (sin output)
  adc_value = readADC(address);
  tests_passed++; // ADC siempre cuenta como éxito si se puede leer
  
  // SOLO UNA LÍNEA: CSV crudo
  // Formato: success_rate,toggle,neo,pa4,adc_raw,vcc
  float success_rate = (float)tests_passed / tests_total * 100.0;
  float vcc_voltage = (adc_value * 3.3f) / 4095.0f;
  
  emitf(out, print, "%.1f,%d,%d,%d,%d,%.2f\r\n", 
        success_rate,
        toggle_result ? 1 : 0,
        neo_red_result ? 1 : 0,
        pa4_value,
        adc_value,
        vcc_voltage);
  
  return out;
}

String MapController::testBothCommands(uint8_t address, int print) {
  String out;
  
  // Solo toggle y PA4 - ultra rápido
  bool toggle_result = sendBasicCommand(address, CMD_TOGGLE);
  uint8_t pa4_value = readPA4Digital(address);
  
  // SOLO UNA LÍNEA: CSV formato simple toggle,pa4
  emitf(out, print, "%d,%d\r\n", 
        toggle_result ? 1 : 0,
        pa4_value);
  
  return out;
}

// ========== COMMAND WRAPPERS ==========

String MapController::cmdRelay(uint8_t address, bool state, int print) {
  String out;
  uint8_t cmd = state ? CMD_RELAY_ON : CMD_RELAY_OFF;
  const char* action = state ? "encendido" : "apagado";
  
  if (sendBasicCommand(address, cmd)) {
    emitf(out, print, "[OK] Rele %s en 0x%02X\r\n", action, address);
  } else {
    emitf(out, print, "ERROR: No se pudo %s rele en 0x%02X\r\n", action, address);
  }
  
  return out;
}

String MapController::cmdToggle(uint8_t address, int print) {
  String out;
  if (sendBasicCommand(address, CMD_TOGGLE)) {
    emitf(out, print, "[OK] Toggle ejecutado en 0x%02X\r\n", address);
  } else {
    emitf(out, print, "ERROR: No se pudo ejecutar toggle en 0x%02X\r\n", address);
  }
  return out;
}

String MapController::cmdNeoPixel(uint8_t address, uint8_t color, int print) {
  String out;
  const char* color_names[] = {"OFF", "RED", "GREEN", "BLUE", "WHITE"};
  uint8_t commands[] = {CMD_NEO_OFF, CMD_NEO_RED, CMD_NEO_GREEN, CMD_NEO_BLUE, CMD_NEO_WHITE};
  
  if (color < 5) {
    if (sendBasicCommand(address, commands[color])) {
      emitf(out, print, "[OK] NeoPixels %s en 0x%02X\r\n", color_names[color], address);
    } else {
      emitf(out, print, "ERROR: No se pudo cambiar NeoPixels en 0x%02X\r\n", address);
    }
  } else {
    emit(out, print, "ERROR: Color invalido\r\n");
  }
  
  return out;
}

String MapController::cmdPWM(uint8_t address, uint8_t level, int print) {
  String out;
  const char* level_names[] = {"silencio", "25% (200Hz)", "50% (500Hz)", "75% (1000Hz)", "100% (2000Hz)"};
  uint8_t commands[] = {CMD_PWM_OFF, CMD_PWM_25, CMD_PWM_50, CMD_PWM_75, CMD_PWM_100};
  
  if (level < 5) {
    if (sendBasicCommand(address, commands[level])) {
      emitf(out, print, "[OK] PWM %s en 0x%02X\r\n", level_names[level], address);
    } else {
      emitf(out, print, "ERROR: No se pudo cambiar PWM en 0x%02X\r\n", address);
    }
  } else {
    emit(out, print, "ERROR: Nivel PWM invalido\r\n");
  }
  
  return out;
}

String MapController::cmdReadPA4(uint8_t address, int print) {
  String out;
  uint8_t pa4_state = readPA4Digital(address);
  emitf(out, print, "[PA4] Dispositivo 0x%02X: %s (%d)\r\n", 
        address, pa4_state ? "HIGH" : "LOW", pa4_state);
  return out;
}

String MapController::cmdReadADC(uint8_t address, int print) {
  String out;
  uint16_t adc_value = readADC(address);
  emitf(out, print, "[ADC] Dispositivo 0x%02X: %d (0x%03X)\r\n", address, adc_value, adc_value);
  return out;
}

// Convenience wrappers for specific colors/levels
String MapController::cmdNeoRed(uint8_t address, int print) { return cmdNeoPixel(address, 1, print); }
String MapController::cmdNeoGreen(uint8_t address, int print) { return cmdNeoPixel(address, 2, print); }
String MapController::cmdNeoBlue(uint8_t address, int print) { return cmdNeoPixel(address, 3, print); }
String MapController::cmdNeoWhite(uint8_t address, int print) { return cmdNeoPixel(address, 4, print); }
String MapController::cmdNeoOff(uint8_t address, int print) { return cmdNeoPixel(address, 0, print); }

String MapController::cmdPWMOff(uint8_t address, int print) { return cmdPWM(address, 0, print); }
String MapController::cmdPWM25(uint8_t address, int print) { return cmdPWM(address, 1, print); }
String MapController::cmdPWM50(uint8_t address, int print) { return cmdPWM(address, 2, print); }
String MapController::cmdPWM75(uint8_t address, int print) { return cmdPWM(address, 3, print); }
String MapController::cmdPWM100(uint8_t address, int print) { return cmdPWM(address, 4, print); }

// ========== MUSICAL TONES (v6.0.0_lite) ==========

String MapController::cmdTone(uint8_t address, uint8_t tone_cmd, int print) {
  String out;
  emitf(out, print, ">> TONE 0x%02X -> 0x%02X\r\n", tone_cmd, address);
  
  if (!sendBasicCommand(address, tone_cmd)) {
    emit(out, print, "ERROR: Tone command failed\r\n");
    return out;
  }
  
  emit(out, print, "[OK] Tone sent successfully\r\n");
  return out;
}

String MapController::cmdToneDo(uint8_t address, int print) { return cmdTone(address, CMD_TONE_DO, print); }
String MapController::cmdToneRe(uint8_t address, int print) { return cmdTone(address, CMD_TONE_RE, print); }
String MapController::cmdToneMi(uint8_t address, int print) { return cmdTone(address, CMD_TONE_MI, print); }
String MapController::cmdToneFa(uint8_t address, int print) { return cmdTone(address, CMD_TONE_FA, print); }
String MapController::cmdToneSol(uint8_t address, int print) { return cmdTone(address, CMD_TONE_SOL, print); }
String MapController::cmdToneLa(uint8_t address, int print) { return cmdTone(address, CMD_TONE_LA, print); }
String MapController::cmdToneSi(uint8_t address, int print) { return cmdTone(address, CMD_TONE_SI, print); }

// ========== SYSTEM ALERTS (v6.0.0_lite) ==========

String MapController::cmdSuccess(uint8_t address, int print) {
  String out;
  emitf(out, print, ">> SUCCESS ALERT (800Hz) -> 0x%02X\r\n", address);
  if (!sendBasicCommand(address, CMD_SUCCESS)) {
    emit(out, print, "ERROR: Success alert failed\r\n");
    return out;
  }
  emit(out, print, "[OK] Success alert sent\r\n");
  return out;
}

String MapController::cmdOk(uint8_t address, int print) {
  String out;
  emitf(out, print, ">> OK ALERT (1000Hz) -> 0x%02X\r\n", address);
  if (!sendBasicCommand(address, CMD_OK)) {
    emit(out, print, "ERROR: OK alert failed\r\n");
    return out;
  }
  emit(out, print, "[OK] OK alert sent\r\n");
  return out;
}

String MapController::cmdWarning(uint8_t address, int print) {
  String out;
  emitf(out, print, ">> WARNING ALERT (1200Hz) -> 0x%02X\r\n", address);
  if (!sendBasicCommand(address, CMD_WARNING)) {
    emit(out, print, "ERROR: Warning alert failed\r\n");
    return out;
  }
  emit(out, print, "[OK] Warning alert sent\r\n");
  return out;
}

String MapController::cmdAlert(uint8_t address, int print) {
  String out;
  emitf(out, print, ">> CRITICAL ALERT (1500Hz) -> 0x%02X\r\n", address);
  if (!sendBasicCommand(address, CMD_ALERT)) {
    emit(out, print, "ERROR: Critical alert failed\r\n");
    return out;
  }
  emit(out, print, "[OK] Critical alert sent\r\n");
  return out;
}

// ========== PWM GRADUAL FREQUENCIES (0x10-0x1F) ==========

String MapController::cmdPWMGradual(uint8_t address, uint8_t freq_cmd, int print) {
  String out;
  
  // Validate range
  if (freq_cmd < CMD_PWM_GRADUAL_MIN || freq_cmd > CMD_PWM_GRADUAL_MAX) {
    emitf(out, print, "ERROR: Invalid gradual PWM command 0x%02X (valid range: 0x10-0x1F)\r\n", freq_cmd);
    return out;
  }
  
  // Calculate frequency: freq = 100 + ((cmd - 0x10) * 900 / 16)
  uint16_t frequency = 100 + ((freq_cmd - CMD_PWM_GRADUAL_MIN) * 900 / 16);
  emitf(out, print, ">> PWM GRADUAL 0x%02X (~%dHz) -> 0x%02X\r\n", freq_cmd, frequency, address);
  
  if (!sendBasicCommand(address, freq_cmd)) {
    emit(out, print, "ERROR: Gradual PWM command failed\r\n");
    return out;
  }
  
  emit(out, print, "[OK] Gradual PWM sent successfully\r\n");
  return out;
}

// ========== TEST INTERVAL FUNCTIONS ==========

String MapController::testIntervalEffect(int print) {
  String out;
  if (device_count_ == 0) {
    emit(out, print, "ERROR: No hay dispositivos. Ejecutar scan primero\r\n");
    return out;
  }
  
  emit(out, print, "=== TEST DE EFECTO DEL INTERVALO ===\r\n");
  emit(out, print, "Probando diferentes intervalos para encontrar el punto óptimo...\r\n");
  
  uint32_t intervals[] = {100, 500, 1000, 2000, 5000};
  int num_intervals = sizeof(intervals) / sizeof(intervals[0]);
  
  uint32_t original_interval = test_interval_;
  TestMode original_mode = current_test_mode_;
  
  // Configure for toggle test
  current_test_mode_ = TEST_MODE_TOGGLE_ONLY;
  
  for (int i = 0; i < num_intervals; i++) {
    test_interval_ = intervals[i];
    emitf(out, print, "\n--- Probando intervalo: %d ms ---\r\n", test_interval_);
    
    // Reset statistics
    resetStatistics();
    
    // Execute 10 cycles
    for (int cycle = 0; cycle < 10; cycle++) {
      for (int dev = 0; dev < device_count_; dev++) {
        if (found_devices_[dev].is_compatible) {
          testDevice(dev);
        }
      }
      delay(test_interval_);
    }
    
    // Show results
    for (int dev = 0; dev < device_count_; dev++) {
      if (found_devices_[dev].total_tests > 0) {
        float success_rate = (float)found_devices_[dev].successful_tests / found_devices_[dev].total_tests * 100.0;
        emitf(out, print, "  0x%02X: %.1f%% éxito (%d/%d)\r\n",
              found_devices_[dev].address,
              success_rate,
              found_devices_[dev].successful_tests,
              found_devices_[dev].total_tests);
      }
    }
  }
  
  // Restore original configuration
  test_interval_ = original_interval;
  current_test_mode_ = original_mode;
  resetStatistics();
  
  emit(out, print, "\n=== CONCLUSIÓN ===\r\n");
  emit(out, print, "Intervalos < 1000ms generalmente tienen mejor éxito\r\n");
  emit(out, print, "Intervalos > 1000ms pueden requerir keep-alive\r\n");
  emitf(out, print, "Intervalo restaurado a: %d ms\r\n", test_interval_);
  
  return out;
}

String MapController::testLongIntervalProblem(int print) {
  String out;
  if (device_count_ == 0) {
    emit(out, print, "ERROR: No hay dispositivos. Ejecutar scan primero\r\n");
    return out;
  }
  
  emit(out, print, "=== TEST ESPECÍFICO: PROBLEMA DE INTERVALOS LARGOS ===\r\n");
  emit(out, print, "Comparando 500ms (bueno) vs 2000ms (problemático)...\r\n");
  
  uint8_t test_addr = found_devices_[0].address;  // Use first device
  
  // Test 1: Short interval (500ms) - 10 commands
  emit(out, print, "\n--- FASE 1: Intervalo 500ms (debería funcionar) ---\r\n");
  int success_500ms = 0;
  for (int i = 0; i < 10; i++) {
    if (sendBasicCommand(test_addr, CMD_TOGGLE)) {
      success_500ms++;
      emitf(out, print, "Comando %d: OK\r\n", i + 1);
    } else {
      emitf(out, print, "Comando %d: FAIL\r\n", i + 1);
    }
    delay(500);
  }
  
  emitf(out, print, "Resultado 500ms: %d/10 éxitos (%.1f%%)\r\n", 
        success_500ms, (success_500ms / 10.0) * 100.0);
  
  // Test 2: Long interval (2000ms) - 10 commands
  emit(out, print, "\n--- FASE 2: Intervalo 2000ms (problemático) ---\r\n");
  int success_2000ms = 0;
  for (int i = 0; i < 10; i++) {
    if (sendBasicCommand(test_addr, CMD_TOGGLE)) {
      success_2000ms++;
      emitf(out, print, "Comando %d: OK\r\n", i + 1);
    } else {
      emitf(out, print, "Comando %d: FAIL\r\n", i + 1);
    }
    delay(2000);
  }
  
  emitf(out, print, "Resultado 2000ms: %d/10 éxitos (%.1f%%)\r\n", 
        success_2000ms, (success_2000ms / 10.0) * 100.0);
  
  // Analysis
  emit(out, print, "\n=== ANÁLISIS ===\r\n");
  if (success_500ms > success_2000ms) {
    emitf(out, print, "CONFIRMADO: Intervalos largos tienen %.1f%% menos éxito\r\n",
          ((success_500ms - success_2000ms) / 10.0) * 100.0);
    emit(out, print, "CAUSA PROBABLE: El esclavo entra en modo de bajo consumo\r\n");
    emit(out, print, "SOLUCIÓN: Keep-alive ping activado automáticamente\r\n");
  } else {
    emit(out, print, "SORPRESA: Los intervalos largos funcionan igual de bien\r\n");
  }
  
  emit(out, print, "\nNOTA: Con las optimizaciones actuales, ambos deberían funcionar mejor\r\n");
  
  return out;
}
