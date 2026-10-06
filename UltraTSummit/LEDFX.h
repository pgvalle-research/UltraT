#ifndef ledFX_H
#define ledFX_H
#include "PID.h"
#include <Adafruit_NeoPixel.h>
#include "placa.h"

#define LED4 3
#define LED5 4
#define LED6 5
#define LED7 7
#define LED1 0
#define NUMPIXELS 8 // quantidade de leds do anel
Adafruit_NeoPixel pixels(NUMPIXELS, LED_STRIP, NEO_GRB + NEO_KHZ800); // necessario

/*----------------------------------------------------------------------------------------*/
void ledLight (int r, int g, int b) {   // luz contínua
  pixels.clear();
  for(int i=0; i<NUMPIXELS; i++) { 
    pixels.setPixelColor(i, pixels.Color(r, g, b));
    pixels.show();    
  }
}

void setDefaultColor(uint8_t r, uint8_t g, uint8_t b) {
  for (uint8_t i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(r, g, b));
  }
}

void ledDetection() {
  setDefaultColor(150, 0, 0); // Define cor padrão
  leituraSensores();
   
  if (leitura[0]) {
    Serial.println("ESQUERDA LATERAL DETECTADO");
    pixels.setPixelColor(LED7, pixels.Color(0, 150, 0));              // LED principal
    pixels.setPixelColor((LED7 - 1) % NUMPIXELS, pixels.Color(0, 150, 0)); // LED ao lado
  }

  if (leitura[1]) {
    Serial.println("ESQUERDA FRONTAL DETECTADO");
    pixels.setPixelColor(LED5, pixels.Color(0, 0, 150));
    pixels.setPixelColor((LED5 + 1) % NUMPIXELS, pixels.Color(0, 0, 150));
  }

  if (leitura[2]) {
    Serial.println("DIREITA FRONTAL DETECTADO");
    pixels.setPixelColor(LED4, pixels.Color(0, 0, 150));
    pixels.setPixelColor((LED4 - 1)   , pixels.Color(0, 0, 150));
  }

  if (leitura[3]) {
    Serial.println("DIREITA LATERAL DETECTADO");
    pixels.setPixelColor(LED1, pixels.Color(0, 150, 0));
    pixels.setPixelColor((LED1 + 1) % NUMPIXELS, pixels.Color(0, 150, 0));
  }
  if (leitura[4]) {
    Serial.println("LINHA ESQUERDA DETECTADO");
    pixels.setPixelColor(LED7, pixels.Color(0, 0, 150));
  }
  if (leitura[5]) {
    Serial.println("LINHA DIREITA DETECTADO");
    pixels.setPixelColor(LED1, pixels.Color(0, 0, 150));
  }
  pixels.show();
  delay(10);
}




#endif

