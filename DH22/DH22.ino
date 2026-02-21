#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>

// --- CONFIGURATION ---
const char* ssid = "CMF";
const char* password = "1234567890";

#define DHTPIN 4
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);
WebServer server(80);

// --- HTML & JAVASCRIPT ---
const char PAGE_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 Weather Station</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: sans-serif; text-align: center; background-color: #f4f4f4; }
    .card { background: white; padding: 20px; margin: 20px auto; width: 300px; border-radius: 15px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); }
    h1 { color: #2c3e50; }
    .value { font-size: 2.5rem; font-weight: bold; color: #3498db; }
    .label { font-size: 1.2rem; color: #7f8c8d; }
  </style>
</head>
<body>
  <h1>Room Climate</h1>
  <div class="card">
    <div class="label">Temperature</div>
    <div class="value"><span id="temp">0</span>&deg;C</div>
  </div>
  <div class="card">
    <div class="label">Humidity</div>
    <div class="value"><span id="hum">0</span>%</div>
  </div>

  <script>
    function updateData() {
      fetch('/data').then(response => response.json()).then(data => {
        document.getElementById('temp').innerText = data.temp;
        document.getElementById('hum').innerText = data.hum;
      });
    }
    setInterval(updateData, 2000); // Update every 2 seconds
  </script>
</body>
</html>
)rawliteral";

// --- ROUTES ---
void handleRoot() {
  server.send(200, "text/html", PAGE_HTML);
}

void handleData() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  
  String json = "{";
  json += "\"temp\":\"" + String(isnan(t) ? 0 : t) + "\",";
  json += "\"hum\":\"" + String(isnan(h) ? 0 : h) + "\"";
  json += "}";
  
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
}

void loop() {
  server.handleClient();
}