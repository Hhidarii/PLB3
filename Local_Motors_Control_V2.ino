#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const char* ssid     = "Dandelion";
const char* password = "hoangvy@";

WebServer server(80);

// Left Motor
int motor1Pin1 = 26; 
int motor1Pin2 = 27; 
int enable1Pin = 14;

// Right Motor
int motor2Pin1 = 33; 
int motor2Pin2 = 25; 
int enable2Pin = 32;

int irSensorPins[] = {36, 39, 34, 35};
int irStatusPins[] = {9 , 18,  5, 17};
int ldrPin = 23;
int lightPin = 2;
int buzzPin = 4;

LiquidCrystal_I2C lcd(0x27, 20, 4);

// Setting PWM properties
const int freq = 30000;
const int resolution = 8;
int dutyCycle = 0;

String valueString = String(0);

void handleRoot() {
  const char html[] PROGMEM = R"rawliteral(
  <!DOCTYPE HTML><html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="icon" href="data:,">
    <style>
      html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center; }
      .button { -webkit-user-select: none; -moz-user-select: none; -ms-user-select: none; user-select: none; background-color: #4CAF50; border: none; color: white; padding: 12px 28px; text-decoration: none; font-size: 26px; margin: 1px; cursor: pointer; }
      .button2 {background-color: #555555;}
    </style>
    <script>
      function moveForward()  { fetch('/forward'); }
      function moveLeft()     { fetch('/left'); }
      function stopRobot()    { fetch('/stop'); }
      function moveRight()    { fetch('/right'); }
      function moveReverse()  { fetch('/reverse'); }

      function updateMotorSpeed(pos) {
        document.getElementById('motorSpeed').innerHTML = pos;
        fetch(`/speed?value=${pos}`);
      }
      function fetchSensorData() {
        fetch('/sensor').then(response => response.json()).then(data => {
          document.getElementById('ir1').innerText = data.irValues[0] ? "ACTIVE" : "INACTIVE";
          document.getElementById('ir2').innerText = data.irValues[1] ? "ACTIVE" : "INACTIVE";
          document.getElementById('ir3').innerText = data.irValues[2] ? "ACTIVE" : "INACTIVE";
          document.getElementById('ir4').innerText = data.irValues[3] ? "ACTIVE" : "INACTIVE";

          document.getElementById('ldr').innerText = data.lightStatus ? "DARK" : "BRIGHT";
          document.getElementById('light').innerText = data.lightState ? "ON" : "OFF";

          setStatusClass('ir1', data.irValues[0]);
          setStatusClass('ir2', data.irValues[1]);
          setStatusClass('ir3', data.irValues[2]);
          setStatusClass('ir4', data.irValues[3]);
          setStatusClass('ldr', data.lightStatus);
          setStatusClass('light', data.lightState);
        });
      }
      function setStatusClass(id, state) {
        document.getElementById(id).className = 'status ' + (state ? 'active' : 'inactive');
      }

      setInterval(fetchSensorData, 1000);

      
    </script>
  </head>
  <body>
    <h1>ESP32 Motor and Sensor Control - DEMO</h1>
    <div>
      <h2>Motor Controls</h2>
      <button class="button" onclick="moveForward()">FORWARD</button>
      <div style="margin: 10px 0;">
        <button class="button" onclick="moveLeft()">LEFT</button>
        <button class="button button2" onclick="stopRobot()">STOP</button>
        <button class="button" onclick="moveRight()">RIGHT</button>
      </div>
      <button class="button" onclick="moveReverse()">REVERSE</button>
      <p>Motor Speed: <span id="motorSpeed">0</span></p>
      <input type="range" min="0" max="100" step="1" id="motorSlider" oninput="updateMotorSpeed(this.value)" value="0"/>
    </div>

    <div>
      <h2>Sensor Status</h2>
      <div>
        <h3>IR Sensors</h3>
        <p>IR Sensor 1: <span id="ir1" class="status inactive">-</span></p>
        <p>IR Sensor 2: <span id="ir2" class="status inactive">-</span></p>
        <p>IR Sensor 3: <span id="ir3" class="status inactive">-</span></p>
        <p>IR Sensor 4: <span id="ir4" class="status inactive">-</span></p>
      </div>
      <div>
        <h3>Light Sensor</h3>
        <p>Ambient Light: <span id="ldr" class="status inactive">-</span></p>
        <p>Light Status: <span id="light" class="status inactive">-</span></p>
      </div>
    </div>
  </body>
  </html>)rawliteral";
  server.send(200, "text/html", html);
}



void handleForward() {
  Serial.println("Forward");
  digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, HIGH); 
  digitalWrite(motor2Pin1, LOW);
  digitalWrite(motor2Pin2, HIGH);
  server.send(200);
}

void handleLeft() {
  Serial.println("Left");
  digitalWrite(motor1Pin1, HIGH); 
  digitalWrite(motor1Pin2, LOW); 
  digitalWrite(motor2Pin1, LOW);
  digitalWrite(motor2Pin2, HIGH);
  server.send(200);
}

void handleStop() {
  Serial.println("Stop");
  digitalWrite(motor1Pin1, LOW); 
  digitalWrite(motor1Pin2, LOW); 
  digitalWrite(motor2Pin1, LOW);
  digitalWrite(motor2Pin2, LOW);   
  server.send(200);
}

void handleRight() {
  Serial.println("Right");
  digitalWrite(motor1Pin1, LOW); 
  digitalWrite(motor1Pin2, HIGH); 
  digitalWrite(motor2Pin1, HIGH);
  digitalWrite(motor2Pin2, LOW);    
  server.send(200);
}

void handleReverse() {
  Serial.println("Reverse");
  digitalWrite(motor1Pin1, HIGH);
  digitalWrite(motor1Pin2, LOW); 
  digitalWrite(motor2Pin1, HIGH);
  digitalWrite(motor2Pin2, LOW);          
  server.send(200);
}

void handleSpeed() {
  if (server.hasArg("value")) {
    valueString = server.arg("value");
    int value = valueString.toInt();
    if (value == 0) {
      ledcWrite(enable1Pin, 0);
      ledcWrite(enable2Pin, 0);
      digitalWrite(motor1Pin1, LOW); 
      digitalWrite(motor1Pin2, LOW); 
      digitalWrite(motor2Pin1, LOW);
      digitalWrite(motor2Pin2, LOW);   
    } else { 
      dutyCycle = map(value, 1, 100, 1, 255);
      ledcWrite(enable1Pin, dutyCycle);
      ledcWrite(enable2Pin, dutyCycle);
      Serial.println("Motor speed set to " + String(value));
    }
  }
  server.send(200);
}

void handleSensor() {
  bool ir1 = digitalRead(36);
  bool ir2 = digitalRead(39);
  bool ir3 = digitalRead(34);
  bool ir4 = digitalRead(35);
  bool ldrValue = digitalRead(23);

  Serial.print("IR1: "); Serial.println(ir1);
  Serial.print("IR2: "); Serial.println(ir2);
  Serial.print("IR3: "); Serial.println(ir3);
  Serial.print("IR4: "); Serial.println(ir4);
  Serial.print("LDR: "); Serial.println(ldrValue);
}

void setup() {
  Serial.begin(115200);
  pinMode(motor1Pin1, OUTPUT);
  pinMode(motor1Pin2, OUTPUT);
  pinMode(motor2Pin1, OUTPUT);
  pinMode(motor2Pin2, OUTPUT);

  // Configure PWM Pins
  ledcAttach(enable1Pin, freq, resolution);
  ledcAttach(enable2Pin, freq, resolution);
    
  // Initialize PWM with 0 duty cycle
  ledcWrite(enable1Pin, 0);
  ledcWrite(enable2Pin, 0);

  // IR Sensors
  for (int i = 0; i < 4; i++) {
    pinMode(irSensorPins[i], INPUT);
  }

  // LDR and LED 
  pinMode(ldrPin, INPUT);
  pinMode(lightPin, OUTPUT);
  digitalWrite(lightPin, LOW);

  lcd.init();
  lcd.backlight();
lcd.setCursor(0, 0);
lcd.print("ISO.AR-SHIPPINGROBOT");

lcd.setCursor(5, 1);
lcd.print("PBL3 DEMO");

lcd.setCursor(0, 2);
lcd.print("ROOM : A103");

lcd.setCursor(0, 3);
lcd.print("NAME : NGUYEN VAN AN");

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

  // Define routes
  server.on("/",        handleRoot);
  server.on("/forward", handleForward);
  server.on("/left",    handleLeft);
  server.on("/stop",    handleStop);
  server.on("/right",   handleRight);
  server.on("/reverse", handleReverse);
  server.on("/speed",   handleSpeed);
  server.on("/sensor",  handleSensor);
  server.begin();
}

void loop() {
  server.handleClient();
  if (digitalRead(ldrPin) == LOW) {
    digitalWrite(lightPin, LOW);
  } else {
    digitalWrite(lightPin, HIGH);
  }
}
