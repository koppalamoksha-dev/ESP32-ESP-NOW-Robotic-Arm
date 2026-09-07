/*
  MASTER ROBOTIC ARM CODE - DEBUGGED & OPTIMIZED
  Based on DroneBot Workshop ESP-NOW Demo
  Controls a robotic arm via ESP-NOW protocol
*/

#include <esp_now.h>
#include <WiFi.h>

// ========== CONFIGURATION ==========
// MAC Address of SLAVE - UPDATE WITH YOUR SLAVE'S MAC ADDRESS
uint8_t broadcastAddress[] = {0x68, 0x09, 0x47, 0x9D, 0xB8, 0x88};

// GPIO Pins for Potentiometers and Button
#define POT_BASE 34        // Base rotation potentiometer
#define POT_SHOULDER 35    // Shoulder potentiometer
#define POT_ELBOW 32       // Elbow potentiometer
#define GRIPPER_BUTTON 4   // Gripper control button

// Update frequency (milliseconds)
#define UPDATE_RATE 20     // 50Hz update rate

// ========== DATA STRUCTURE ==========
typedef struct struct_message {
  int angleBase;       // Base rotation angle (0-180)
  int angleShoulder;   // Shoulder angle (0-180)
  int angleElbow;      // Elbow angle (0-180)
  int gripperState;    // Gripper state (0=Open, 1=Close)
} struct_message;

struct_message myData;

// Peer info
esp_now_peer_info_t peerInfo;

// ========== VARIABLES ==========
unsigned long lastSendTime = 0;
int failureCount = 0;
const int MAX_FAILURES = 5;

// ========== CALLBACK FUNCTION ==========
// Called when data is sent to slave
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("[");
  Serial.print(millis());
  Serial.print("] Last Packet Send Status: ");
  
  if (status == ESP_NOW_SEND_SUCCESS) {
    Serial.println("✓ Delivery Success");
    failureCount = 0; // Reset failure counter
  } else {
    Serial.println("✗ Delivery Failed");
    failureCount++;
    
    if (failureCount >= MAX_FAILURES) {
      Serial.println("WARNING: Multiple send failures - check slave connection!");
      failureCount = 0;
    }
  }
}

// ========== SETUP FUNCTION ==========
void setup() {
  
  // Initialize Serial Monitor
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n========================================");
  Serial.println("ESP32 ESP-NOW MASTER - ROBOTIC ARM");
  Serial.println("========================================\n");
  
  // Configure GPIO pins
  Serial.println("Configuring GPIO pins...");
  pinMode(POT_BASE, INPUT);
  pinMode(POT_SHOULDER, INPUT);
  pinMode(POT_ELBOW, INPUT);
  pinMode(GRIPPER_BUTTON, INPUT_PULLUP);
  
  // Print ESP32 MAC Address
  Serial.print("Master MAC Address: ");
  Serial.println(WiFi.macAddress());
  Serial.print("Slave MAC Address: ");
  printMacAddress(broadcastAddress);
  
  // Set ESP32 as Wi-Fi Station
  Serial.println("\nInitializing Wi-Fi in STA mode...");
  WiFi.mode(WIFI_STA);
  WiFi.setChannel(1);      // Channel must match slave
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
  
  // Register send callback
  if (esp_now_register_send_cb(OnDataSent) != ESP_OK) {
    Serial.println("✗ Error registering send callback");
    return;
  }
  Serial.println("✓ Send callback registered");
  
  // Register peer (slave)
  Serial.println("\nRegistering peer (slave)...");
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 1;      // MUST MATCH slave's channel
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("✗ Failed to add peer");
    return;
  }
  
  Serial.println("✓ Peer registered successfully");
  Serial.println("\n========================================");
  Serial.println("MASTER READY - Waiting for input...");
  Serial.println("========================================\n");
  
  lastSendTime = millis();
}

// ========== MAIN LOOP ==========
void loop() {
  
  // Check if it's time to send data
  if (millis() - lastSendTime >= UPDATE_RATE) {
    lastSendTime = millis();
    
    // Read Potentiometer values (0-4095)
    int valBase = analogRead(POT_BASE);
    int valShoulder = analogRead(POT_SHOULDER);
    int valElbow = analogRead(POT_ELBOW);
    
    // Map potentiometer values to servo angles
    // Adjust these ranges based on your potentiometer calibration
    myData.angleBase = map(constrain(valBase, 0, 4095), 0, 4095, 180, 0);
    myData.angleShoulder = map(constrain(valShoulder, 407, 3355), 407, 3355, 140, 19);
    myData.angleElbow = map(constrain(valElbow, 2005, 3649), 2005, 3649, 118, 55);
    
    // Read Gripper button (active LOW due to INPUT_PULLUP)
    if (digitalRead(GRIPPER_BUTTON) == LOW) {
      myData.gripperState = 1; // Button pressed -> Close gripper
    } else {
      myData.gripperState = 0; // Button released -> Open gripper
    }
    
    // Send data via ESP-NOW
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
    
    if (result == ESP_OK) {
      Serial.print("✓ [");
      Serial.print(millis());
      Serial.print("ms] Base: ");
      Serial.print(myData.angleBase);
      Serial.print("° | Shoulder: ");
      Serial.print(myData.angleShoulder);
      Serial.print("° | Elbow: ");
      Serial.print(myData.angleElbow);
      Serial.print("° | Gripper: ");
      Serial.println(myData.gripperState == 1 ? "CLOSED" : "OPEN");
    }
    else {
      Serial.print("✗ Send Error Code: ");
      Serial.println(result);
    }
  }
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
