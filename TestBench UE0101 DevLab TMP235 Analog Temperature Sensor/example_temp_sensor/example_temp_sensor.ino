#define TMP_PIN 6

float readTempC() {
  const int N = 20;
  long sum_mv = 0;

  for (int i = 0; i < N; i++) {
    sum_mv += analogReadMilliVolts(TMP_PIN);
    delay(5);
  }

  float v_mv = sum_mv / (float)N;

  // Tu curva: 0°C = 500mV, 10mV/°C
  float tempC = (v_mv - 500.0f) / 10.0f;
  return tempC;
}

void setup() {
  Serial.begin(115200);
  delay(500);

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
}

void loop() {
  int raw = analogRead(TMP_PIN);
  int mv  = analogReadMilliVolts(TMP_PIN);
  float t = readTempC();

  Serial.print("Raw: "); Serial.print(raw);
  Serial.print(" | mV: "); Serial.print(mv);
  Serial.print(" | Temp: "); Serial.print(t, 2);
  Serial.println(" C");

  delay(1000);
}