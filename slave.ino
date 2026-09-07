/*
  SLAVE ROBOTIC ARM CODE - SINGLE SERVO CONTROL
  Receives single angle value and moves servo with high accuracy
  Error-checked and optimized for smooth motion
*/

#include <esp_now.h>
#include <WiFi.h>
#include <ESP32Servo.h>

// ========== SERVO CONFIGURATION ==========
Servo servo;              // Single servo object
const int SERVO_PIN = 13; // Servo PWM pin

// Servo angle limits
const int SERVO_MIN = 0;
const int SERVO_MAX = 180;

// ========== DATA STRUCTURE (MUST MATCH MASTER) ==========
typedef struct struct_message {
  int servoAngle;       // Single angle value (0-180)
} struct_message;

struct_message myData;

// ========== VARIABLES ==========
unsigned long lastReceivedTime = 0;
int lastServoAngle = -1;  // Track current servo position
int receiveCount = 0;
const unsigned long CONNECTION_TIMEOUT = 3000; // 3 seconds
bool isConnected = false;

// ========== CALLBACK FUNCTION ==========
// Called when data is received from master
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingData, int len) {
  
  // Verify data size
  if (len != sizeof(myData)) {
    Serial.print("✗ ERROR: Data size mismatch! Expected: ");
    Serial.print(sizeof(myData));
    Serial.print(" bytes, Got: ");
    Serial.println(len);
    return;
  }
  
  // Copy received data
  memcpy(&myData, incomingData, sizeof(myData));
  receiveCount++;
  lastReceivedTime = millis();
  
  // Constrain angle to safe range
  int safeAngle = constrain(myData.servoAngle, SERVO_MIN, SERVO_MAX);
  
  // Only move servo if angle changed
  if (safeAngle != lastServoAngle) {
    lastServoAngle = safeAngle;
    
    // Move servo with precise position
    servo.write(safeAngle);
    
    // Provide feedback
    Serial.print("✓ Packet #");
    Serial.print(receiveCount);
    Serial.print(" | Received: ");
    Serial.print(myData.servoAngle);
    Serial.print("° | Servo moved to: ");
    Serial.print(safeAngle);
    Serial.println("°");
    
    isConnected = true;
  }
}

// ========== SETUP FUNCTION ==========
void setup() {
  
  // Initialize Serial Monitor
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n========================================");
  Serial.println("ESP32 ESP-NOW SLAVE - SINGLE SERVO");
  Serial.println("========================================\n");
  
  // Attach servo with error checking
  Serial.println("Initializing servo...");
  if (!servo.attach(SERVO_PIN, 1000, 2000)) {
    Serial.print("✗ FATAL ERROR: Could not attach servo to pin ");
    Serial.println(SERVO_PIN);
    while(1) delay(1000);
  }
  
  Serial.print("✓ Servo attached to pin ");
  Serial.println(SERVO_PIN);
  
  // Move servo to initial position (middle)
  Serial.println("Moving servo to home position (90°)...");
  servo.write(90);
  lastServoAngle = 90;
  delay(1000);
  
  // Print Slave MAC Address
  Serial.print("\nSlave MAC Address: ");
  Serial.println(WiFi.macAddress());
  
  // Set ESP32 as Wi-Fi Station
  Serial.println("\nInitializing Wi-Fi in STA mode...");
  WiFi.mode(WIFI_STA);
  WiFi.setChannel(1);      // MUST MATCH Master's channel
  WiFi.setSleep(false);    // Disable Wi-Fi sleep
  
  // Initialize ESP-NOW
  Serial.println("Initializing ESP-NOW...");
  if (esp_now_init() != ESP_OK) {
    Serial.println("✗ FATAL ERROR: ESP-NOW initialization failed!");
    while(1) delay(1000);
  }
  
  Serial.println("✓ ESP-NOW initialized");
  
  // Register receive callback
  if (esp_now_register_recv_cb(OnDataRecv) != ESP_OK) {
    Serial.println("✗ ERROR: Could not register receive callback");
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
  
  // Check for connection timeout
  unsigned long timeSinceLastPacket = millis() - lastReceivedTime;
  
  if (timeSinceLastPacket > CONNECTION_TIMEOUT && isConnected) {
    isConnected = false;
    Serial.println("\n⚠ WARNING: Connection lost from Master!");
    Serial.println("Moving servo to safe home position (90°)...\n");
    servo.write(90);
    lastServoAngle = 90;
  }
  
  // Connection restored
  if (timeSinceLastPacket <= CONNECTION_TIMEOUT && !isConnected && receiveCount > 0) {
    isConnected = true;
    Serial.println("\n✓ Connection restored with Master!\n");
  }
  
  delay(100); // Prevent watchdog timeout
}
