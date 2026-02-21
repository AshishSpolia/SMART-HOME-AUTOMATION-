#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_NeoPixel.h>

#define PIN        13 
#define NUMPIXELS  12 
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

WebServer server(80);

// Global Control Variables
int currentMode = 0; 
int globalBrightness = 150;
int animationSpeed = 50; // Delay in ms (lower is faster)
uint32_t ringColor = pixels.Color(0, 255, 255); 
unsigned long lastUpdate = 0;
unsigned long lastModeChange = 0;
int patternStep = 0;

void setup() {
  Serial.begin(115200);
  pixels.begin();
  pixels.setBrightness(globalBrightness);
  
  WiFi.begin("CMF", "1234567890"); 
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/setMode", []() {
    currentMode = server.arg("m").toInt();
    patternStep = 0;
    server.send(200, "text/plain", "OK");
  });
  server.on("/setColor", []() {
    String hex = server.arg("c");
    long number = strtol(&hex[1], NULL, 16);
    ringColor = pixels.Color((number >> 16) & 0xFF, (number >> 8) & 0xFF, number & 0xFF);
    currentMode = 0; 
    server.send(200, "text/plain", "OK");
  });
  server.on("/setBright", []() {
    globalBrightness = server.arg("b").toInt();
    pixels.setBrightness(globalBrightness);
    server.send(200, "text/plain", "OK");
  });
  server.on("/setSpeed", []() {
    animationSpeed = server.arg("s").toInt();
    server.send(200, "text/plain", "OK");
  });

  server.begin();
}

void loop() {
  server.handleClient();
  runAnimation();
}

void runAnimation() {
  if (millis() - lastUpdate < animationSpeed) return; 
  lastUpdate = millis();

  if (currentMode == 21) { 
    if (millis() - lastModeChange > 6000) { lastModeChange = millis(); }
    executePattern(((millis() - lastModeChange) / 6000 % 20) + 1);
  } else {
    executePattern(currentMode);
  }
}

void executePattern(int mode) {
  patternStep++;
  switch (mode) {
    case 0:  staticColor(); break;
    case 1:  rainbow(); break;
    case 2:  theaterChase(); break;
    case 3:  bounce(); break;
    case 4:  scanner(); break;
    case 5:  strobe(); break;
    case 6:  sparkle(); break;
    case 7:  fire(); break;
    case 8:  police(); break;
    case 9:  breathing(); break;
    case 10: meteor(); break;
    case 11: heartbeat(); break;
    case 12: ocean(); break;
    case 13: forest(); break;
    case 14: plasma(); break;
    case 15: comet(); break; // FIXED
    case 16: glitch(); break;
    case 17: orbit(); break;
    case 18: candy(); break;
    case 19: sunrise(); break;
    case 20: lightning(); break;
  }
}

// --- Custom Helper for Fading ---
void fadeAll(int factor) {
  for(int i=0; i<NUMPIXELS; i++) {
    uint32_t c = pixels.getPixelColor(i);
    uint8_t r = (uint8_t)(c >> 16), g = (uint8_t)(c >> 8), b = (uint8_t)c;
    pixels.setPixelColor(i, r > factor ? r-factor : 0, g > factor ? g-factor : 0, b > factor ? b-factor : 0);
  }
}

// --- Animation Library ---
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) return pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  if(WheelPos < 170) { WheelPos -= 85; return pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3); }
  WheelPos -= 170; return pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void staticColor() { for(int i=0; i<NUMPIXELS; i++) pixels.setPixelColor(i, ringColor); pixels.show(); }
void rainbow() { for(int i=0; i<NUMPIXELS; i++) pixels.setPixelColor(i, Wheel(((i * 256 / NUMPIXELS) + patternStep) & 255)); pixels.show(); }
void theaterChase() { for(int i=0; i<NUMPIXELS; i++) pixels.setPixelColor(i, (i + (patternStep/5)) % 3 == 0 ? ringColor : 0); pixels.show(); }
void bounce() { int pos = abs(((patternStep/2) % (NUMPIXELS * 2)) - NUMPIXELS); pixels.clear(); pixels.setPixelColor(pos, ringColor); pixels.show(); }
void scanner() { int pos = (patternStep/2) % NUMPIXELS; pixels.clear(); pixels.setPixelColor(pos, ringColor); pixels.setPixelColor((pos+1)%NUMPIXELS, ringColor); pixels.show(); }
void strobe() { if((patternStep/10) % 2 == 0) staticColor(); else { pixels.clear(); pixels.show(); } }
void sparkle() { pixels.clear(); pixels.setPixelColor(random(NUMPIXELS), ringColor); pixels.show(); }
void fire() { for(int i=0; i<NUMPIXELS; i++) pixels.setPixelColor(i, pixels.Color(255, random(80), 0)); pixels.show(); }
void police() { pixels.clear(); if((patternStep/10) % 2 == 0) { for(int i=0; i<NUMPIXELS/2; i++) pixels.setPixelColor(i, 255, 0, 0); } else { for(int i=NUMPIXELS/2; i<NUMPIXELS; i++) pixels.setPixelColor(i, 0, 0, 255); } pixels.show(); }
void breathing() { float val = (exp(sin(patternStep/20.0*PI)) - 0.36787944)*108.0; pixels.setBrightness(val * globalBrightness / 255); staticColor(); pixels.setBrightness(globalBrightness); }
void meteor() { for(int i=0; i<NUMPIXELS; i++) { pixels.setPixelColor(i, (i == (patternStep % NUMPIXELS)) ? ringColor : 0); } pixels.show(); }
void heartbeat() { float b = (sin(patternStep * 0.1) > 0) ? pow(sin(patternStep * 0.1), 10) : 0; for(int i=0; i<NUMPIXELS; i++) pixels.setPixelColor(i, pixels.Color(b*255, 0, 0)); pixels.show(); }
void ocean() { for(int i=0; i<NUMPIXELS; i++) pixels.setPixelColor(i, pixels.Color(0, random(50,150), 255)); pixels.show(); }
void forest() { for(int i=0; i<NUMPIXELS; i++) pixels.setPixelColor(i, pixels.Color(0, 255, random(50))); pixels.show(); }
void plasma() { for(int i=0; i<NUMPIXELS; i++) pixels.setPixelColor(i, Wheel((int)(128 + 127 * sin(i/4.0 + patternStep/10.0)))); pixels.show(); }
void comet() { fadeAll(20); pixels.setPixelColor(patternStep % NUMPIXELS, ringColor); pixels.show(); }
void glitch() { if(random(10)>8) { for(int i=0; i<NUMPIXELS; i++) pixels.setPixelColor(i, random(0xFFFFFF)); } else { pixels.clear(); } pixels.show(); }
void orbit() { pixels.clear(); pixels.setPixelColor(patternStep % NUMPIXELS, ringColor); pixels.setPixelColor((patternStep + NUMPIXELS/2) % NUMPIXELS, 0xFFFFFF); pixels.show(); }
void candy() { for(int i=0; i<NUMPIXELS; i++) pixels.setPixelColor(i, (i+patternStep/5)%2==0 ? 0xFF007F : 0xFFFFFF); pixels.show(); }
void sunrise() { int r = min(patternStep, 255); int g = min(max(0, patternStep-100), 150); for(int i=0; i<NUMPIXELS; i++) pixels.setPixelColor(i, r, g, 0); pixels.show(); if(patternStep>350) patternStep=0; }
void lightning() { if(random(50) == 1) { for(int i=0; i<NUMPIXELS; i++) pixels.setPixelColor(i, 255, 255, 255); pixels.show(); delay(20); } pixels.clear(); pixels.show(); }

// --- Professional Dashboard ---
void handleRoot() {
  String names[] = {"", "Rainbow", "Chase", "Bounce", "Scanner", "Strobe", "Sparkle", "Fire", "Police", "Breathe", "Meteor", "Heart", "Ocean", "Forest", "Plasma", "Comet", "Glitch", "Orbit", "Candy", "Sunrise", "Bolt"};
  String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1.0'><style>";
  html += "body { background: #0f0c29; background: linear-gradient(to bottom, #1a1a2e, #16213e); color: white; font-family: 'Segoe UI', sans-serif; text-align: center; margin: 0; padding: 15px; }";
  html += ".card { background: rgba(255,255,255,0.05); backdrop-filter: blur(15px); border-radius: 25px; padding: 20px; border: 1px solid rgba(255,255,255,0.1); max-width: 450px; margin: auto; }";
  html += ".wheel-container { margin: 15px auto; width: 140px; height: 140px; border-radius: 50%; border: 3px solid #00dbde; overflow: hidden; position: relative; }";
  html += "input[type='color'] { width: 200%; height: 200%; cursor: pointer; border: none; position: absolute; top: -50%; left: -50%; }";
  html += ".grid { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; margin-top: 15px; }";
  html += "button { padding: 14px; border-radius: 12px; border: none; background: rgba(255,255,255,0.08); color: #eee; font-weight: 500; cursor: pointer; }";
  html += ".auto-btn { grid-column: span 2; background: linear-gradient(45deg, #00dbde, #fc00ff); font-weight: bold; }";
  html += "input[type='range'] { width: 100%; margin: 10px 0; accent-color: #00dbde; }";
  html += ".label { font-size: 12px; text-transform: uppercase; color: #888; margin-top: 10px;}";
  html += "</style></head><body><div class='card'><h1>PRO LUX</h1>";
  html += "<div class='wheel-container'><input type='color' value='#00ffff' oninput='cmd(\"/setColor?c=\"+encodeURIComponent(this.value))'></div>";
  html += "<div class='label'>Brightness</div><input type='range' min='10' max='255' value='"+String(globalBrightness)+"' onchange='cmd(\"/setBright?b=\"+this.value)'>";
  html += "<div class='label'>Speed</div><input type='range' min='10' max='200' step='10' value='"+String(animationSpeed)+"' dir='rtl' onchange='cmd(\"/setSpeed?s=\"+this.value)'>";
  html += "<div class='grid'><button class='auto-btn' onclick='cmd(\"/setMode?m=21\")'>🚀 AUTO PILOT 2.0</button>";
  for(int i=1; i<=20; i++) { html += "<button onclick='cmd(\"/setMode?m="+String(i)+"\")'>"+names[i]+"</button>"; }
  html += "</div></div><script>function cmd(url){fetch(url);}</script></body></html>";
  server.send(200, "text/html", html);
}