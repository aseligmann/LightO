// Import required libraries
#include "WiFi.h"
#include <ESPmDNS.h>
#include <WiFiClient.h>
//// https://github.com/me-no-dev/ESPAsyncWebServer/issues/418#issuecomment-667976368
#include <WiFiManager.h>
WiFiManager wifiManager;
#define WEBSERVER_H
#include "ESPAsyncWebServer.h"
////
#include "SPIFFS.h"
#include "math.h"
#include <NeoPixelBus.h>
//#include "Credentials.h"




// Multi-core definitions //////////////////////////////////////////////////////////////////////////
TaskHandle_t taskCore0;
TaskHandle_t taskCore1;
SemaphoreHandle_t taskSemaphore;



// LED Definitions //////////////////////////////////////////////////////////////////////////
#define NUM_LEDS 61
#define DATA_PIN 32

#define brightnessDefault 50

// four element pixels, RGBW
NeoPixelBus<NeoGrbwFeature, Neo800KbpsMethod> strip(NUM_LEDS, DATA_PIN);

RgbwColor red = RgbwColor(brightnessDefault, 0, 0, 0);
RgbwColor green = RgbwColor(0, brightnessDefault, 0, 0);
RgbwColor blue = RgbwColor(0, 0, brightnessDefault, 0);
RgbwColor white = RgbwColor(0, 0, 0, brightnessDefault);
RgbwColor black = RgbwColor(0, 0, 0, 0);

//HslColor hslRed(red);
//HslColor hslGreen(green);
//HslColor hslBlue(blue);
//HslColor hslWhite(white);
//HslColor hslBlack(black);

// Set status LED GPIO
const int ledPin = 23;

// Stores LED state
String ledState;

// LED colors
int hue;
int sat;
int val;
int r;
int g;
int b;
int w;
bool colorUpdate;


// Timing
long t;
long t_prev;



// Wi-Fi Definitions //////////////////////////////////////////////////////////////////////////
// Replace with your network credentials
//const char* ssid = CREDENTIALS_WIFI_SSID;
//const char* password = CREDENTIALS_WIFI_PASS;



// Server Definitions //////////////////////////////////////////////////////////////////////////
// Variable to hold HTTP request
const char* PARAM_INPUT_1 = "h";
const char* PARAM_INPUT_2 = "s";
const char* PARAM_INPUT_3 = "v";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Replaces placeholder with LED state value
String processor(const String& var) {
  Serial.println(var);
  if (var == "STATE") {
    if (digitalRead(ledPin)) {
      ledState = "ON";
    } else {
      ledState = "OFF";
    }
    Serial.print(ledState);
    return ledState;
  }
  return String();
}



// LED functions ////////////////////////////////////////////////////////////////////////////////

// Convert from HSV to RGB
void HSV_to_RGB(int hueIn, int satIn, int valIn) {
  int i;
  double f, p, q, t, h, s, v;

  h = max(0.0, min(360.0, (double) hueIn));
  s = max(0.0, min(100.0, (double) satIn));
  v = max(0.0, min(100.0, (double) valIn));

  s /= 100.0;
  v /= 100.0;

  if (s == 0) {
    // Achromatic (grey)
    r = 0;
    g = 0;
    b = 0;
    return;
  }

  h /= 60.0; // sector 0 to 5
  i = (int) floor(h);
  f = h - i; // factorial part of h
  p = v * (1 - s);
  q = v * (1 - s * f);
  t = v * (1 - s * (1 - f));
  switch (i) {
    case 1:
      r = (int) round(255 * q);
      g = (int) round(255 * v);
      b = (int) round(255 * p);
      break;
    case 2:
      r = (int) round(255 * p);
      g = (int) round(255 * v);
      b = (int) round(255 * t);
      break;
    case 3:
      r = (int) round(255 * p);
      g = (int) round(255 * q);
      b = (int) round(255 * v);
      break;
    case 4:
      r = (int) round(255 * t);
      g = (int) round(255 * p);
      b = (int) round(255 * v);
      break;
    case 5:
      r = (int) round(255 * v);
      g = (int) round(255 * p);
      b = (int) round(255 * q);
    default: // case 0
      r = (int) round(255 * v);
      g = (int) round(255 * t);
      b = (int) round(255 * p);
      break;
  }
}


// Convert from HSV to RGB
//void HSV_to_RGBW(int hueIn, int satIn, int valIn) {
//  HSV_to_RGB(hueIn, satIn, valIn);
//
//  double M = (double) max(r, max(g, b));
//  double m = (double) min(r, min(g, b));
//
//  if((m / M) < 0.5){
//    w = (int) ((m * M) / (M - m));
//  } else {
//    w = (int) M;
//  }
//  
//  double K = (w + M) / M;
//  r = (int) floor((K * r) - w);
//  g = (int) floor((K * g) - w);
//  b = (int) floor((K * b) - w);
//}


//void RGB_to_RGBW(int rIn, int gIn, int bIn){
//  // Get the maximum between R, G, and B
//  float tM = max(rIn, max(gIn, bIn));
//  
//  // If the maximum value is 0, immediately return pure black.
//  if(tM == 0){ 
//    r = 0;
//    g = 0;
//    b = 0;
//    w = 0;
//    return;
//  }
//  
//  // This section serves to figure out what the color with 100% hue is
//  float multiplier = 255.0 / tM;
//  float hR = rIn * multiplier;
//  float hG = gIn * multiplier;
//  float hB = bIn * multiplier;  
//  
//  // This calculates the Whiteness (not strictly speaking Luminance) of the color
//  float M = max(hR, max(hG, hB));
//  float m = min(hR, min(hG, hB));
//  float lum = ((M + m) - 255.0) / multiplier;
//  
//  // Calculate the output values
//  r = (int) (rIn - lum);
//  g = (int) (gIn - lum);
//  b = (int) (bIn - lum);
//  w = (int) lum;
//  
//  //Trim them so that they are all between 0 and 255
//  if (r < 0){
//    r = 0;
//  }
//  if (g < 0){
//    g = 0;
//  }
//  if (b < 0){
//    b = 0;
//  }
//  if (w < 0){
//    w = 0;
//  }
//  if (r > 255){
//    r = 255;
//  }
//  if (g > 255){
//    g = 255;
//  }
//  if (b > 255){
//    b = 255;
//  }
//  if (w > 255){
//    w = 255;
//  }
//}
//
//void HSV_to_RGBW(int hueIn, int satIn, int valIn){
//  HSV_to_RGB(hueIn, satIn, valIn);
//  //RGB_to_RGBW(r, g, b);
//
//  float lum = satIn / 100.0;
//  float bright = valIn / 100.0;
//  
//  r = (int) (bright * lum * r);
//  g = (int) (bright * lum * g);
//  b = (int) (bright * lum * b);
//  w = (int) (bright * (1.0 - lum));
//}

void handleColor(int hueIn, int satIn, int valIn) {
  HSV_to_RGB(hueIn, satIn, valIn);
  //RGB_to_RGBW(r, g, b);

  float lum = satIn / 100.0;
  float bright = valIn / 100.0;
  Serial.println(lum);
  Serial.println(bright);
  
  
  r = (int) (bright * lum * r);
  g = (int) (bright * lum * g);
  b = (int) (bright * lum * b);
  w = (int) 255 * (bright * (1.0 - lum));
}

// Set color of all pixels
void colorAll(RgbwColor color) {
  for (uint16_t i = 0; i < strip.PixelCount(); i++) {
    strip.SetPixelColor(i, color);
  }
  strip.Show();
}

// Fade from one color to another
void fade(RgbwColor colorStart, RgbwColor colorEnd, float progressInc, int waitMillis) {
  Serial.println("fade()");
  float progress = 0.0;
  while (progress <= 1.0) {
    Serial.print("\tProgress: ");
    Serial.println(progress);
    RgbwColor updatedColor = RgbwColor::LinearBlend(colorStart, colorEnd, progress);
    colorAll(updatedColor);
    delay(waitMillis);
    progress = progress + progressInc;
  }
}

// Cycle through brightness-levels and colors
void cycleColor(int brightness, int waitMillis) {
  Serial.println("cycleColor()");
  if (brightness > 255) {
    brightness = 255;
  }
  if (brightness < 0) {
    brightness = 0;
  }

  Serial.println("\tRED");
  colorAll(RgbwColor(brightness, 0, 0, 0)); // Full RED
  delay(waitMillis);

  Serial.println("\tGREEN");
  colorAll(RgbwColor(0, brightness, 0, 0)); // Full GREEN
  delay(waitMillis);

  Serial.println("\tBLUE");
  colorAll(RgbwColor(0, 0, brightness, 0)); // Full BLUE
  delay(waitMillis);

  Serial.println("\tWHITE");
  colorAll(RgbwColor(0, 0, 0, brightness)); // Full WHITE
  delay(waitMillis);
}

// Cycle through brightness-levels and colors
void cycleAll(int brightnessStart, int brightnessEnd, int steps, int waitFade, int waitColor) {
  Serial.println("cycleAll()");
  float progressInc = 1 / ((float) steps);

  Serial.println("\tRED");
  RgbwColor colorStart = RgbwColor(brightnessStart, 0, 0, 0); // RED
  RgbwColor colorEnd = RgbwColor(brightnessEnd, 0, 0, 0);
  fade(colorStart, colorEnd, progressInc, waitFade);
  delay(waitColor);

  Serial.println("\tBLUE");
  colorStart = RgbwColor(0, brightnessStart, 0, 0); // BLUE
  colorEnd = RgbwColor(0, brightnessEnd, 0, 0);
  fade(colorStart, colorEnd, progressInc, waitFade);
  delay(waitColor);

  Serial.println("\tGREEN");
  colorStart = RgbwColor(0, 0, brightnessStart, 0); // GREEN
  colorEnd = RgbwColor(0, 0, brightnessEnd, 0);
  fade(colorStart, colorEnd, progressInc, waitFade);
  delay(waitColor);

  Serial.println("\tWHITE");
  colorStart = RgbwColor(0, 0, 0, brightnessStart); // WHITE
  colorEnd = RgbwColor(0, 0, 0, brightnessEnd);
  fade(colorStart, colorEnd, progressInc, waitFade);
}

// Set pixels one at a time
void colorWipe(RgbwColor color, int wait) {
  Serial.println("colorWipe()");
  for (int i = 0; i < strip.PixelCount(); i++) { // For each pixel in strip...
    Serial.print("\tLED: ");
    Serial.println(i + 1);
    strip.SetPixelColor(i, color);         //  Set pixel's color (in RAM)
    strip.Show();                          //  Update strip to match
    delay(wait);                           //  Pause for a moment
  }
}

// Go through all 4 colors by setting pixels one at a time
void colorWipeAll(int brightness, int wait) {
  Serial.println("colorWipeAll()");

  Serial.print("\tRED");
  colorWipe(RgbwColor(brightness, 0, 0, 0), wait);

  Serial.print("\tBLUE");
  colorWipe(RgbwColor(0, brightness, 0, 0), wait);

  Serial.print("\tGREEN");
  colorWipe(RgbwColor(0, 0, brightness, 0), wait);

  Serial.print("\tWHITE");
  colorWipe(RgbwColor(0, 0, 0, brightness), wait);
}
















void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);

  Serial.println();
  Serial.println("Initializing...");
  Serial.flush();

  // Set LED output
  pinMode(ledPin, OUTPUT);



  // SPIFFS ////////////////////////////////////////////////////////////////////////////////
  // Initialize SPIFFS file server
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }



  // Wi-Fi ////////////////////////////////////////////////////////////////////////////////
//  // Connect to Wi-Fi
//  WiFi.begin(ssid, password);
//  while (WiFi.status() != WL_CONNECTED) {
//    delay(1000);
//    Serial.println("Connecting to WiFi..");
//  }
//
//  // Print IP address of web server on serial interface
//  Serial.println(WiFi.localIP());
  
  // WiFiManager
  // reset settings - for testing
  //wifiManager.resetSettings();

  // Sets timeout until configuration portal gets turned off
  // Useful to make it all retry or go to sleep
  // In seconds
  wifiManager.setTimeout(180);
  
  // Fetches ssid and pass and tries to connect
  // If it does not connect it starts an access point with the specified name "LightO"
  // Goes into a blocking loop awaiting configuration
  if(!wifiManager.autoConnect("LightO")) {
    Serial.println("Failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(5000);
  } 

  //if you get here you have connected to the WiFi
  Serial.println("WiFi configured and connected");



  // Server ////////////////////////////////////////////////////////////////////////////////
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/style.css", "text/css");
  });

  // Route to set GPIO to HIGH
  server.on("/on", HTTP_GET, [](AsyncWebServerRequest * request) {
    digitalWrite(ledPin, HIGH);
    Serial.println("Switch: ON");
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  // Route to set GPIO to LOW
  server.on("/off", HTTP_GET, [](AsyncWebServerRequest * request) {
    digitalWrite(ledPin, LOW);
    Serial.println("Switch: OFF");
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/favicon.ico", "image/png");
  });

  server.on("/get", HTTP_GET, [](AsyncWebServerRequest * request) {
    // Handle semaphore
    xSemaphoreTake(taskSemaphore, portMAX_DELAY);

    String getHue;
    String getSat;
    String getVal;
    String inputParam1;
    String inputParam2;
    String inputParam3;

    // GET input1 value on <ESP_IP>/get?h=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) {
      getHue = request->getParam(PARAM_INPUT_1)->value();
      hue = (int) getHue.toInt();
      inputParam1 = PARAM_INPUT_1;
    }

    // GET input2 value on <ESP_IP>/get?s=<inputMessage>
    if (request->hasParam(PARAM_INPUT_2)) {
      getSat = request->getParam(PARAM_INPUT_2)->value();
      sat = (int) getSat.toInt();
      inputParam2 = PARAM_INPUT_2;
    }

    // GET input3 value on <ESP_IP>/get?v=<inputMessage>
    if (request->hasParam(PARAM_INPUT_3)) {
      getVal = request->getParam(PARAM_INPUT_3)->value();
      val = (int) getVal.toInt();
      inputParam3 = PARAM_INPUT_3;
    }
    
    Serial.println("Got from webpage:");
    Serial.print("Hue = ");
    Serial.print(hue);
    Serial.print(", Sat = ");
    Serial.print(sat);
    Serial.print(", Val = ");
    Serial.print(val);
    Serial.println();
    
    request->send(200);

    // Set flag
    colorUpdate = true;

    // Make semaphore available
    xSemaphoreGive(taskSemaphore);
  });

  // Start server
  server.begin();

  // Set up mDNS responder:
  // - first argument is the domain name, in this example
  //   the fully-qualified domain name is "LightO.local"
  // - second argument is the IP address to advertise
  //   we send our IP address on the WiFi network
  if (!MDNS.begin("LightO")) {
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");

  // Start TCP (HTTP) server
  server.begin();
  Serial.println("TCP server started");

  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);



  // LED Stuff ////////////////////////////////////////////////////////////////////////////////
  Serial.println("Setting up LEDs...");
  // Reset all the pixels to the off state
  strip.Begin();
  strip.Show();



  // Multi-core stuff ////////////////////////////////////////////////////////////////////////////////
  Serial.println("Finalising and starting multi-core execution...");
  
  taskSemaphore = xSemaphoreCreateMutex();
  
  // Create a task that will be executed in the taskCore0Handler() function, with priority 1 and executed on core 0
  xTaskCreatePinnedToCore(
    &taskCore0Handler,  /* Task function. */
    "task0",            /* name of task. */
    10000,              /* Stack size of task */
    NULL,               /* parameter of the task */
    1,                  /* priority of the task */
    &taskCore0,         /* Task handle to keep track of created task */
    0);                 /* pin task to core 0 */
  delay(500);
  // Create a task that will be executed in the taskCore1Handler() function, with priority 1 and executed on core 0
  xTaskCreatePinnedToCore(
    &taskCore1Handler,  /* Task function. */
    "task1",            /* name of task. */
    10000,              /* Stack size of task */
    NULL,               /* parameter of the task */
    1,                  /* priority of the task */
    &taskCore1,         /* Task handle to keep track of created task */
    1);                 /* pin task to core 0 */

}

void taskCore0Handler(void *pvParameters) {
  Serial.print("LED handler running on core ");
  Serial.println(xPortGetCoreID());

  while (true) {
    // Handle semaphore and update LEDS
    xSemaphoreTake(taskSemaphore, portMAX_DELAY);
    LEDHandler();
    xSemaphoreGive(taskSemaphore);

    // Delay for watchdog
    vTaskDelay(10);
  }
}


void taskCore1Handler(void *pvParameters) {
  Serial.print("Webpage running on core ");
  Serial.println(xPortGetCoreID());

  while (true) {
    //xSemaphoreTake(taskSemaphore, portMAX_DELAY);
    //WebHandler();
    //xSemaphoreGive(taskSemaphore);

    // Delay for watchdog
    vTaskDelay(10);
  }
}


void LEDHandler() {
//  Serial.println("Cycling through colors");
//  // brightness, wait
//  cycleColor(50, 1000);
//  colorAll(black);
//  delay(3000);
//
//  Serial.println("Cycling through colors and fading brightness");
//  // brightnessStart, brightnessEnd, steps, waitFade, waitColor
//  cycleAll(0, 255, 100, 25, 0);
//  colorAll(black);
//  delay(3000);
//
//  Serial.println("Wiping all colors");
//  // brightness, wait
//  colorWipeAll(50, 25);
//  colorAll(black);
//  delay(3000);

  if(colorUpdate){
    // Convert colorspace
    handleColor(hue, sat, val);
    
    Serial.print("Updating color to R = ");
    Serial.print(r);
    Serial.print(", G = ");
    Serial.print(g);
    Serial.print(", B = ");
    Serial.print(b);
    Serial.print(", W = ");
    Serial.print(w);
    Serial.println();
    Serial.println();

    // Set color
    RgbwColor color = RgbwColor(r, g, b, w);
    colorAll(color);

    // Reset flag
    colorUpdate = false;
  }
}


void WebHandler(){
  //
}


void loop()
{
  // Empty
  delay(1000);
}