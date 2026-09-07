/*
  SLAVE ROBOTIC ARM CODE - DEBUGGED & OPTIMIZED
  Based on DroneBot Workshop ESP-NOW Demo
  Receives commands via ESP-NOW and controls servo motors
*/

#include <esp_now.h>
#include <WiFi.h>
#include <ESP32Servo.h>

// ========== SERVO OBJECTS ==========
Servo servo1; // Base rotation
Servo servo2; // Shoulder
Servo servo3; // Elbow
Servo servo4; // Gripper

// ========== SERVO PIN CONFIGURATION ==========
const int SERVO_PIN_BASE = 13;
const int SERVO_PIN_SHOULDER = 12;
const int SERVO_PIN_ELBOW = 14;
const int SERVO_PIN_GRIPPER = 27;

// ========== SERVO ANGLE LIMITS ==========
const int BASE_MIN = 0;
const int BASE_MAX = 180;

const int SHOULDER_MIN = 19;
const int SHOULDER_MAX = 140;

const int ELBOW_MIN = 55;
const int ELBOW_MAX = 118;

// ========== GRIPPER ANGLES ==========
const int GRIPPER_OPEN = 0;
const int GRIPPER_CLOSED = 180;

// ========== DATA STRUCTURE (MUST MATCH MASTER) ==========
typedef struct struct_message {
  int angleBase;       // Base rotation angle
  int angleShoulder;   // Shoulder angle
  int angleElbow;      // Elbow angle
  int gripperState;    // Gripper state (0=Open, 1=Close)
} struct_message;

struct_message myData;

// ========== VARIABLES ==========
unsigned long lastReceivedTime = 0;
unsigned long lastPrintTime = 0;
const unsigned long TIMEOUT = 2000; // Timeout if no data received (ms)
bool connectionLost = false;

// ========== CALLBACK FUNCTION ==========
// Called when data is received from master
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingData, int len) {
  
  // Verify data size
  if (len != sizeof(myData)) {
    Serial.print("✗ Error: Received data size mismatch. Expected: ");
    Serial.print(sizeof(myData));
    Serial.print(" bytes, Got: ");
    Serial.println(len);
    return;
  }
  
  // Copy received data
  memcpy(&myData, incomingData, sizeof(myData));
  lastReceivedTime = millis();
  connectionLost = false;
  
  // Constrain values to safe ranges
  int safeAngleBase = constrain(myData.angleBase, BASE_MIN, BASE_MAX);
  int safeAngleShoulder = constrain(myData.angleShoulder, SHOULDER_MIN, SHOULDER_MAX);
  int safeAngleElbow = constrain(myData.angleElbow, ELBOW_MIN, ELBOW_MAX);
  
  // Write angles to servos
  servo1.write(safeAngleBase);
  servo2.write(safeAngleShoulder);
  servo3.write(safeAngleElbow);
  
  // Control gripper
  int gripperAngle = (myData.gripperState == 1) ? GRIPPER_CLOSED : GRIPPER_OPEN;
  servo4.write(gripperAngle);
  
  // Print status every 500ms to avoid serial spam
  if (millis() - lastPrintTime >= 500) {
    lastPrintTime = millis();
    Serial.print("✓ [");
    Serial.print(millis());
    Serial.print("ms] Base: ");
    Serial.print(safeAngleBase);
    Serial.print("° | Shoulder: ");
    Serial.print(safeAngleShoulder);
    Serial.print("° | Elbow: ");
    Serial.print(safeAngleElbow);
    Serial.print("° | Gripper: ");
    Serial.println(gripperAngle == GRIPPER_CLOSED ? "CLOSED" : "OPEN");
  }
}

// ========== SETUP FUNCTION ==========
void setup() {
  
  // Initialize Serial Monitor
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n========================================");
  Serial.println("ESP32 ESP-NOW SLAVE - ROBOTIC ARM");
  Serial.println("========================================\n");
  
  // Attach servos with pin configuration
  Serial.println("Attaching servos...");
  if (!servo1.attach(SERVO_PIN_BASE)) {
    Serial.println("✗ Error: Could not attach servo1 (Base)");
  } else {
    Serial.println("✓ Servo1 (Base) attached to pin 13");
  }
  
  if (!servo2.attach(SERVO_PIN_SHOULDER)) {
    Serial.println("✗ Error: Could not attach servo2 (Shoulder)");
  } else {
    Serial.println("✓ Servo2 (Shoulder) attached to pin 12");
  }
  
  if (!servo3.attach(SERVO_PIN_ELBOW)) {
    Serial.println("✗ Error: Could not attach servo3 (Elbow)");
  } else {
    Serial.println("✓ Servo3 (Elbow) attached to pin 14");
  }
  
  if (!servo4.attach(SERVO_PIN_GRIPPER)) {
    Serial.println("✗ Error: Could not attach servo4 (Gripper)");
  } else {
    Serial.println("✓ Servo4 (Gripper) attached to pin 27");
  }
  
  // Move servos to initial position (all open)
  Serial.println("\nInitializing servos to home position...");
  servo1.write(90);      // Base - middle
  servo2.write(80);      // Shoulder - middle
  servo3.write(85);      // Elbow - middle
  servo4.write(GRIPPER_OPEN);  // Gripper - open
  delay(500);
  
  // Print Slave MAC Address
  Serial.print("\nSlave MAC Address: ");
  Serial.println(WiFi.macAddress());
  
  // Set ESP32 as Wi-Fi Station
  Serial.println("\nInitializing Wi-Fi in STA mode...");
  WiFi.mode(WIFI_STA);
  WiFi.setChannel(1);      // MUST MATCH Master's channel
  WiFi.setSleep(false);    // Disable Wi-Fi sleep for faster communication
  
  // Initialize ESP-NOW
  Serial.println("Initializing ESP-NOW...");
  if (esp_now_init() != ESP_OK) {
    Serial.println("✗ Error initializing ESP-NOW");
    while(1) {
      Serial.println("FATAL: Rebooting...");
      delay(1000);
    }
    return;
  }
  
  Serial.println("✓ ESP-NOW initialized successfully");
  
  // Register receive callback
  if (esp_now_register_recv_cb(OnDataRecv) != ESP_OK) {
    Serial.println("✗ Error registering receive callback");
    return;
  }
  
  Serial.println("✓ Receive callback registered");
  Serial.println("\n========================================");
  Serial.println("SLAVE READY - Waiting for commands...");
  Serial.println("========================================\n");
  
  lastReceivedTime = millis();
}

// ========== MAIN LOOP ==========
void loop() {
  
  // Check for connection timeout (no data received for 2 seconds)
  if (millis() - lastReceivedTime > TIMEOUT && !connectionLost) {
    connectionLost = true;
    Serial.println("\n⚠ WARNING: Connection lost from Master!");
    Serial.println("Moving arm to safe position (all servos to middle)...\n");
    
    // Move to safe position
    servo1.write(90);
    servo2.write(80);
    servo3.write(85);
    servo4.write(GRIPPER_OPEN);
  }
  
  // Connection restored
  if (millis() - lastReceivedTime < TIMEOUT && connectionLost) {
    connectionLost = false;
    Serial.println("\n✓ Connection restored with Master!\n");
  }
  
  delay(100); // Small delay to prevent watchdog trigger
}

// ========== HELPER FUNCTIONS ==========
// Print MAC address in readable format
void printMacAddress(const uint8_t *mac) {
  for (int i = 0; i < 6; i++) {
    if (mac[i] < 16) Serial.print("0");
    Serial.print(mac[i], HEX);
    if (i < 5) Serial.print(":");
  }
  Serial.println();
}
