
#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

#define PIN 1       // El pin al que está conectada tu tira
#define NUMPIXELS 100 // Cantidad de LEDs en tu tira
const int lux = 20; 

// Declaramos la tira de NeoPixels
Adafruit_NeoPixel tira(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

// Definimos los colores patrios 
// (Usamos un valor de 150 en lugar de 255 para no deslumbrar y ahorrar energía, pero puedes subirlo a 255)
uint32_t verde = tira.Color(0, lux, 0);
uint32_t blanco = tira.Color(lux, lux, lux);
uint32_t rojo = tira.Color(lux, 0, 0);

void setup() {
  tira.begin();
  tira.show(); // Inicializa todos los LEDs apagados
}

void loop() {
  // --- SECUENCIA 1: Bandera estática (se muestra por 3 segundos) ---
  mostrarBandera();
  delay(3000); 

  // --- SECUENCIA 2: Llenado de colores (Olas verde, blanco, rojo) ---
  llenadoDeColor(verde, 30);
  llenadoDeColor(blanco, 30);
  llenadoDeColor(rojo, 30);

  // --- SECUENCIA 3: Carrusel corriendo (se repite 20 veces) ---
  carruselPatrio(100, 20);
}

// ================= FUNCIONES DE LOS EFECTOS =================

// Función 1: Muestra los 100 LEDs divididos en 3 franjas
void mostrarBandera() {
  for (int i = 0; i < NUMPIXELS; i++) {
    if (i < 33) {
      tira.setPixelColor(i, verde);
    } else if (i < 66) {
      tira.setPixelColor(i, blanco);
    } else {
      tira.setPixelColor(i, rojo);
    }
  }
  tira.show();
}

// Función 2: Llena la tira led por led con un color
void llenadoDeColor(uint32_t color, int pausa) {
  for (int i = 0; i < NUMPIXELS; i++) {
    tira.setPixelColor(i, color);
    tira.show();
    delay(pausa);
  }
}

// Función 3: Crea un efecto de luces en movimiento intercalando los 3 colores
void carruselPatrio(int pausa, int ciclos) {
  for (int j = 0; j < ciclos * 3; j++) { 
    for (int i = 0; i < NUMPIXELS; i++) {
      // Determinamos qué color le toca a cada LED basándonos en la posición y el ciclo
      if ((i + j) % 3 == 0) {
        tira.setPixelColor(i, verde);
      } else if ((i + j) % 3 == 1) {
        tira.setPixelColor(i, blanco);
      } else {
        tira.setPixelColor(i, rojo);
      }
    }
    tira.show();
    delay(pausa);
  }
}

