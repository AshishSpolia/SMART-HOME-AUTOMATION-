#include <WiFi.h>
#include <WebServer.h>

// --- CONFIGURATION ---
const char* ssid = "CMF";
const char* password = "1234567890";

// Pin Definitions
#define MQ7_PIN 34    // Analog
#define MQ8_PIN 35    // Analog
#define FLAME_PIN 27  // Digital
#define RELAY_PIN 26  

WebServer server(80);

// Thresholds (Adjust based on testing)
const int GAS_THRESHOLD = 2000; 

// Global Variables
int coValue = 0;
int h2Value = 0;
bool flameDetected = false;
bool alarmActive = false;

const char PAGE_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Kitchen Safety Hub</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: 'Segoe UI', sans-serif; text-align: center; background-color: #1a1a2e; color: white; padding: 20px; }
    .card { background: #16213e; padding: 20px; margin: 10px auto; max-width: 400px; border-radius: 15px; border: 1px solid #0f3460; }
    h1 { color: #e94560; }
    .sensor-val { font-size: 1.2rem; margin: 10px 0; color: #4ecca3; }
    .status-box { padding: 20px; border-radius: 12px; font-size: 1.5rem; font-weight: bold; margin-bottom: 15px; }
    .secure { background-color: #0f3460; color: #4ecca3; border: 2px solid #4ecca3; }
    .alarm { background-color: #e94560; color: white; animation: pulse 0.6s infinite; }
    @keyframes pulse { 0% { transform: scale(1); } 50% { transform: scale(1.02); } 100% { transform: scale(1); } }
  </style>
</head>
<body>
  <h1>Kitchen Monitor</h1>
  <div class="card">
    <div id="secStatus" class="status-box secure">SYSTEM CLEAR</div>
    <div class="sensor-val">CO Level: <span id="mq7">0</span></div>
    <div class="sensor-val">H2/Gas Level: <span id="mq8">0</span></div>
    <div class="sensor-val">Flame: <span id="flame">NO</span></div>
  </div>

  <script>
    function updateStatus() {
      fetch('/data').then(response => response.json()).then(data => {
        document.getElementById('mq7').innerText = data.mq7;
        document.getElementById('mq8').innerText = data.mq8;
        document.getElementById('flame').innerText = data.flame ? "DETECTED" : "NONE";
        
        const secBox = document.getElementById('secStatus');
        if (data.alarm) {
            secBox.innerText = "DANGER DETECTED";
            secBox.className = "status-box alarm";
        } else {
            secBox.innerText = "SYSTEM CLEAR";
            secBox.className = "status-box secure";
        }
      });
    }
    setInterval(updateStatus, 1000); 
  </script>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", PAGE_HTML);
}

void handleData() {
  String json = "{";
  json += "\"mq7\":" + String(coValue) + ",";
  json += "\"mq8\":" + String(h2Value) + ",";
  json += "\"flame\":" + String(flameDetected ? "true" : "false") + ",";
  json += "\"alarm\":" + String(alarmActive ? "true" : "false");
  json += "}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  pinMode(FLAME_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // Relay OFF (assuming Active Low)

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  
  Serial.println("\nSystem Online");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
}

void loop() {
  server.handleClient();

  // Read Sensors
  coValue = analogRead(MQ7_PIN);
  h2Value = analogRead(MQ8_PIN);
  flameDetected = (digitalRead(FLAME_PIN) == LOW); // Flame sensors usually pull LOW when triggered

  // Logic: Trigger relay if gas is too high OR flame is detected
  if (coValue > GAS_THRESHOLD || h2Value > GAS_THRESHOLD || flameDetected) {
    alarmActive = true;
    digitalWrite(RELAY_PIN, LOW); // Turn on Exhaust/Siren
  } else {
    alarmActive = false;
    digitalWrite(RELAY_PIN, HIGH);
  }

  delay(200); // Small stability delay
}