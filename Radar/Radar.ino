#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>

// --- 1. FILL IN YOUR WIFI DETAILS ---
const char* ssid = "CMF";
const char* password = "1234567890";

// --- 2. PIN DEFINITIONS ---
const int servoPin = 18;
const int trigPin = 5;
const int echoPin = 19;

// --- 3. OBJECTS & VARIABLES ---
Servo myServo;
WebServer server(80);

int currentAngle = 0;
int currentDistance = 0;
int sweepDirection = 1;

// --- 4. PROFESSIONAL RADAR INTERFACE (HTML/JS) ---
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 PRO RADAR</title>
    <style>
        body { background: #050505; color: #00ff41; font-family: 'Courier New', monospace; text-align: center; margin: 0; overflow: hidden; }
        .container { display: flex; flex-direction: column; align-items: center; justify-content: center; height: 100vh; }
        canvas { background: #000; border: 2px solid #004400; border-radius: 10px; box-shadow: 0 0 20px rgba(0,255,65,0.2); width: 90%; max-width: 800px; }
        .stats { margin-top: 20px; font-size: 1.5em; text-shadow: 0 0 8px #00ff41; }
    </style>
</head>
<body>
    <div class="container">
        <h1>RADAR SYSTEM ACTIVE</h1>
        <canvas id="radarCanvas"></canvas>
        <div class="stats">ANGLE: <span id="ang">0</span>° | DISTANCE: <span id="dst">0</span> cm</div>
    </div>
<script>
    const canvas = document.getElementById('radarCanvas');
    const ctx = canvas.getContext('2d');
    canvas.width = 800; canvas.height = 400;
    const cx = 400, cy = 400, r = 380;

    function fetchData() {
        fetch('/data').then(res => res.json()).then(data => {
            document.getElementById('ang').innerText = data.angle;
            document.getElementById('dst').innerText = data.dist;
            draw(data.angle, data.dist);
        });
    }

    function draw(angle, distance) {
        ctx.fillStyle = "rgba(0, 0, 0, 0.1)";
        ctx.fillRect(0, 0, canvas.width, canvas.height);

        // Draw Rings
        ctx.strokeStyle = "#003300";
        for(let i=1; i<=4; i++) {
            ctx.beginPath(); ctx.arc(cx, cy, (r/4)*i, Math.PI, 0); ctx.stroke();
        }

        // Draw Sweep Line
        let rad = (angle * Math.PI) / 180;
        ctx.strokeStyle = "#00ff41"; ctx.lineWidth = 3;
        ctx.beginPath(); ctx.moveTo(cx, cy);
        ctx.lineTo(cx + Math.cos(-rad)*r, cy + Math.sin(-rad)*r);
        ctx.stroke();

        // Draw Object Blip
        if (distance > 0 && distance < 100) {
            let ox = cx + Math.cos(-rad) * (distance * (r/100));
            let oy = cy + Math.sin(-rad) * (distance * (r/100));
            ctx.fillStyle = "red"; ctx.beginPath();
            ctx.arc(ox, oy, 6, 0, Math.PI*2); ctx.fill();
        }
    }
    setInterval(fetchData, 40);
</script>
</body>
</html>
)rawliteral";

// --- 5. DISTANCE SENSOR LOGIC ---
int readDistance() {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    long duration = pulseIn(echoPin, HIGH, 25000); // 25ms timeout
    int distance = duration * 0.034 / 2;
    return (distance <= 0 || distance > 150) ? 150 : distance;
}

// --- 6. SETUP & LOOP ---
void setup() {
    Serial.begin(115200);
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    
    myServo.setPeriodHertz(50);
    myServo.attach(servoPin, 500, 2400);

    WiFi.begin(ssid, password);
    Serial.print("Connecting WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nIP Address: " + WiFi.localIP().toString());

    // Routes
    server.on("/", []() {
        server.send(200, "text/html", index_html);
    });

    server.on("/data", []() {
        String json = "{\"angle\":" + String(currentAngle) + ",\"dist\":" + String(currentDistance) + "}";
        server.send(200, "application/json", json);
    });

    server.begin();
}

void loop() {
    server.handleClient();
    
    static unsigned long lastMove = 0;
    if (millis() - lastMove > 35) { // Controls sweep speed
        lastMove = millis();
        
        myServo.write(currentAngle);
        currentDistance = readDistance();
        
        currentAngle += sweepDirection;
        if (currentAngle >= 180 || currentAngle <= 0) {
            sweepDirection *= -1;
        }
    }
}