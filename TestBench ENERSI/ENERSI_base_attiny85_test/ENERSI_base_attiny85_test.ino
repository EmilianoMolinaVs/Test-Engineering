/*
Este es el blink de prueba para el Attiny85 en la placa base del proyecto ENERSI. 
Se flashea por medio del programador de avr usando avrdude desde la terminal.
*/

#include <SoftwareSerial.h>

SoftwareSerial miSerial(3, 4);  // RX, TX

void setup() {
  miSerial.begin(9600);
  miSerial.println("Hola Mundo");
}

void loop() {
  if (miSerial.available() > 0) {
    // Leemos hasta el salto de línea
    String input = miSerial.readStringUntil('\n');
    input.trim(); // Limpiamos espacios o retornos de c+arro (\r) extra

    if (input == "ping") {
      // Respuesta simplificada en lugar de JSON
      miSerial.println("pong");
    } 
    else {
      miSerial.println("FAIL: invalid option");
    }
  }
}