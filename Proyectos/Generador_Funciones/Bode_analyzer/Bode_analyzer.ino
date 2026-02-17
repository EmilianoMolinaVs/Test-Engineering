#include <Arduino.h>

// ---- Pines de lectura analógicos ----
#define Vin  6
#define Vout 1

// ---- Estructura para retornar datos ----
struct ResultadoMedicion {
  uint16_t vinMin;
  uint16_t vinMax;
  uint16_t voutMin;
  uint16_t voutMax;
  uint16_t deltaVout;   // <-- nuevo
  float ganancia;
};

void setup() {
  Serial.begin(115200);

  //analogReadResolution(12);
  //analogSetAttenuation(ADC_11db);

  // Encabezado en columnas
  Serial.println("VinMin\tVinMax\tVoutMin\tVoutMax\tDeltaVout\tGanancia");
}

void loop() {

  ResultadoMedicion datos = medirGanancia(Vin, Vout, 10, 400);

  // Mostrar en columnas ordenadas
  Serial.print(datos.vinMin);     Serial.print("\t");
  Serial.print(datos.vinMax);     Serial.print("\t");
  Serial.print(datos.voutMin);    Serial.print("\t");
  Serial.print(datos.voutMax);    Serial.print("\t");
  Serial.print(datos.deltaVout);  Serial.print("\t");
  Serial.println(datos.ganancia, 4);

  delay(50);
}

// -----------------------------------------------------

ResultadoMedicion medirGanancia(uint8_t pinIn, uint8_t pinOut,
                                 uint32_t tiempoEstabilizacion_ms,
                                 uint32_t tiempoMedicion_ms)
{
  ResultadoMedicion res;

  delay(tiempoEstabilizacion_ms);

  res.vinMax  = 0;
  res.vinMin  = 4095;
  res.voutMax = 0;
  res.voutMin = 4095;

  uint32_t tiempoMedicion_us = tiempoMedicion_ms * 1000UL;
  uint32_t inicio = micros();

  while (micros() - inicio < tiempoMedicion_us)
  {
    uint16_t vin  = analogRead(pinIn);
    uint16_t vout = analogRead(pinOut);

    if (vin  > res.vinMax)  res.vinMax  = vin;
    if (vin  < res.vinMin)  res.vinMin  = vin;

    if (vout > res.voutMax) res.voutMax = vout - 130;
    if (vout < res.voutMin) res.voutMin = vout - 130;
  }

  // Calcular delta Vout (pico a pico)
  res.deltaVout = res.voutMax - res.voutMin;

  float ampIn  = (res.vinMax  - res.vinMin)  / 2.0;
  float ampOut = res.deltaVout / 2.0;

  if (ampIn < 1) {
    res.ganancia = 0;
  } else {
    res.ganancia = ampOut / ampIn;
  }

  return res;
}
