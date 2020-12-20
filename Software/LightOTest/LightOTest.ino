// Import required libraries
#include "math.h"
#include <NeoPixelBus.h> // LED control

// LED Definitions //////////////////////////////////////////////////////////////////////////
#define NUM_LEDS 61
#define DATA_PIN 32

#define brightnessDefault 255

// four element pixels, RGBW
NeoPixelBus<NeoGrbwFeature, Neo800KbpsMethod> strip(NUM_LEDS, DATA_PIN);


// Set color of all pixels
void colorAll(RgbwColor color) {
  for (uint16_t i = 0; i < strip.PixelCount(); i++) {
    strip.SetPixelColor(i, color);
  }
  strip.Show();
}


void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);

  Serial.println();
  Serial.println("Initializing...");
  Serial.flush();

  // Set LED output
  pinMode(DATA_PIN, OUTPUT);

  // LED stuff ////////////////////////////////////////////////////////////////////////////////
  Serial.println("Setting up LEDs...");
  // Reset all the pixels to the off state
  strip.Begin();
  strip.Show();
}


void loop() {
  delay(1000);
  RgbwColor colorR = RgbwColor(brightnessDefault, 0, 0, 0);
  colorAll(colorR);
  delay(1000);
  RgbwColor colorG = RgbwColor(0, brightnessDefault, 0, 0);
  colorAll(colorG);
  delay(1000);
  RgbwColor colorB = RgbwColor(0, 0, brightnessDefault, 0);
  colorAll(colorB);
  delay(1000);
  RgbwColor colorW = RgbwColor(0, 0, 0, brightnessDefault);
  colorAll(colorW);
  delay(1000);
  RgbwColor color0 = RgbwColor(0, 0, 0, 0);
  colorAll(color0);
}
