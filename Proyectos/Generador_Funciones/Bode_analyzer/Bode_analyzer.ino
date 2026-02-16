#include <ArduinoJson.h>
#include <Arduino.h>

// ---- Pines de lectura analógicos [2]----
#define Vin 6
#define Vout 0

void setup() {
  Serial.begin(115200);

  // ---- Configuración del ADC ----
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
}

void loop() {
  float ganancia, v_sal = medirGanancia(Vin, Vout, 1, 400);

  Serial.println(ganancia);

  delay(50);  // tiempo entre frecuencias
}



float medirGanancia(uint8_t pinIn, uint8_t pinOut,
                    uint32_t tiempoEstabilizacion_ms,
                    uint32_t tiempoMedicion_ms) {
  // 1️⃣ Esperar estabilización
  delay(tiempoEstabilizacion_ms);

  uint16_t vinMax = 0;
  uint16_t vinMin = 4095;
  uint16_t voutMax = 0;
  uint16_t voutMin = 4095;

  // 2️⃣ Medición durante ventana
  uint32_t inicio = micros();
 while (micros() - inicio < tiempoMedicion_ms)

  {
    uint16_t vin = analogRead(pinIn);
    uint16_t vout = analogRead(pinOut);

    if (vin > vinMax) vinMax = vin;
    if (vin < vinMin) vinMin = vin;

    if (vout > voutMax) voutMax = vout;
    if (vout < voutMin) voutMin = vout;
  }

  // 3️⃣ Calcular amplitudes
  float ampIn = (vinMax - vinMin) / 2.0;
  float ampOut = (voutMax - voutMin) / 2.0;

  //if (ampIn < 1) return 0;  // evitar división rara

  float ganancia = ampOut / ampIn;

  return ganancia, voutMax;
}
