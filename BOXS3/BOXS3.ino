#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "CMF";
const char* password = "1234567890";
WebServer server(80);

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, 44, 43); // RX=44, TX=43
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  
  server.on("/", []() {
    String html = "<h1>Smart Home Dashboard</h1>";
    for(int i=0; i<8; i++) {
      html += "<button onclick=\"fetch('/t?i="+String(i)+"')\">Relay "+String(i+1)+"</button><br>";
    }
    server.send(200, "text/html", html);
  });

  server.on("/t", []() {
    String id = server.arg("i");
    Serial1.println("TGL" + id); // Send command to Mega
    server.send(200);
  });

  server.begin();
}

void loop() {
  server.handleClient();
  
  // Receive status from Mega
  if (Serial1.available()) {
    String status = Serial1.readStringUntil('\n');
    // Display this on the ESP32-S3-BOX-3 screen
  }
}