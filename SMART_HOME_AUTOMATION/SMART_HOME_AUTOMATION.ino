#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>
#include <ESP32Servo.h>
#include <Adafruit_NeoPixel.h>

// --- 1. CONFIGURATION ---
const char* ssid = "CMF";
const char* password = "1234567890";

// --- 2. PIN DEFINITIONS ---
#define DHTPIN 4
#define DHTTYPE DHT22
#define MQ7_PIN 34
#define MQ8_PIN 35
#define FLAME_PIN 27
#define LDR_PIN 32
#define BUZZER_PIN 25
#define LASER_PIN 26
#define PIXEL_PIN 2 
#define NUMPIXELS 12
const int servoPin = 18;
const int trigPin = 5;
const int echoPin = 19;

// Relays (8-Channel) - Relay index [1] (GPIO 12) is Exhaust
const int relayPins[8] = {13, 12, 14, 23, 22, 21, 33, 15};
bool relayStates[8] = {false, false, false, false, false, false, false, false};

// --- 3. OBJECTS & GLOBAL VARIABLES ---
DHT dht(DHTPIN, DHTTYPE);
Servo myServo;
WebServer server(80);
Adafruit_NeoPixel pixels(NUMPIXELS, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

int currentAngle = 0, currentDistance = 0, sweepDirection = 1;
int currentMode = 0, globalBrightness = 150, animationSpeed = 50;
uint32_t ringColor = pixels.Color(0, 255, 255);
unsigned long lastRadar = 0, lastPixel = 0, patternStep = 0;

// --- 4. WEB UI ---
const char PAGE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta name="viewport" content="width=device-width, initial-scale=1">
<title>ULTIMATE HUB 2.0</title>
<style>
  :root { --primary: #00ff41; --accent: #00dbde; --bg: #050505; }
  body { font-family: 'Segoe UI', sans-serif; background: var(--bg); color: #fff; margin: 0; padding: 15px; }
  .layout { display: flex; flex-wrap: wrap; gap: 15px; justify-content: center; }
  .card { background: rgba(255,255,255,0.05); backdrop-filter: blur(10px); padding: 15px; border-radius: 20px; border: 1px solid rgba(255,255,255,0.1); flex: 1; min-width: 300px; }
  canvas { background: #000; border: 1px solid #004400; border-radius: 15px; width: 100%; }
  .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(120px, 1fr)); gap: 10px; }
  .val { font-size: 1.8rem; font-weight: 800; color: var(--primary); display: block; }
  .label { font-size: 0.7rem; color: #888; text-transform: uppercase; margin-bottom: 5px; }
  .btn { width: 100%; border: none; padding: 12px; border-radius: 10px; cursor: pointer; background: #222; color: #fff; font-weight: bold; }
  .btn-on { background: #10b981; box-shadow: 0 0 10px #10b981; } .lux-btn { background: linear-gradient(45deg, #00dbde, #fc00ff); }
  input[type='range'] { width: 100%; accent-color: var(--accent); }
</style></head>
<body>
  <div class="layout">
    <div class="card" style="max-width:600px;">
      <span class="label">Radar Sweep</span><canvas id="radarCanvas"></canvas>
      <div style="margin-top:10px;">ANG: <span id="ang">0</span>° | DST: <span id="dst">0</span> cm</div>
    </div>
    <div class="card" style="max-width:350px;">
      <span class="label">Pro Lux Lighting</span>
      <input type="color" value="#00ffff" style="width:100%; height:40px; border:none; border-radius:10px;" oninput="fetch('/setColor?c='+encodeURIComponent(this.value))">
      <div class="label" style="margin-top:10px;">Brightness</div>
      <input type="range" min="10" max="255" onchange="fetch('/setBright?b='+this.value)">
      <div class="grid" style="margin-top:15px;">
        <button class="lux-btn" style="grid-column: span 2;" onclick="fetch('/setMode?m=21')">AUTO PILOT</button>
        <button onclick="fetch('/setMode?m=1')">Rainbow</button><button onclick="fetch('/setMode?m=11')">Heart</button>
        <button onclick="fetch('/setMode?m=7')">Fire</button><button onclick="fetch('/setMode?m=20')">Bolt</button>
      </div>
    </div>
    <div class="card">
      <div class="grid">
        <div class="card"><span class="label">Temp</span><span id="t" class="val">--</span></div>
        <div class="card"><span class="label">Humi</span><span id="h" class="val">--</span></div>
      </div>
      <div class="grid" id="relayGrid" style="margin-top:15px;"></div>
    </div>
  </div>
<script>
  const canvas = document.getElementById('radarCanvas'); const ctx = canvas.getContext('2d');
  canvas.width = 600; canvas.height = 300; const cx = 300, cy = 300, r = 280;
  function update() {
    fetch('/data').then(res => res.json()).then(d => {
      document.getElementById('ang').innerText = d.angle; document.getElementById('dst').innerText = d.dist;
      drawRadar(d.angle, d.dist); document.getElementById('t').innerText = d.t; document.getElementById('h').innerText = d.h;
      d.states.forEach((s, i) => { const b = document.getElementById('b' + i); if(b){ b.className = s ? "btn btn-on" : "btn"; b.innerText = s ? "ON" : "OFF"; } });
    });
  }
  function drawRadar(angle, distance) {
    ctx.fillStyle = "rgba(0, 0, 0, 0.2)"; ctx.fillRect(0, 0, 600, 300);
    ctx.strokeStyle = "#004400"; for(let i=1; i<=4; i++) { ctx.beginPath(); ctx.arc(cx, cy, (r/4)*i, Math.PI, 0); ctx.stroke(); }
    let rad = (angle * Math.PI) / 180; ctx.strokeStyle = "#00ff41"; ctx.beginPath(); ctx.moveTo(cx, cy);
    ctx.lineTo(cx + Math.cos(-rad)*r, cy + Math.sin(-rad)*r); ctx.stroke();
    if (distance > 0 && distance < 100) { ctx.fillStyle = "red"; ctx.beginPath(); ctx.arc(cx + Math.cos(-rad)*(distance*2.8), cy + Math.sin(-rad)*(distance*2.8), 5, 0, 7); ctx.fill(); }
  }
  const names = ["Door", "Exhaust", "Bath", "Light", "Fan", "AC", "Light 2", "Garage"];
  let grid = document.getElementById('relayGrid');
  names.forEach((n, i) => { grid.innerHTML += `<div class='card'><span class='label'>${n}</span><button class='btn' id='b${i}' onclick="fetch('/toggle?id=${i}')">OFF</button></div>`; });
  setInterval(update, 500);
</script></body></html>
)rawliteral";

// --- 5. ANIMATION ENGINE ---
void executePattern(int mode) {
  patternStep++;
  switch (mode) {
    case 1: for(int i=0; i<NUMPIXELS; i++) pixels.setPixelColor(i, pixels.Color(random(255),random(255),random(255))); break;
    case 7: for(int i=0; i<NUMPIXELS; i++) pixels.setPixelColor(i, 255, random(80), 0); break;
    case 11: {
      float b = (sin(patternStep * 0.1) > 0) ? pow(sin(patternStep * 0.1), 10) : 0;
      for(int i=0; i<NUMPIXELS; i++) pixels.setPixelColor(i, b*255, 0, 0);
      break;
    }
    case 20: if(random(50) == 1) { for(int i=0; i<NUMPIXELS; i++) pixels.setPixelColor(i, 255, 255, 255); pixels.show(); delay(20); } pixels.clear(); break;
    default: for(int i=0; i<NUMPIXELS; i++) pixels.setPixelColor(i, ringColor); break;
  }
  pixels.show();
}

// --- 6. MAIN SETUP ---
void setup() {
  Serial.begin(115200); 
  dht.begin(); 
  pixels.begin(); 
  pixels.setBrightness(globalBrightness);
  
  pinMode(trigPin, OUTPUT); 
  pinMode(echoPin, INPUT); 
  pinMode(FLAME_PIN, INPUT); 
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(MQ7_PIN, INPUT); 
  pinMode(MQ8_PIN, INPUT);
  
  myServo.attach(servoPin, 500, 2400);
  
  for(int i=0; i<8; i++) { 
    pinMode(relayPins[i], OUTPUT); 
    digitalWrite(relayPins[i], HIGH); // Relays OFF (High level)
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nWiFi Connected. IP: " + WiFi.localIP().toString());

  server.on("/", []() { server.send(200, "text/html", PAGE_HTML); });
  
  server.on("/data", []() {
    String j = "{\"t\":"+String(dht.readTemperature(),1)+",\"h\":"+String(dht.readHumidity(),0)+",\"angle\":"+String(currentAngle)+",\"dist\":"+String(currentDistance)+",\"states\":[";
    for(int i=0; i<8; i++) j += String(relayStates[i]?"true":"false") + (i<7?",":"");
    j += "]}"; server.send(200, "application/json", j);
  });

  server.on("/toggle", []() { 
    int id = server.arg("id").toInt(); 
    relayStates[id] = !relayStates[id]; 
    digitalWrite(relayPins[id], relayStates[id] ? LOW : HIGH); 
    server.send(200); 
  });

  server.on("/setColor", []() { String hex = server.arg("c"); long n = strtol(&hex[1], NULL, 16); ringColor = pixels.Color((n>>16)&0xFF, (n>>8)&0xFF, n&0xFF); currentMode = 0; server.send(200); });
  server.on("/setMode", []() { currentMode = server.arg("m").toInt(); patternStep = 0; server.send(200); });
  server.on("/setBright", []() { globalBrightness = server.arg("b").toInt(); pixels.setBrightness(globalBrightness); server.send(200); });
  
  server.begin();
}

// --- 7. MAIN LOOP ---
void loop() {
  server.handleClient();

  // Radar logic
  if (millis() - lastRadar > 45) {
    lastRadar = millis(); 
    myServo.write(currentAngle);
    digitalWrite(trigPin, LOW); delayMicroseconds(2); 
    digitalWrite(trigPin, HIGH); delayMicroseconds(10); 
    digitalWrite(trigPin, LOW);
    currentDistance = pulseIn(echoPin, HIGH, 25000) * 0.034 / 2;
    currentAngle += sweepDirection; 
    if (currentAngle >= 180 || currentAngle <= 0) sweepDirection *= -1;
  }

  // NeoPixel timing
  if (millis() - lastPixel > animationSpeed) { 
    lastPixel = millis(); 
    executePattern(currentMode); 
  }

  // SAFETY LOGIC: MQ7, MQ8, and Flame
  int gas7 = analogRead(MQ7_PIN);
  int gas8 = analogRead(MQ8_PIN);
  bool isFlame = (digitalRead(FLAME_PIN) == LOW);

  // Auto-On Exhaust if Gas or Flame detected (Threshold 2000)
  if ((gas7 > 2000 || gas8 > 2000 || isFlame) && relayStates[1] == false) {
    relayStates[1] = true;
    digitalWrite(relayPins[1], LOW); // Low level trigger ON
  }

  // Buzzer Beep on Flame Detection
  if (isFlame) {
    digitalWrite(BUZZER_PIN, HIGH);
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }
}