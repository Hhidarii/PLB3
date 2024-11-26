#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// WiFi credentials
const char* ssid     = "Dandelion";
const char* password = "hoangvy@";

// Web server
WebServer server(80);

// Left Motor
int motor1Pin1 = 26; 
int motor1Pin2 = 27; 
int enable1Pin = 14;

// Right Motor
int motor2Pin1 = 33; 
int motor2Pin2 = 25; 
int enable2Pin = 32;

// IR sensor pins and status LED pins
int irSensorPins[] = {36, 39, 34, 35};
int irStatusPins[] = {9, 18, 5, 17};

// LDR and light pins
int ldrPin = 23;
int lightPin = 2;

// Buzzer
int buzzPin = 4;

// LCD display
LiquidCrystal_I2C lcd(0x27, 20, 4);

bool lightState = false; // LED light status
const int freq = 30000;
const int resolution = 8;
int dutyCycle = 0;

// HTML interface
const char html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html { font-family: Helvetica; text-align: center; }
    .button { background-color: #4CAF50; color: white; padding: 10px 20px; border: none; cursor: pointer; margin: 5px; }
    .button:hover { background-color: #45a049; }
    .status { font-weight: bold; color: red; }
    iframe { border: none; margin-top: 20px; }
  </style>
  <script>
    setInterval(() => {
      fetch('/sensor').then(res => res.json()).then(data => {
        ['slot1', 'slot2', 'slot3', 'slot4'].forEach((id, i) => {
          document.getElementById(id).textContent = data.irValues[i] ? "ACTIVE" : "INACTIVE";
        });
        document.getElementById('light').textContent = data.lightState ? "ON" : "OFF";
      });
    }, 1000);

    function controlBuzzer(state) {
      fetch('/buzzer?state=' + state);
    }
  </script>
</head>
<body>
  <h1>ESP32 Robot Control</h1>
  <h2>Motor Controls</h2>
  <button class="button" onclick="fetch('/forward')">Forward</button>
  <button class="button" onclick="fetch('/left')">Left</button>
  <button class="button" onclick="fetch('/stop')">Stop</button>
  <button class="button" onclick="fetch('/right')">Right</button>
  <button class="button" onclick="fetch('/reverse')">Reverse</button>
  
  <h2>Sensor Status</h2>
  <p>Slot 1: <span id="slot1" class="status">-</span></p>
  <p>Slot 2: <span id="slot2" class="status">-</span></p>
  <p>Slot 3: <span id="slot3" class="status">-</span></p>
  <p>Slot 4: <span id="slot4" class="status">-</span></p>
  <p>Light Status: <span id="light" class="status">-</span></p>
  
  <h2>Buzzer Control</h2>
  <button class="button" onclick="controlBuzzer(1)">Turn ON</button>
  <button class="button" onclick="controlBuzzer(0)">Turn OFF</button>

  <h2>Camera Stream</h2>
  <iframe src="http://192.168.1.194:81/stream" width="640" height="480"></iframe>
</body>
</html>
)rawliteral";

// Update light state based on LDR
void updateLightState() {
  if (digitalRead(ldrPin) == LOW) { // Environment is dark
    digitalWrite(lightPin, HIGH);  // Turn on the light
    lightState = true;
  } else { // Environment is bright
    digitalWrite(lightPin, LOW);   // Turn off the light
    lightState = false;
  }
}

// Send sensor data as JSON
void handleSensor() {
  bool irValues[4];
  for (int i = 0; i < 4; i++) {
    irValues[i] = digitalRead(irSensorPins[i]);
    digitalWrite(irStatusPins[i], irValues[i] ? HIGH : LOW); // Update LED status
  }

  // Return JSON with sensor and light data
  String json = "{\"irValues\":[" + String(irValues[0]) + "," + String(irValues[1]) + "," + String(irValues[2]) + "," + String(irValues[3]) +
                "],\"lightState\":" + (lightState ? "true" : "false") + "}";
  server.send(200, "application/json", json);
}

// Buzzer control
void handleBuzzer() {
  if (server.hasArg("state")) {
    int state = server.arg("state").toInt();
    digitalWrite(buzzPin, state);
    server.send(200, "text/plain", "Buzzer updated");
  }
}

// Motor controls
void handleForward() {
  digitalWrite(motor1Pin1, LOW); digitalWrite(motor1Pin2, HIGH);
  digitalWrite(motor2Pin1, LOW); digitalWrite(motor2Pin2, HIGH);
  server.send(200);
}

void handleLeft() {
  digitalWrite(motor1Pin1, HIGH); digitalWrite(motor1Pin2, LOW);
  digitalWrite(motor2Pin1, LOW); digitalWrite(motor2Pin2, HIGH);
  server.send(200);
}

void handleStop() {
  digitalWrite(motor1Pin1, LOW); digitalWrite(motor1Pin2, LOW);
  digitalWrite(motor2Pin1, LOW); digitalWrite(motor2Pin2, LOW);
  server.send(200);
}

void handleRight() {
  digitalWrite(motor1Pin1, LOW); digitalWrite(motor1Pin2, HIGH);
  digitalWrite(motor2Pin1, HIGH); digitalWrite(motor2Pin2, LOW);
  server.send(200);
}

void handleReverse() {
  digitalWrite(motor1Pin1, HIGH); digitalWrite(motor1Pin2, LOW);
  digitalWrite(motor2Pin1, HIGH); digitalWrite(motor2Pin2, LOW);
  server.send(200);
}

void setup() {
  Serial.begin(115200);
  // Configure pins
  pinMode(motor1Pin1, OUTPUT); pinMode(motor1Pin2, OUTPUT);
  pinMode(motor2Pin1, OUTPUT); pinMode(motor2Pin2, OUTPUT);
  pinMode(lightPin, OUTPUT); digitalWrite(lightPin, LOW);
  pinMode(buzzPin, OUTPUT); digitalWrite(buzzPin, LOW);
  for (int i = 0; i < 4; i++) {
    pinMode(irSensorPins[i], INPUT);
    pinMode(irStatusPins[i], OUTPUT);
  }

  // LCD initialization
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0); lcd.print("ISO.AR-SHIPPINGROBOT");
  lcd.setCursor(5, 1); lcd.print("PBL3 DEMO");
  lcd.setCursor(0, 2); lcd.print("ROOM : A103");
  lcd.setCursor(0, 3); lcd.print("NAME : NGUYEN VAN AN");

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Set up server routes
  server.on("/", []() { server.send(200, "text/html", html); });
  server.on("/sensor", handleSensor);
  server.on("/buzzer", handleBuzzer);
  server.on("/forward", handleForward);
  server.on("/left", handleLeft);
  server.on("/stop", handleStop);
  server.on("/right", handleRight);
  server.on("/reverse", handleReverse);
  server.begin();
}
void loop() {
  server.handleClient();
  updateLightState();
}
