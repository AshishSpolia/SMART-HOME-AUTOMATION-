
#include <WiFi.h>
#include <WebServer.h>

// --- CONFIGURATION ---
const char* ssid = "CMF";
const char* password = "1234567890";

#define LDR_PIN 34    // LDR Signal (Analog)
#define BUZZER_PIN 25 // Buzzer Signal
#define LASER_PIN 26  // Laser Power Control

WebServer server(80);

// --- THRESHOLD SETTING ---
int threshold = 1000; // Alarm triggers if value is MORE than 1000

// --- HTML & JAVASCRIPT ---
const char PAGE_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Security Hub</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: 'Segoe UI', sans-serif; text-align: center; background-color: #1a1a2e; color: white; padding: 20px; }
    .card { background: #16213e; padding: 30px; margin: 20px auto; max-width: 400px; border-radius: 20px; box-shadow: 0 10px 30px rgba(0,0,0,0.5); border: 1px solid #0f3460; }
    h1 { color: #e94560; }
    .status-box { padding: 20px; border-radius: 12px; font-size: 1.5rem; font-weight: bold; transition: 0.3s; margin-bottom: 15px; }
    .secure { background-color: #0f3460; color: #4ecca3; border: 2px solid #4ecca3; }
    .alarm { background-color: #e94560; color: white; box-shadow: 0 0 20px #e94560; animation: pulse 0.6s infinite; }
    @keyframes pulse { 0% { transform: scale(1); } 50% { transform: scale(1.02); } 100% { transform: scale(1); } }
    .val-display { font-size: 1.2rem; color: #f1c40f; }
  </style>
</head>
<body>
  <h1>Laser Security System</h1>
  <div class="card">
    <div id="secStatus" class="status-box secure">SYSTEM SECURE</div>
    <p>Current Intensity: <span id="ldr-val" class="val-display">0</span></p>
    <p style="color: #666; font-size: 0.9rem;">Trigger Threshold: > 1000</p>
  </div>

  <script>
    function updateStatus() {
      fetch('/data').then(response => response.json()).then(data => {
        document.getElementById('ldr-val').innerText = data.ldr;
        const secBox = document.getElementById('secStatus');
        if (data.alarm === "ON") {
            secBox.innerText = "ALARM TRIGGERED!";
            secBox.className = "status-box alarm";
        } else {
            secBox.innerText = "SYSTEM SECURE";
            secBox.className = "status-box secure";
        }
      });
    }
    setInterval(updateStatus, 200); // UI updates every 0.2 seconds
  </script>
</body>
</html>
)rawliteral";

// --- ROUTES ---
void handleRoot() {
  server.send(200, "text/html", PAGE_HTML);
}

void handleData() {
  int ldrVal = analogRead(LDR_PIN);
  
  // LOGIC: Trigger if light intensity goes ABOVE 1000
  bool isAlarm = (ldrVal > threshold);

  if(isAlarm) {
    digitalWrite(BUZZER_PIN, HIGH);
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }
  
  String json = "{";
  json += "\"ldr\":\"" + String(ldrVal) + "\",";
  json += "\"alarm\":\"" + String(isAlarm ? "ON" : "OFF") + "\"";
  json += "}";
  
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LASER_PIN, OUTPUT);
  digitalWrite(LASER_PIN, HIGH); 

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected!");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
}

void loop() {
  server.handleClient();
}