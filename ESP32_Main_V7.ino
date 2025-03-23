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
  const char html[] PROGMEM = R"rawliteral(
  <!DOCTYPE HTML><html>
  <head>
    <title>ISOLATED -  AREA SHIPPING ROBOT</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="icon" href="data:,">
    <style>
      html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center; }
      .button { -webkit-user-select: none; -moz-user-select: none; -ms-user-select: none; user-select: none; background-color: #4CAF50; border: none; color: white; padding: 12px 28px; text-decoration: none; font-size: 26px; margin: 1px; cursor: pointer; }
      .button2 {background-color: #FFFFFF;}
      input { padding: 10px; font-size: 16px; margin: 5px; width: 80%; }
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
  <h2>STREAM VIDEO</h2>
    <iframe 
        src="http://192.168.1.104/" 
        width="400" 
        height="296" 
        frameborder="0" 
        allowfullscreen>
    </iframe>
    
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

    <h2>Patient Info</h2>
    <form action="/update" method="POST">
      <input type="text" name="name" placeholder="Enter Name" required><br>
      <input type="text" name="room" placeholder="Enter Room Number" required><br>
      <input type="text" name="slot" placeholder="Enter Slot Number" required><br>
      <button class="button" type="submit">Update LCD</button>
    </form>
    
    <h2>Buzzer Control</h2>
    <button class="button" ontouchstart="controlBuzzer(1)"  ontouchend="controlBuzzer(0)" onmousedown="controlBuzzer(1)" onmouseup="controlBuzzer(0)">BUZZER</button>
   
    <h2>Light Control</h2>
    <button class="button" onclick="controlLight(1)">ON</button>
    <button class="button" onclick="controlLight(0)">OFF</button>

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

  xTaskCreatePinnedToCore(
    sensorReadingTask,  // Function to implement the task
    "SensorReading",    // Name of the task
    10000,             // Stack size in words
    NULL,              // Task input parameter
    1,                 // Priority of the task
    &Task2,            // Task handle
    1);                // Core where the task should run

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
