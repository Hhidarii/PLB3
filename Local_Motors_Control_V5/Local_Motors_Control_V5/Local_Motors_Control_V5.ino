#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// WiFi credentials
const char* ssid     = "Dandelion";
const char* password = "hoangvy@";

// Web server
WebServer server(80);

// Left Motor #1
int motor1Pin1 = 33; 
int motor1Pin2 = 25; 
int enable1Pin = 32;

// Right Motor #2
int motor2Pin1 = 14; 
int motor2Pin2 = 27; 
int enable2Pin = 26;

// IR sensor and status LED pins
int irSensorPins[] = {36, 39, 34, 35};
int slotPins[] = {19, 18, 5, 17};

// LDR and light control pins

int lightPin = 2;
int buzzPin = 4;

// LCD display
LiquidCrystal_I2C lcd(0x27, 20, 4);

// PWM settings
const int freq = 30000;
const int resolution = 8;
int dutyCycle = 0;

String valueString = String(0);

// HTML for web interface
void handleRoot() {
  const char html[] PROGMEM = R"rawliteral(
  <!DOCTYPE HTML><html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="icon" href="data:,">
    <style>
      html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center; }
      .button { -webkit-user-select: none; -moz-user-select: none; -ms-user-select: none; user-select: none; background-color: #4CAF50; border: none; color: white; padding: 12px 28px; text-decoration: none; font-size: 26px; margin: 1px; cursor: pointer; }
      .button2 {background-color: #FFFFFF;}
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
          document.getElementById('ir1').innerText = data.irValues[0] ? "EMPTY" : "FILLED";
          document.getElementById('ir2').innerText = data.irValues[1] ? "EMPTY" : "FILLED";
          document.getElementById('ir3').innerText = data.irValues[2] ? "EMPTY" : "FILLED";
          document.getElementById('ir4').innerText = data.irValues[3] ? "EMPTY" : "FILLED";
          
          setStatusClass('ir1', data.irValues[0]);
          setStatusClass('ir2', data.irValues[1]);
          setStatusClass('ir3', data.irValues[2]);
          setStatusClass('ir4', data.irValues[3]);
        });
      }
      function setStatusClass(id, state) {
        document.getElementById(id).className = 'status ' + (state ? 'active' : 'inactive');
      }

      setInterval(fetchSensorData, 1000);
      
      function controlBuzzer(state) {
        fetch('/buzzer?state=' + state);
      }

      function controlLight(state) {
        fetch('/light?state=' + state);
      }
    </script>
  </head>
  <body>
    <h1>ISOLATED -  AREA SHIPPING ROBOT</h1>
    <div>
      <h2>Motor Control</h2>
      <button class="button" ontouchstart="moveForward()" ontouchend="stopRobot()" onmousedown="moveForward()" onmouseup="stopRobot()">FORWARD</button>
      <div style="margin: 10px 0;">
        <button class="button" ontouchstart="moveLeft()"  ontouchend="stopRobot()" onmousedown="moveLeft()" onmouseup="stopRobot()">LEFT</button>
        <button class="button button2" onclick="stopRobot()">                         </button>
        <button class="button" ontouchstart="moveRight()" ontouchend="stopRobot()" onmousedown="moveRight()" onmouseup="stopRobot()">RIGHT</button>
      </div>
      <button class="button" ontouchstart="moveReverse()" ontouchend="stopRobot()" onmousedown="moveReverse()" onmouseup="stopRobot()">BACKWARD</button>
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
    </div>
    
    <h2>Buzzer Control</h2>
    <button class="button" ontouchstart="controlBuzzer(1)"  ontouchend="controlBuzzer(0)" onmousedown="controlBuzzer(1)" onmouseup="controlBuzzer(0)">BUZZER</button>
   
    <h2>Light Control</h2>
    <button class="button" onclick="controlLight(1)">ON</button>
    <button class="button" onclick="controlLight(0)">OFF</button>
    
//----------------------------------------------------------------------------------//
    <h2>Light Control</h2>
    <button class="button" onclick="controlLight(1)">ON</button>
    <button class="button" onclick="controlLight(0)">OFF</button>
//----------------------------------------------------------------------------------//
  
  </body>
  </html>)rawliteral";
  server.send(200, "text/html", html);
}

// Motor control functions
void handleForward() {
  digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, HIGH); 
  digitalWrite(motor2Pin1, LOW);
  digitalWrite(motor2Pin2, HIGH);
  server.send(200);
}

void handleLeft() {
  digitalWrite(motor1Pin1, HIGH); 
  digitalWrite(motor1Pin2, LOW); 
  digitalWrite(motor2Pin1, LOW);
  digitalWrite(motor2Pin2, HIGH);
  server.send(200);
}

void handleStop() {
  digitalWrite(motor1Pin1, LOW); 
  digitalWrite(motor1Pin2, LOW); 
  digitalWrite(motor2Pin1, LOW);
  digitalWrite(motor2Pin2, LOW);   
  server.send(200);
}

void handleRight() {
  digitalWrite(motor1Pin1, LOW); 
  digitalWrite(motor1Pin2, HIGH); 
  digitalWrite(motor2Pin1, HIGH);
  digitalWrite(motor2Pin2, LOW);    
  server.send(200);
}

void handleReverse() {
  digitalWrite(motor1Pin1, HIGH);
  digitalWrite(motor1Pin2, LOW); 
  digitalWrite(motor2Pin1, HIGH);
  digitalWrite(motor2Pin2, LOW);          
  server.send(200);
}

// Handle motor speed adjustment
void handleSpeed() {
  if (server.hasArg("value")) {
    valueString = server.arg("value");
    int value = valueString.toInt();
    if (value == 0) {
      ledcWrite(enable1Pin, 0);
      ledcWrite(enable2Pin, 0);
    } else { 
      dutyCycle = map(value, 1, 100, 150, 255);
      ledcWrite(enable1Pin, dutyCycle);
      ledcWrite(enable2Pin, dutyCycle);
    }
  }
  server.send(200);
}

// Handle sensor data
void handleSensor() {
  bool ir1 = digitalRead(36);
  bool ir2 = digitalRead(39);
  bool ir3 = digitalRead(34);
  bool ir4 = digitalRead(35);

  String json = "{\"irValues\":[" + String(ir1) + "," + String(ir2) + "," + String(ir3) + "," + String(ir4) + "]}";
  //String json = "{\"irValues\":[" + String(ir1) + "," + String(ir2) + "," + String(ir3) + "," + String(ir4) + "]}";

  server.send(200, "application/json", json);
}

// Control buzzer (on/off)
void handleBuzzer() {
  if (server.hasArg("state")) {
    int state = server.arg("state").toInt();
    if (state == 1) {
      digitalWrite(buzzPin, HIGH);
    } else {
      digitalWrite(buzzPin, LOW);
    }
  }
  server.send(200);
}

// Control buzzer (on/off)
void handleLight() {
  if (server.hasArg("state")) {
    int state = server.arg("state").toInt();
    if (state == 1) {
      digitalWrite(lightPin, HIGH);
    } else {
      digitalWrite(lightPin, LOW);
    }
  }
  server.send(200);
}

void setup() {
  Serial.begin(115200);
  pinMode(motor1Pin1, OUTPUT);
  pinMode(motor1Pin2, OUTPUT);
  pinMode(motor2Pin1, OUTPUT);
  pinMode(motor2Pin2, OUTPUT);
  pinMode(buzzPin, OUTPUT);
  pinMode(lightPin, OUTPUT);

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

  // Connect to WiFi
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


  // Web server routes
  server.on("/",        handleRoot);
  server.on("/forward", handleForward);
  server.on("/left",    handleLeft);
  server.on("/stop",    handleStop);
  server.on("/right",   handleRight);
  server.on("/reverse", handleReverse);
  server.on("/speed",   handleSpeed);
  server.on("/sensor",  handleSensor);
  server.on("/buzzer",  handleBuzzer);
  server.on("/light",   handleLight);
  server.begin();
}

void loop() {
  server.handleClient();

  
}
