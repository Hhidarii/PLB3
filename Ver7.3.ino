#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// WiFi credentials
const char* ssid     = "HTN";
const char* password = "dinhphuoc";

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

// Task handles
TaskHandle_t Task1;
TaskHandle_t Task2;

// Queue handles
QueueHandle_t motorQueue;
QueueHandle_t sensorQueue;

// Motor control commands
enum MotorCommand {
  FORWARD,
  LEFT,
  STOP,
  RIGHT,
  REVERSE,
  SPEED
};

struct MotorControl {
  MotorCommand command;
  int speed;
};

// Sensor data structure
struct SensorData {
  bool irValues[4];
};

// HTML for web interface
void handleRoot() {
  const char html[] = R"rawliteral(
  <!DOCTYPE HTML>
<html>
<head>
  <meta charset="UTF-8" />
  <title>ISOLATED - AREA SHIPPING ROBOT</title>
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <style>
    body {
      margin: 0; font-family: Arial, sans-serif;
      background: url('https://png.pngtree.com/background/20230417/original/pngtree-medical-doctor-image-picture-image_2447169.jpg') no-repeat center/cover;
      text-align: center; color: #2c3e50;
    }
    h1, h2, h3 { margin: 10px 0; }
    .button {
      background: #3498db; border: none; color: white;
      padding: 12px 20px; font-size: 20px;
      margin: 5px; cursor: pointer; border-radius: 6px;
      transition: background 0.3s;
    }
    .button:hover { background: #2980b9; }
    input[type="text"], input[type="range"] {
      padding: 10px; font-size: 16px; margin: 8px auto;
      width: 80%; border-radius: 5px; border: 1px solid #ccc;
      display: block;
    }
    .panel-grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
      gap: 20px;
      max-width: 1200px;
      margin: 20px auto 40px;
      padding: 0 10px;
      justify-items: center;
    }
    .control-panel, .sensor-panel, .form-panel, .video-panel {
      background: rgba(255,255,255,0.9);
      padding: 20px;
      border-radius: 10px;
      box-shadow: 0 2px 8px rgba(0,0,0,0.1);
      width: 100%; max-width: 400px;
      box-sizing: border-box;
    }
    .joystick {
      display: grid;
      grid-template-columns: 1fr 1fr 1fr;
      justify-items: center;
      margin-bottom: 10px;
    }
    .status {
      font-weight: bold;
      padding: 5px 10px;
      border-radius: 4px;
      display: inline-block;
      min-width: 70px;
    }
    .active { background: #2ecc71; color: white; }
    .inactive { background: #e74c3c; color: white; }
    iframe {
      border: none;
      border-radius: 8px;
      max-width: 100%;
      height: auto;
    }
  </style>
  <script>
    const sendCommand = (path) => fetch(path);

    function moveForward()  { sendCommand('/forward'); }
    function moveLeft()     { sendCommand('/left'); }
    function stopRobot()    { sendCommand('/stop'); }
    function moveRight()    { sendCommand('/right'); }
    function moveReverse()  { sendCommand('/reverse'); }

    function updateMotorSpeed(pos) {
      document.getElementById('motorSpeed').textContent = pos;
      sendCommand('/speed?value=' + pos);
    }

    function fetchSensorData() {
      fetch('/sensor')
        .then(res => res.json())
        .then(data => {
          ['ir1','ir2','ir3','ir4'].forEach((id, idx) => {
            const el = document.getElementById(id);
            const state = data.irValues[idx];
            el.textContent = state ? 'EMPTY' : 'FILLED';
            el.className = 'status ' + (state ? 'active' : 'inactive');
          });
        })
        .catch(console.error);
    }

    setInterval(fetchSensorData, 1000);

    function controlBuzzer(state) { sendCommand('/buzzer?state=' + state); }
    function controlLight(state) { sendCommand('/light?state=' + state); }
  </script>
</head>
<body>
  <h1>ISOLATED - AREA SHIPPING ROBOT</h1>
  <div class="panel-grid">
    <div class="control-panel">
      <h2>Motor Control</h2>
      <div class="joystick">
        <div></div>
        <button class="button" ontouchstart="moveForward()" ontouchend="stopRobot()" onmousedown="moveForward()" onmouseup="stopRobot()">↑</button>
        <div></div>
        <button class="button" ontouchstart="moveLeft()" ontouchend="stopRobot()" onmousedown="moveLeft()" onmouseup="stopRobot()">←</button>
        <button class="button" onclick="stopRobot()" style="visibility:hidden;">             </button>
        <button class="button" ontouchstart="moveRight()" ontouchend="stopRobot()" onmousedown="moveRight()" onmouseup="stopRobot()">→</button>
        <div></div>
        <button class="button" ontouchstart="moveReverse()" ontouchend="stopRobot()" onmousedown="moveReverse()" onmouseup="stopRobot()">↓</button>
        <div></div>
      </div>
      <p>Motor Speed: <span id="motorSpeed">0</span></p>
      <input type="range" min="0" max="100" step="1" value="0" oninput="updateMotorSpeed(this.value)" />
    </div>
    <div class="sensor-panel">
      <h2>Sensor Status</h2>
      <h3>IR Sensors</h3>
      <p>IR Sensor 1: <span id="ir1" class="status inactive">-</span></p>
      <p>IR Sensor 2: <span id="ir2" class="status inactive">-</span></p>
      <p>IR Sensor 3: <span id="ir3" class="status inactive">-</span></p>
      <p>IR Sensor 4: <span id="ir4" class="status inactive">-</span></p>
    </div>
    <div class="control-panel">
      <h2>Buzzer Control</h2>
      <button class="button" ontouchstart="controlBuzzer(1)" ontouchend="controlBuzzer(0)" onmousedown="controlBuzzer(1)" onmouseup="controlBuzzer(0)">BUZZER</button>
      <h2>Light Control</h2>
      <button class="button" onclick="controlLight(1)">ON</button>
      <button class="button" onclick="controlLight(0)">OFF</button>
    </div>
  </div>
  <div class="panel-grid" style="max-width:900px;">
    <div class="video-panel control-panel">
      <h2>STREAM VIDEO</h2>
      <iframe src="http://192.168.1.41" width="400" height="296" allowfullscreen></iframe>
    </div>
    <div class="form-panel control-panel">
      <h2>Patient Info</h2>
      <form action="/update" method="POST">
        <input type="text" name="name" placeholder="Enter Name" required />
        <input type="text" name="room" placeholder="Enter Room Number" required />
        <input type="text" name="slot" placeholder="Enter Slot Number" required />
        <button class="button" type="submit">Update LCD</button>
      </form>
    </div>
  </div>
</body>
</html>)rawliteral";
  server.send(200, "text/html", html);
}

// Motor control task
void motorControlTask(void * parameter) {
  MotorControl motorControl;
  while (1) {
    if (xQueueReceive(motorQueue, &motorControl, portMAX_DELAY)) {
      switch (motorControl.command) {
        case FORWARD:
          digitalWrite(motor1Pin1, LOW);
          digitalWrite(motor1Pin2, HIGH); 
          digitalWrite(motor2Pin1, LOW);
          digitalWrite(motor2Pin2, HIGH);
          break;
        case LEFT:
          digitalWrite(motor1Pin1, HIGH); 
          digitalWrite(motor1Pin2, LOW); 
          digitalWrite(motor2Pin1, LOW);
          digitalWrite(motor2Pin2, HIGH);
          break;
        case STOP:
          digitalWrite(motor1Pin1, LOW); 
          digitalWrite(motor1Pin2, LOW); 
          digitalWrite(motor2Pin1, LOW);
          digitalWrite(motor2Pin2, LOW);   
          break;
        case RIGHT:
          digitalWrite(motor1Pin1, LOW); 
          digitalWrite(motor1Pin2, HIGH); 
          digitalWrite(motor2Pin1, HIGH);
          digitalWrite(motor2Pin2, LOW);    
          break;
        case REVERSE:
          digitalWrite(motor1Pin1, HIGH);
          digitalWrite(motor1Pin2, LOW); 
          digitalWrite(motor2Pin1, HIGH);
          digitalWrite(motor2Pin2, LOW);          
          break;
        case SPEED:
          if (motorControl.speed == 0) {
            ledcWrite(enable1Pin, 0);
            ledcWrite(enable2Pin, 0);
          } else { 
            dutyCycle = map(motorControl.speed, 1, 100, 150, 255);
            ledcWrite(enable1Pin, dutyCycle);
            ledcWrite(enable2Pin, dutyCycle);
          }
          break;
      }
    }
  }
}

// Sensor reading task
void sensorReadingTask(void * parameter) {
  SensorData sensorData;
  while (1) {
    for (int i = 0; i < 4; i++) {
      sensorData.irValues[i] = digitalRead(irSensorPins[i]);
    }
    xQueueSend(sensorQueue, &sensorData, portMAX_DELAY);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

// Handle motor control commands from web server
void handleForward() {
  MotorControl motorControl = {FORWARD, 0};
  xQueueSend(motorQueue, &motorControl, portMAX_DELAY);
  server.send(200);
}

void handleLeft() {
  MotorControl motorControl = {LEFT, 0};
  xQueueSend(motorQueue, &motorControl, portMAX_DELAY);
  server.send(200);
}

void handleStop() {
  MotorControl motorControl = {STOP, 0};
  xQueueSend(motorQueue, &motorControl, portMAX_DELAY);
  server.send(200);
}

void handleRight() {
  MotorControl motorControl = {RIGHT, 0};
  xQueueSend(motorQueue, &motorControl, portMAX_DELAY);
  server.send(200);
}

void handleReverse() {
  MotorControl motorControl = {REVERSE, 0};
  xQueueSend(motorQueue, &motorControl, portMAX_DELAY);
  server.send(200);
}

void handleSpeed() {
  if (server.hasArg("value")) {
    valueString = server.arg("value");
    int value = valueString.toInt();
    MotorControl motorControl = {SPEED, value};
    xQueueSend(motorQueue, &motorControl, portMAX_DELAY);
  }
  server.send(200);
}

// Handle sensor data request from web server
void handleSensor() {
  SensorData sensorData;
  if (xQueueReceive(sensorQueue, &sensorData, 0)) {
    String json = "{\"irValues\":[" + String(sensorData.irValues[0]) + "," + String(sensorData.irValues[1]) + "," + String(sensorData.irValues[2]) + "," + String(sensorData.irValues[3]) + "]}";
    server.send(200, "application/json", json);
  } else {
    server.send(500, "application/json", "{\"error\":\"No sensor data available\"}");
  }
}

// Control buzzer (on/off)
void handleBuzzer() {
  if (server.hasArg("state")) {
    int state = server.arg("state").toInt();
    digitalWrite(buzzPin, state);
  }
  server.send(200);
}

// Control light (on/off)
void handleLight() {
  if (server.hasArg("state")) {
    int state = server.arg("state").toInt();
    digitalWrite(lightPin, state);
  }
  server.send(200);
}

//Read string form webserver to display on LCD
void handleUpdate() {
  if (server.hasArg("name") && server.hasArg("room") && server.hasArg("slot")) {
    String Pname = server.arg("name");
    String Proom = server.arg("room");
    String Pslot = server.arg("slot");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ISO.AREA SHIP-ROBOT");
    lcd.setCursor(0, 1);
    lcd.print("Name: " + Pname);
    lcd.setCursor(0, 2);
    lcd.print("Room: " + Proom);
    lcd.setCursor(0, 3);
    lcd.print("Slot: " + Pslot);

    for (int i = 0; i < 4; i++) {
      digitalWrite(slotPins[i], LOW);
    
    int slotNumber = server.arg("slot").toInt();
    if (slotNumber >= 1 && slotNumber <= 4) {
      digitalWrite(slotPins[slotNumber - 1], HIGH);
  }
  server.send(200);
}
}
}

void setup() {
  Serial.begin(115200);
  pinMode(motor1Pin1, OUTPUT);
  pinMode(motor1Pin2, OUTPUT);
  pinMode(motor2Pin1, OUTPUT);
  pinMode(motor2Pin2, OUTPUT);
  pinMode(buzzPin, OUTPUT);
  pinMode(lightPin, OUTPUT);
  
  for (int i = 0; i < 4; i++) {
  pinMode(slotPins[i], OUTPUT);
  
  digitalWrite(slotPins[i], LOW);
  }
  lcd.init();
  lcd.backlight();
  
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

  // Create queues
  motorQueue = xQueueCreate(10, sizeof(MotorControl));
  sensorQueue = xQueueCreate(10, sizeof(SensorData));

  // Create tasks
  xTaskCreatePinnedToCore(
    motorControlTask,   // Function to implement the task
    "MotorControl",     // Name of the task
    10000,             // Stack size in words
    NULL,              // Task input parameter
    1,                 // Priority of the task
    &Task1,            // Task handle
    0);                // Core where the task should run

  xTaskCreatePinnedToCore(sensorReadingTask, "SensorReading", 10000, NULL, 1, &Task2, 0);

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
  server.on("/update", HTTP_POST, handleUpdate);

  server.begin();
}

void loop() {
  server.handleClient();
}
