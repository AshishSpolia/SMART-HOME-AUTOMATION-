#include <WiFi.h>
#include <WebServer.h>

// --- CONFIGURATION ---
const char* ssid = "CMF";
const char* password = "1234567890";

#define IR_PIN 27    
#define RELAY_PIN 26 

WebServer server(80);

// --- TIMER VARIABLES ---
unsigned long lastTriggerTime = 0;
const unsigned long relayDelay = 10000; // 10 seconds in milliseconds
bool relayActive = false;

// --- HTML & JAVASCRIPT ---
const char PAGE_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Timed Security Hub</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: 'Segoe UI', sans-serif; text-align: center; background-color: #1a1a2e; color: white; padding: 20px; }
    .card { background: #16213e; padding: 30px; margin: 20px auto; max-width: 400px; border-radius: 20px; box-shadow: 0 10px 30px rgba(0,0,0,0.5); border: 1px solid #0f3460; }
    h1 { color: #e94560; }
    .status-box { padding: 20px; border-radius: 12px; font-size: 1.5rem; font-weight: bold; transition: 0.3s; margin-bottom: 15px; }
    .secure { background-color: #0f3460; color: #4ecca3; border: 2px solid #4ecca3; }
    .alarm { background-color: #e94560; color: white; box-shadow: 0 0 20px #e94560; animation: pulse 0.6s infinite; }
    @keyframes pulse { 0% { transform: scale(1); } 50% { transform: scale(1.02); } 100% { transform: scale(1); } }
  </style>
</head>
<body>
  <h1>IR Timed System</h1>
  <div class="card">
    <div id="secStatus" class="status-box secure">PATH CLEAR</div>
    <p id="relay-text" style="color: #f1c40f;">Relay: OFF</p>
    <p id="timer-text" style="font-size: 0.8rem; color: #888;">Relay stays ON for 10s after detection</p>
  </div>

  <script>
    function updateStatus() {
      fetch('/data').then(response => response.json()).then(data => {
        const secBox = document.getElementById('secStatus');
        const relayTxt = document.getElementById('relay-text');
        
        if (data.relay === "ON") {
            secBox.innerText = "ALARM ACTIVE";
            secBox.className = "status-box alarm";
            relayTxt.innerText = "Relay: ON";
        } else {
            secBox.innerText = "PATH CLEAR";
            secBox.className = "status-box secure";
            relayTxt.innerText = "Relay: OFF";
        }
      });
    }
    setInterval(updateStatus, 500); 
  </script>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", PAGE_HTML);
}

void handleData() {
  String json = "{";
  json += "\"relay\":\"" + String(relayActive ? "ON" : "OFF") + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  pinMode(IR_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // Relay OFF (Active Low)

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }

  Serial.println("\nConnected!");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
}

void loop() {
  server.handleClient();

  int irState = digitalRead(IR_PIN);

  // If object detected (LOW), reset timer and turn relay ON
  if (irState == LOW) {
    relayActive = true;
    lastTriggerTime = millis();
    digitalWrite(RELAY_PIN, LOW); // ON
  }

  // If 10 seconds have passed since the last detection, turn relay OFF
  if (relayActive && (millis() - lastTriggerTime >= relayDelay)) {
    relayActive = false;
    digitalWrite(RELAY_PIN, HIGH); // OFF
  }
}