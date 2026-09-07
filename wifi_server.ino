/*
  ESP32 WEB SERVER - ROBOTIC ARM CONTROL VIA MOBILE
  Access from mobile browser: http://<ESP32_IP_ADDRESS>
  Controls single servo via WiFi with web interface
*/

#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>

// ========== WIFI CONFIGURATION ==========
const char* ssid = "Your_WiFi_SSID";           // Change to your WiFi name
const char* password = "Your_WiFi_Password";   // Change to your WiFi password

// ========== SERVO CONFIGURATION ==========
Servo servo;
const int SERVO_PIN = 13;
const int SERVO_MIN = 0;
const int SERVO_MAX = 180;

// ========== WEB SERVER ==========
WebServer server(80);  // Server on port 80

// ========== VARIABLES ==========
int currentAngle = 90;  // Current servo position
int receiveCount = 0;

// ========== HTML PAGE ==========
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Robotic Arm Control</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            justify-content: center;
            align-items: center;
            padding: 20px;
        }
        
        .container {
            background: white;
            border-radius: 20px;
            box-shadow: 0 20px 60px rgba(0, 0, 0, 0.3);
            padding: 40px;
            max-width: 500px;
            width: 100%;
        }
        
        .header {
            text-align: center;
            margin-bottom: 40px;
        }
        
        .header h1 {
            color: #333;
            font-size: 32px;
            margin-bottom: 10px;
        }
        
        .header p {
            color: #666;
            font-size: 14px;
        }
        
        .status-box {
            background: #f0f0f0;
            border-left: 4px solid #667eea;
            padding: 15px;
            border-radius: 8px;
            margin-bottom: 30px;
        }
        
        .status-label {
            color: #666;
            font-size: 12px;
            text-transform: uppercase;
            letter-spacing: 1px;
            margin-bottom: 5px;
        }
        
        .status-value {
            color: #333;
            font-size: 28px;
            font-weight: bold;
        }
        
        .control-section {
            margin-bottom: 30px;
        }
        
        .control-label {
            color: #333;
            font-size: 14px;
            font-weight: 600;
            margin-bottom: 15px;
            display: block;
        }
        
        .slider-container {
            position: relative;
        }
        
        input[type="range"] {
            width: 100%;
            height: 8px;
            border-radius: 5px;
            background: linear-gradient(to right, #667eea, #764ba2);
            outline: none;
            -webkit-appearance: none;
            appearance: none;
        }
        
        input[type="range"]::-webkit-slider-thumb {
            -webkit-appearance: none;
            appearance: none;
            width: 25px;
            height: 25px;
            border-radius: 50%;
            background: #667eea;
            cursor: pointer;
            box-shadow: 0 2px 8px rgba(102, 126, 234, 0.4);
            transition: all 0.3s ease;
        }
        
        input[type="range"]::-webkit-slider-thumb:active {
            transform: scale(1.2);
            box-shadow: 0 4px 12px rgba(102, 126, 234, 0.6);
        }
        
        input[type="range"]::-moz-range-thumb {
            width: 25px;
            height: 25px;
            border-radius: 50%;
            background: #667eea;
            cursor: pointer;
            border: none;
            box-shadow: 0 2px 8px rgba(102, 126, 234, 0.4);
            transition: all 0.3s ease;
        }
        
        input[type="range"]::-moz-range-thumb:active {
            transform: scale(1.2);
        }
        
        .range-labels {
            display: flex;
            justify-content: space-between;
            font-size: 12px;
            color: #999;
            margin-top: 8px;
        }
        
        .button-group {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 10px;
            margin-bottom: 20px;
        }
        
        button {
            padding: 12px 24px;
            font-size: 14px;
            font-weight: 600;
            border: none;
            border-radius: 8px;
            cursor: pointer;
            transition: all 0.3s ease;
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }
        
        .btn-preset {
            background: #667eea;
            color: white;
        }
        
        .btn-preset:hover {
            background: #5568d3;
            transform: translateY(-2px);
            box-shadow: 0 8px 16px rgba(102, 126, 234, 0.3);
        }
        
        .btn-preset:active {
            transform: translateY(0);
        }
        
        .full-width {
            grid-column: 1 / -1;
        }
        
        .info-panel {
            background: #e8f4f8;
            border-left: 4px solid #00bcd4;
            padding: 15px;
            border-radius: 8px;
            font-size: 12px;
            color: #00695c;
            line-height: 1.6;
        }
        
        .connection-indicator {
            display: flex;
            align-items: center;
            margin-bottom: 20px;
            padding: 10px;
            background: #f5f5f5;
            border-radius: 8px;
        }
        
        .indicator-dot {
            width: 12px;
            height: 12px;
            border-radius: 50%;
            background: #4caf50;
            margin-right: 10px;
            animation: pulse 2s infinite;
        }
        
        .indicator-dot.offline {
            background: #f44336;
            animation: none;
        }
        
        @keyframes pulse {
            0%, 100% {
                opacity: 1;
            }
            50% {
                opacity: 0.5;
            }
        }
        
        .indicator-text {
            font-size: 12px;
            color: #666;
        }
        
        .angle-display {
            text-align: center;
            padding: 15px;
            background: #f9f9f9;
            border-radius: 8px;
            margin-bottom: 20px;
        }
        
        .angle-number {
            font-size: 36px;
            font-weight: bold;
            color: #667eea;
        }
        
        .angle-unit {
            font-size: 14px;
            color: #999;
        }
        
        @media (max-width: 480px) {
            .container {
                padding: 25px;
            }
            
            .header h1 {
                font-size: 24px;
            }
            
            button {
                padding: 10px 20px;
                font-size: 12px;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🤖 Robotic Arm</h1>
            <p>WiFi Control System</p>
        </div>
        
        <div class="connection-indicator">
            <div class="indicator-dot" id="statusDot"></div>
            <span class="indicator-text" id="statusText">Connected</span>
        </div>
        
        <div class="angle-display">
            <div class="angle-number" id="angleValue">90</div>
            <div class="angle-unit">Degrees</div>
        </div>
        
        <div class="control-section">
            <label class="control-label">Servo Position Control</label>
            <div class="slider-container">
                <input type="range" id="servoSlider" min="0" max="180" value="90" step="1">
                <div class="range-labels">
                    <span>0°</span>
                    <span>90°</span>
                    <span>180°</span>
                </div>
            </div>
        </div>
        
        <div class="button-group">
            <button class="btn-preset" onclick="moveServo(0)">Min (0°)</button>
            <button class="btn-preset" onclick="moveServo(45)">45°</button>
            <button class="btn-preset" onclick="moveServo(90)">Center (90°)</button>
            <button class="btn-preset" onclick="moveServo(135)">135°</button>
            <button class="btn-preset" onclick="moveServo(180)">Max (180°)</button>
        </div>
        
        <div class="info-panel">
            <strong>ℹ️ Instructions:</strong><br>
            • Drag the slider to control servo position<br>
            • Use preset buttons for quick positioning<br>
            • Real-time angle feedback displayed above<br>
            • Commands sent instantly via WiFi
        </div>
    </div>

    <script>
        const servoSlider = document.getElementById('servoSlider');
        const angleValue = document.getElementById('angleValue');
        const statusDot = document.getElementById('statusDot');
        const statusText = document.getElementById('statusText');
        
        let isUpdating = false;
        
        // Update angle display when slider changes
        servoSlider.addEventListener('input', function() {
            angleValue.textContent = this.value;
            
            if (!isUpdating) {
                isUpdating = true;
                sendAngle(this.value);
                
                // Debounce: prevent sending too many commands
                setTimeout(() => {
                    isUpdating = false;
                }, 100);
            }
        });
        
        // Send angle to ESP32
        function sendAngle(angle) {
            fetch(`/control?angle=${angle}`)
                .then(response => {
                    if (response.ok) {
                        updateStatus(true);
                    } else {
                        updateStatus(false);
                    }
                })
                .catch(error => {
                    console.error('Error:', error);
                    updateStatus(false);
                });
        }
        
        // Move servo to preset position
        function moveServo(angle) {
            servoSlider.value = angle;
            angleValue.textContent = angle;
            sendAngle(angle);
        }
        
        // Update connection status
        function updateStatus(connected) {
            if (connected) {
                statusDot.classList.remove('offline');
                statusText.textContent = 'Connected';
            } else {
                statusDot.classList.add('offline');
                statusText.textContent = 'Disconnected';
            }
        }
        
        // Check connection status every 5 seconds
        function checkStatus() {
            fetch('/status')
                .then(response => response.json())
                .then(data => {
                    updateStatus(data.connected);
                    angleValue.textContent = data.angle;
                    servoSlider.value = data.angle;
                })
                .catch(error => {
                    console.error('Status check failed:', error);
                    updateStatus(false);
                });
        }
        
        // Initial status check
        checkStatus();
        
        // Periodic status check
        setInterval(checkStatus, 5000);
    </script>
</body>
</html>
)rawliteral";

// ========== ROUTES ==========
void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

void handleControl() {
  if (server.hasArg("angle")) {
    String angleStr = server.arg("angle");
    int angle = angleStr.toInt();
    
    // Constrain angle
    angle = constrain(angle, SERVO_MIN, SERVO_MAX);
    currentAngle = angle;
    receiveCount++;
    
    // Move servo
    servo.write(angle);
    
    // Send response
    server.send(200, "application/json", "{\"status\":\"success\",\"angle\":" + String(angle) + "}");
    
    Serial.print("✓ Command #");
    Serial.print(receiveCount);
    Serial.print(" | Servo moved to: ");
    Serial.print(angle);
    Serial.println("°");
  } else {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing angle parameter\"}");
  }
}

void handleStatus() {
  String json = "{\"connected\":true,\"angle\":" + String(currentAngle) + ",\"packets\":" + String(receiveCount) + "}";
  server.send(200, "application/json", json);
}

void handleNotFound() {
  server.send(404, "text/plain", "Page Not Found");
}

// ========== SETUP ==========
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n========================================");
  Serial.println("ESP32 WiFi SERVO CONTROL - WEB SERVER");
  Serial.println("========================================\n");
  
  // Attach servo
  Serial.println("Initializing servo...");
  if (!servo.attach(SERVO_PIN, 1000, 2000)) {
    Serial.println("✗ FATAL ERROR: Could not attach servo!");
    while(1) delay(1000);
  }
  Serial.println("✓ Servo attached to pin 13");
  
  // Move to initial position
  servo.write(90);
  currentAngle = 90;
  delay(500);
  
  // Connect to WiFi
  Serial.println("\nConnecting to WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✓ WiFi connected!");
    Serial.print("ESP32 IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.println("\nOpen browser and go to: http://" + WiFi.localIP().toString());
  } else {
    Serial.println("\n✗ WiFi connection failed!");
  }
  
  // Setup routes
  server.on("/", handleRoot);
  server.on("/control", handleControl);
  server.on("/status", handleStatus);
  server.onNotFound(handleNotFound);
  
  // Start server
  server.begin();
  Serial.println("\n✓ Web server started on port 80");
  Serial.println("\n========================================");
  Serial.println("READY - Access from mobile browser!");
  Serial.println("========================================\n");
}

// ========== LOOP ==========
void loop() {
  server.handleClient();
}
