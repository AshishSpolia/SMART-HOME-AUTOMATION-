#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "CMF";
const char* password = "1234567890";

// Relay Configuration
const int relayPins[8] = {13, 12, 14, 27, 26, 25, 33, 32};
String relayNames[] = {"Main Door", "Exhaust Fan", "Bath Fan", "Room Light", "Ceiling Fan", "Air Con", "Light 2", "Garage"};
String relayIcons[] = {"🚪", "🌀", "🧼", "💡", "🌬️", "❄️", "💡", "🚗"};
bool relayStates[8] = {false, false, false, false, false, false, false, false};

WebServer server(80);

void handleRoot() {
  String html = "<!DOCTYPE html><html lang='en'><head>";
  html += "<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Smart Home</title>";
  html += "<style>";
  html += "body { font-family: 'Segoe UI', sans-serif; background: #0f172a; color: white; margin: 0; padding: 20px; display: flex; flex-direction: column; align-items: center; }";
  html += "h1 { margin-bottom: 20px; color: #38bdf8; text-shadow: 0 0 10px rgba(56,189,248,0.5); }";
  html += ".container { display: grid; grid-template-columns: repeat(2, 1fr); gap: 15px; max-width: 400px; width: 100%; }";
  html += ".card { background: rgba(30, 41, 59, 0.7); border: 1px solid #334155; border-radius: 20px; padding: 20px; text-align: center; transition: 0.3s; }";
  html += ".icon { font-size: 35px; margin-bottom: 10px; display: block; }";
  html += ".label { font-size: 14px; font-weight: 500; margin-bottom: 15px; color: #94a3b8; }";
  html += ".btn { border: none; padding: 10px; width: 100%; border-radius: 12px; font-weight: bold; cursor: pointer; transition: 0.2s; }";
  html += ".on { background: #10b981; color: white; box-shadow: 0 0 15px rgba(16,185,129,0.4); }";
  html += ".off { background: #ef4444; color: white; }";
  html += ".btn:active { transform: scale(0.95); }";
  html += "</style></head><body>";
  
  html += "<h1>🏠 MY HOME</h1>";
  html += "<div class='container'>";

  for (int i = 0; i < 8; i++) {
    String stateClass = relayStates[i] ? "on" : "off";
    String stateText = relayStates[i] ? "ON" : "OFF";
    html += "<div class='card'>";
    html += "<span class='icon'>" + relayIcons[i] + "</span>";
    html += "<div class='label'>" + relayNames[i] + "</div>";
    html += "<button class='btn " + stateClass + "' onclick=\"location.href='/toggle?id=" + String(i) + "'\">" + stateText + "</button>";
    html += "</div>";
  }

  html += "</div></body></html>";
  server.send(200, "text/html", html);
}

void handleToggle() {
  if (server.hasArg("id")) {
    int id = server.arg("id").toInt();
    if (id >= 0 && id < 8) {
      relayStates[id] = !relayStates[id];
      // Note: Relay logic is typically inverted (LOW = ON)
      digitalWrite(relayPins[id], relayStates[id] ? LOW : HIGH);
    }
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

void setup() {
  Serial.begin(115200);
  for (int i = 0; i < 8; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], HIGH); // Relays OFF at start
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  
  Serial.println("\nConnected!");
  Serial.print("IP: "); Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/toggle", handleToggle);
  server.begin();
}

void loop() {
  server.handleClient();
}