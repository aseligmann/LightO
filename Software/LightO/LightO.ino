// Import required libraries
#include "WiFi.h"
#include <ESPmDNS.h> // For user-friendly web address
#include <WiFiClient.h>
//// https://github.com/me-no-dev/ESPAsyncWebServer/issues/418#issuecomment-667976368
#include <WiFiManager.h> // Captive AP to configure new network
WiFiManager wifiManager;
#define WEBSERVER_H
#include "ESPAsyncWebServer.h" // Web server
////
#include "SPIFFS.h" // Data storage
#include "math.h"
#include <NeoPixelBus.h> // LED control


// Debugging //////////////////////////////////////////////////////////////////////////
bool debug = false;


// Multi-core definitions //////////////////////////////////////////////////////////////////////////
TaskHandle_t taskCore0;
TaskHandle_t taskCore1;
SemaphoreHandle_t taskSemaphore;



// LED Definitions //////////////////////////////////////////////////////////////////////////
#define NUM_LEDS 61
#define DATA_PIN 32

#define brightnessDefault 255

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
const int switchPin = 23;

// Stores LED state
String ledState;

// LED colors
int hue = 0;
int sat = 100;
int val = 100;
int r;
int g;
int b;
int w;
bool colorUpdate;

//// Modes
// Colorwheel
// Candle
int timeCandleWaitR = 0;
int timeCandleWaitG = 0;
int timeCandleR = 0;
int timeCandleG = 0;
// Cycle
int timeCycleStep = 50;
float hueFloat = 0;

// Timing
long t;
long t_prev;


// Server Definitions //////////////////////////////////////////////////////////////////////////
// Variable to hold HTTP request
const char* PARAM_INPUT_1 = "h";
const char* PARAM_INPUT_2 = "s";
const char* PARAM_INPUT_3 = "v";
const char* PARAM_MODE = "mode";

int lampMode = 0;
// 0 = setup
// 1 = colorwheel
// 2 = candle
// 3 = cycle
 
// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Toggle GPIO
String handleSwitch(const String& var) {
  Serial.println(var);
  if (var == "STATE") {
    if (digitalRead(switchPin)) {
      ledState = "ON";
    } else {
      ledState = "OFF";
    }
    Serial.print(ledState);
    return ledState;
  }
  return String();
}

int lastColorUpdate = 0;
int intervalColorUpdate = 100; // ms


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

  if (debug) {
    Serial.println("-------------- HSV_to_RGB --------------");
    Serial.print("h = ");
    Serial.print(h);
    Serial.print(", i = ");
    Serial.print(i);
    Serial.print(", f = ");
    Serial.println(f);
    Serial.print("v = ");
    Serial.print(v);
    Serial.print(", p = ");
    Serial.print(p);
    Serial.print(", q = ");
    Serial.print(q);
    Serial.print(", t = ");
    Serial.print(t);
    Serial.println();
  }
  
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
      break;
    default: // case 0
      r = (int) round(255 * v);
      g = (int) round(255 * t);
      b = (int) round(255 * p);
      break;
  }
  
  if (debug) {
    Serial.println("-------------- HSV_to_RGB --------------");
    Serial.print("R = ");
    Serial.print(r);
    Serial.print(", G = ");
    Serial.print(g);
    Serial.print(", B = ");
    Serial.print(b);
    Serial.println();
  }
}


// Handle input color
void handleColor(int hueIn, int satIn, int valIn) {
  // We interpret hue as color, saturation as luminance and value as brightness.
  // This is a bit unconventional, but makes for a better user experience
  
  if (debug) {
    Serial.println("============== handleColor INPUT ==============");
    Serial.print("Hue = ");
    Serial.print(hueIn);
    Serial.print(", Sat = ");
    Serial.print(satIn);
    Serial.print(", Val = ");
    Serial.print(valIn);
    Serial.println();
  }
  
  HSV_to_RGB(hueIn, satIn, valIn);
  //RGB_to_RGBW(r, g, b);
  
  float lum = satIn / 100.0;
  float bright = valIn / 100.0;
  
  r = (int) (bright * lum * r);
  g = (int) (bright * lum * g);
  b = (int) (bright * lum * b);
  w = (int) 255 * (bright * (1.0 - lum));

  if (debug) {
    Serial.println("-------------- handleColor OUTPUT --------------");
    Serial.print("R = ");
    Serial.print(r);
    Serial.print(", G = ");
    Serial.print(g);
    Serial.print(", B = ");
    Serial.print(b);
    Serial.print(", W = ");
    Serial.print(w);
    Serial.println();
    Serial.println("===============================================");
    Serial.println();
  }
}


// Convenience function for setting hue, sat and val, and setting the colorUpdate flag
void colorSet(int hueIn, int satIn, int valIn){
  hue = hueIn;
  sat = satIn;
  val = valIn;
  colorUpdate = true;
}


// Set color of select pixels
void colorSelect(RgbwColor color, int pxStart, int pxEnd) {
  for (uint16_t i = pxStart; i < pxEnd; i++) {
    strip.SetPixelColor(i, color);
  }
  strip.Show();
}


// Set color of all pixels
void colorAll(RgbwColor color) {
  colorSelect(color, 0, strip.PixelCount());
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
  pinMode(switchPin, OUTPUT);

  lampMode = 0;



  // LED stuff ////////////////////////////////////////////////////////////////////////////////
  Serial.println("Setting up LEDs...");
  // Reset all the pixels to the off state
  strip.Begin();
  strip.Show();
  RgbwColor colorWiFi = RgbwColor(0, 0, 255, 0);
  RgbwColor colorServer = RgbwColor(0, 255, 255, 0);
  RgbwColor colorDone = RgbwColor(0, 255, 0, 0);
  RgbwColor colorFailure = RgbwColor(255, 0, 0, 0);
  RgbwColor colorOff = RgbwColor(0, 0, 0, 0);



  // SPIFFS ////////////////////////////////////////////////////////////////////////////////
  // Initialize SPIFFS file server
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }



  // Wi-Fi ////////////////////////////////////////////////////////////////////////////////
  // Wi-Fi feedback
  Serial.println("Set leds blue for Wi-Fi stage...");
  colorSelect(colorWiFi, 45, strip.PixelCount()); // Outermost ring
  colorSelect(colorOff, 31, 45); // Next-outermost ring
  int timeWiFi = millis(); // Track time
  
  // Connect to Wi-Fi
  //WiFi.begin(ssid, password);
  //while (WiFi.status() != WL_CONNECTED) {
  //  delay(1000);
  //  Serial.println("Connecting to WiFi..");
  //}

  // Print IP address of web server on serial interface
  //Serial.println(WiFi.localIP());
  
  // WiFiManager
  // reset settings - for testing
  //wifiManager.resetSettings();

  // Sets timeout until configuration portal gets turned off
  // Useful to make it all retry or go to sleep
  // In seconds
  wifiManager.setTimeout(180);
  wifiManager.setClass("invert"); // Dark theme
  
  // Fetches ssid and pass and tries to connect
  // If it does not connect it starts an access point with the specified name "LightO"
  // Goes into a blocking loop awaiting configuration
  if(!wifiManager.autoConnect("LightO")) {
    colorSelect(colorFailure, 31, 45); // Next-outermost ring
    Serial.println("Failed to connect and hit timeout");
    delay(3000);
    // Reset and try again
    ESP.restart();
    delay(5000);
  } 

  Serial.println("WiFi configured and connected");
  while(millis() - timeWiFi < 200) {
    // Wait for 200 ms to have passed since showing WiFi feedback
    // Purely to avoid flashing LEDs too quickly as the user won't notice
  }



  // Server ////////////////////////////////////////////////////////////////////////////////
  // Server feedback
  Serial.println("Set leds teal for Server stage...");
  colorSelect(colorServer, 45, strip.PixelCount()); // Outermost ring
  colorSelect(colorOff, 31, 45); // Next-outermost ring
  int timeServer = millis(); // Track time
  
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/index.html", String(), false, handleSwitch);
  });

  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/style.css", "text/css");
  });

  // Route to set GPIO to HIGH
  server.on("/on", HTTP_GET, [](AsyncWebServerRequest * request) {
    digitalWrite(switchPin, HIGH);
    Serial.println("Switch: ON");
    request->send(SPIFFS, "/index.html", String(), false, handleSwitch);
  });

  // Route to set GPIO to LOW
  server.on("/off", HTTP_GET, [](AsyncWebServerRequest * request) {
    digitalWrite(switchPin, LOW);
    Serial.println("Switch: OFF");
    request->send(SPIFFS, "/index.html", String(), false, handleSwitch);
  });

  // Change mode
  server.on("/mode", HTTP_GET, [](AsyncWebServerRequest * request) {
    
    String getMode;
    
    // GET input1 value on <ESP_IP>/get?h=<inputMessage>
    if (request->hasParam(PARAM_MODE)) {
      getMode = request->getParam(PARAM_MODE)->value();
      lampMode = (int) getMode.toInt();
      hueFloat = hue;
    }

    if (debug) {
      Serial.println("=============== WebServer /mode ===============");
      Serial.print("Time = ");
      Serial.println(millis());
      Serial.println("Got new mode:");
      Serial.print("Mode = ");
      Serial.print(lampMode);
      Serial.println();
      Serial.println("===============================================");
      Serial.println();
    }
    
    request->send(200);
  });

  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/favicon.ico", "image/png");
  });

  server.on("/stateHSV", HTTP_GET, [](AsyncWebServerRequest *request){
    char state[12];
    sprintf(state, "%d,%d,%d", hue, sat, val);

    if (debug) {
      Serial.println("=============== WebServer /stateHSV ===============");
      Serial.print("Time = ");
      Serial.println(millis());
      Serial.println("Got request for COLOR state. Sending:");
      Serial.print("Hue = ");
      Serial.print(hue);
      Serial.print(", Sat = ");
      Serial.print(sat);
      Serial.print(", Val = ");
      Serial.print(val);
      Serial.println();
      Serial.println("===============================================");
      Serial.println();
    }
    
    request->send(200, "text/plain", state);
  });

  server.on("/stateMode", HTTP_GET, [](AsyncWebServerRequest *request){
    char state[12];
    sprintf(state, "%d", lampMode);

    if (debug) {
      Serial.println("=============== WebServer /stateMode ===============");
      Serial.print("Time = ");
      Serial.println(millis());
      Serial.println("Got request for MODE state. Sending:");
      Serial.print("Mode = ");
      Serial.print(lampMode);
      Serial.println();
      Serial.println("===============================================");
      Serial.println();
    }
    
    request->send(200, "text/plain", state);
  });

  server.on("/get", HTTP_GET, [](AsyncWebServerRequest * request) {
    // Handle semaphore
    xSemaphoreTake(taskSemaphore, portMAX_DELAY);

    String getHue;
    String getSat;
    String getVal;
    
    // GET input1 value on <ESP_IP>/get?h=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) {
      getHue = request->getParam(PARAM_INPUT_1)->value();
      hue = (int) getHue.toInt();
    }

    // GET input2 value on <ESP_IP>/get?s=<inputMessage>
    if (request->hasParam(PARAM_INPUT_2)) {
      getSat = request->getParam(PARAM_INPUT_2)->value();
      sat = (int) getSat.toInt();
    }

    // GET input3 value on <ESP_IP>/get?v=<inputMessage>
    if (request->hasParam(PARAM_INPUT_3)) {
      getVal = request->getParam(PARAM_INPUT_3)->value();
      val = (int) getVal.toInt();
    }
    
    // Set flag
    colorUpdate = true;
    
    if (debug) {
      Serial.println("=============== WebServer /get ===============");
      Serial.print("Time = ");
      Serial.println(millis());
      Serial.println("Got new color:");
      Serial.print("Hue = ");
      Serial.print(hue);
      Serial.print(", Sat = ");
      Serial.print(sat);
      Serial.print(", Val = ");
      Serial.print(val);
      Serial.println();
      Serial.println("===============================================");
      Serial.println();
    }
    
    request->send(200);

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
      colorSelect(colorFailure, 31, 45); // Next-outermost ring
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");

  // Start TCP (HTTP) server
  server.begin();
  Serial.println("TCP server started");

  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);
  
  Serial.println("WiFi configured and launched");
  while(millis() - timeServer < 200) {
    // Wait for 200 ms to have passed since showing WiFi feedback
    // Purely to avoid flashing LEDs too quickly as the user won't notice
  }



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



  // Boot feedback ////////////////////////////////////////////////////////////////////////////////
  Serial.println("Flashing leds green to indicate boot succesful...");
  colorSelect(colorDone, 45, strip.PixelCount()); // Outermost ring
  colorSelect(colorOff, 31, 45); // Next-outermost ring
  delay(200);
  colorSelect(colorOff, 45, strip.PixelCount()); // Outermost ring
  delay(200);
  
  lampMode = 1;
  colorSet(0, 0, 100); // White

  Serial.println("Done booting.");
  
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
  switch (lampMode) {
    case 0:
      handleModeSetup();
      break;
    case 1:
      handleModeColorWheel();
      break;
    case 2:
      handleModeCandle();
      break;
    case 3:
      handleModeCycle();
      break;
  }
}

void handleModeSetup() {
  if (colorUpdate) {
    int timeNow = millis();
    if (timeNow - lastColorUpdate > intervalColorUpdate) {
      // Convert colorspace
      handleColor(hue, sat, val);
      
      if (debug) {
        Serial.println("================= Setup =================");
        Serial.print("Updating color to R = ");
        Serial.print(r);
        Serial.print(", G = ");
        Serial.print(g);
        Serial.print(", B = ");
        Serial.print(b);
        Serial.print(", W = ");
        Serial.print(w);
        Serial.println();
        Serial.println("===============================================");
        Serial.println();
      }
    
      // Set color
      RgbwColor color = RgbwColor(r, g, b, w);
      colorSelect(color, 45, strip.PixelCount());
    
      // Reset flag
      colorUpdate = false;
      
      // Reset time
      lastColorUpdate = timeNow;
    }
  }
}

void handleModeColorWheel() {
  if (colorUpdate) {
    int timeNow = millis();
    if (timeNow - lastColorUpdate > intervalColorUpdate) {
      // Convert colorspace
      handleColor(hue, sat, val);
      
      if (debug) {
        Serial.println("================= ColorWheel =================");
        Serial.print("Updating color to R = ");
        Serial.print(r);
        Serial.print(", G = ");
        Serial.print(g);
        Serial.print(", B = ");
        Serial.print(b);
        Serial.print(", W = ");
        Serial.print(w);
        Serial.println();
        Serial.println("===============================================");
        Serial.println();
      }
    
      // Set color
      RgbwColor color = RgbwColor(r, g, b, w);
      colorAll(color);
    
      // Reset flag
      colorUpdate = false;
      
      // Reset time
      lastColorUpdate = timeNow;
    }
  }
}

void handleModeCandle() {
  // Inspired by
  // https://github.com/grantwinney/52-Weeks-of-Pi/blob/master/07-Candle-Simulation-on-RGB-LED/CandleSimulation.py
  float intensity = 1.0;
  int timeNow = millis();

  // Randomize red channel
  if (timeNow - timeCandleR > timeCandleWaitR) {
    //float candle_r = min(((int) (random(75, 100) * pow(intensity + 0.1, 0.75))), 100); // Value from 0 - 100
    //r = (candle_r * 255) / 100;
    int r_min = 130;
    int r_max = 255;
    int r_target = (r_min + r_max) / 2;
    r = min(max(  (int) (r + random(-2, 2) + random(((r_target - r)/16) - 2, ((r_target - r)/16) + 2))  , r_min), r_max);
    
    timeCandleWaitR = random(30, 100); // Wait 80 to 300 millis
    
    timeCandleR = timeNow;
    colorUpdate = true;
  }
  
  // Randomize green channel
  if (timeNow - timeCandleG > timeCandleWaitG) {
    //float candle_g = random(33, 44) * pow(intensity, 2); // Value from 0 - 100
    //g = (candle_g * 255) / 100;
    int g_min = 15;
    int g_max = 55;
    int g_target = (g_min + g_max) / 2;
    g = min(max(  (int) (g + random(-3, 3) + random(((g_target - g)/8) - 2, ((g_target - g)/8) + 2))  , g_min), g_max);
    
    timeCandleWaitG = random(30, 80); // Wait 80 to 300 millis
    
    timeCandleG = timeNow;
    colorUpdate = true;
  }


  // Set value of unused color channels
  b = 0;
  w = 0;


  if (colorUpdate) {
    if (debug) {
      Serial.println("================= Candle =================");
      Serial.print("Updating color to R = ");
      Serial.print(r);
      Serial.print(", G = ");
      Serial.print(g);
      Serial.print(", B = ");
      Serial.print(b);
      Serial.print(", W = ");
      Serial.print(w);
      Serial.println();
      Serial.println("===============================================");
      Serial.println();
    }
    
    // Set color
    RgbwColor color = RgbwColor(r, g, b, w);
    colorAll(color);
  
    // Reset flag
    colorUpdate = false;
  }
}

void handleModeCycle() {
  int timeNow = millis();

  if (timeNow - timeCycleStep > lastColorUpdate) {
    // Time to do one full cycle
    int timeCycleFull = 60000; // 60 seconds in millis
    // Stepsize
    float stepSize = (360.0 / timeCycleFull) * timeCycleStep;
  
    // Increment hue
    hueFloat = hueFloat + stepSize;
    if (hueFloat >= 360) {
      hueFloat = hueFloat - 360;
    }
    hue = floor(hueFloat);
    
    sat = 100;
    val = 100;
    
    // Convert colorspace
    handleColor(hue, sat, val);
  
    
    if (debug) {
      Serial.println("================= Cycle =================");
      Serial.print("hueFloat = ");
      Serial.println(hueFloat);
      Serial.print("Updating color to R = ");
      Serial.print(r);
      Serial.print(", G = ");
      Serial.print(g);
      Serial.print(", B = ");
      Serial.print(b);
      Serial.print(", W = ");
      Serial.print(w);
      Serial.println();
      Serial.println("===============================================");
      Serial.println();
    }
      
    // Set color
    RgbwColor color = RgbwColor(r, g, b, w);
    colorAll(color);
      
    // Reset time
    lastColorUpdate = timeNow;
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
