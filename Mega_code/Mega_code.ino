#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <DHT.h>

// --- Pin Map ---
#define RFID_SS 53
#define RFID_RST 5
#define SERVO_PIN 11
#define TRIG 5
#define ECHO 18
#define MQ7 A0
#define MQ8 A1
#define FLAME 27
#define IR_SENSOR 40
#define DHTPIN 4
#define BUZZER 45

MFRC522 rfid(RFID_SS, RFID_RST);
Servo radarServo;
DHT dht(DHTPIN, DHT22);

const int relayPins[] = {22, 23, 24, 25, 28, 29, 30, 31};
int angle = 0, step = 2;

void setup() {
  Serial.begin(115200); // Debug
  Serial1.begin(9600);  // Comm to ESP32
  SPI.begin();
  rfid.PCD_Init();
  radarServo.attach(SERVO_PIN);
  dht.begin();
  
  for(int i=0; i<8; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], HIGH); // OFF
  }
  pinMode(FLAME, INPUT);
  pinMode(IR_SENSOR, INPUT);
  pinMode(BUZZER, OUTPUT);
}

void loop() {
  // 1. Radar Sweep & Distance
  radarServo.write(angle);
  long dist = getDistance();
  angle += step;
  if (angle <= 0 || angle >= 180) step = -step;

  // 2. Kitchen Safety Logic
  bool fire = (digitalRead(FLAME) == LOW);
  if (fire || analogRead(MQ8) > 700) {
    digitalWrite(relayPins[1], LOW); // Exhaust ON
    digitalWrite(BUZZER, HIGH);
  } else {
    digitalWrite(BUZZER, LOW);
  }

  // 3. RFID Logic
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    Serial1.println("AUTH_SUCCESS"); // Send to ESP32 for UI update
    digitalWrite(relayPins[0], LOW); // Open Door
    delay(2000);
    digitalWrite(relayPins[0], HIGH);
    rfid.PICC_HaltA();
  }

  // 4. Handle Commands from ESP32
  if (Serial1.available()) {
    String cmd = Serial1.readStringUntil('\n');
    cmd.trim();
    if(cmd.startsWith("TGL")) {
      int id = cmd.substring(3).toInt();
      digitalWrite(relayPins[id], !digitalRead(relayPins[id]));
    }
  }
}

long getDistance() {
  digitalWrite(TRIG, LOW); delayMicroseconds(2);
  digitalWrite(TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  return pulseIn(ECHO, HIGH) * 0.034 / 2;
}