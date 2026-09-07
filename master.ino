/*
  MASTER ROBOTIC ARM CODE - SINGLE VALUE CONTROL
  Reads ONE potentiometer and transmits ONLY when value changes
  Error-checked and optimized for accuracy
*/

#include <esp_now.h>
#include <WiFi.h>

// ========== CONFIGURATION ==========
// MAC Address of SLAVE - UPDATE WITH YOUR SLAVE'S MAC ADDRESS
uint8_t broadcastAddress[] = {0x68, 0x09, 0x47, 0x9D, 0xB8, 0x88};

// GPIO Pin for single Potentiometer
#define POT_PIN 34        // Potentiometer analog input

// Update frequency and sensitivity
#define UPDATE_RATE 50    // Check potentiometer every 50ms
#define THRESHOLD 2       // Only send if change > 2 units (reduces noise)

// ========== DATA STRUCTURE ==========
typedef struct struct_message {
  int servoAngle;       // Single servo angle value (0-180)
} struct_message;

struct_message myData;

// Peer info
esp_now_peer_info_t peerInfo;

// ========== VARIABLES ==========
unsigned long lastSendTime = 0;
unsigned long lastCheckTime = 0;
int lastSentAngle = -1;  // Store last sent angle to detect changes
int sendCount = 0;

// ========== CALLBACK FUNCTION ==========
// Called when data is sent to slave
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) {
    Serial.print("✓ Packet #");
    Serial.print(sendCount);
    Serial.print(" sent successfully | Angle: ");
    Serial.print(myData.servoAngle);
    Serial.println("°");
  } else {
    Serial.print("✗ Packet #");
    Serial.print(sendCount);
    Serial.println(" - Delivery FAILED!");
  }
}

// ========== SETUP FUNCTION ==========
void setup() {
  
  // Initialize Serial Monitor
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n========================================");
  Serial.println("ESP32 ESP-NOW MASTER - SINGLE VALUE");
  Serial.println("========================================\n");
  
  // Configure GPIO pin
  Serial.println("Configuring ADC pin...");
  pinMode(POT_PIN, INPUT);
  
  // Print ESP32 MAC Address
  Serial.print("Master MAC Address: ");
  Serial.println(WiFi.macAddress());
  Serial.print("Slave MAC Address: ");
  printMacAddress(broadcastAddress);
  
  // Set ESP32 as Wi-Fi Station
  Serial.println("\nInitializing Wi-Fi in STA mode...");
  WiFi.mode(WIFI_STA);
  WiFi.setChannel(1);      // Channel must match slave
  WiFi.setSleep(false);    // Disable Wi-Fi sleep
  
  // Initialize ESP-NOW
  Serial.println("Initializing ESP-NOW...");
  if (esp_now_init() != ESP_OK) {
    Serial.println("✗ FATAL ERROR: ESP-NOW initialization failed!");
    return;
  }
  
  Serial.println("✓ ESP-NOW initialized");
  
  // Register send callback
  if (esp_now_register_send_cb(OnDataSent) != ESP_OK) {
    Serial.println("✗ ERROR: Could not register send callback");
    return;
  }
  Serial.println("✓ Send callback registered");
  
  // Register peer (slave)
  Serial.println("\nRegistering peer (slave device)...");
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 1;
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("✗ FATAL ERROR: Failed to add peer!");
    return;
  }
  
  Serial.println("✓ Peer registered successfully");
  Serial.println("\n========================================");
  Serial.println("MASTER READY - Move potentiometer...");
  Serial.println("========================================\n");
  
  lastCheckTime = millis();
}

// ========== MAIN LOOP ==========
void loop() {
  
  // Check potentiometer at defined interval
  if (millis() - lastCheckTime >= UPDATE_RATE) {
    lastCheckTime = millis();
    
    // Read potentiometer value (0-4095)
    int rawValue = analogRead(POT_PIN);
    
    // Map to servo angle range (0-180)
    // Adjust the input range (0, 4095) based on your potentiometer calibration
    int currentAngle = map(constrain(rawValue, 0, 4095), 0, 4095, 0, 180);
    
    // Only send if angle changed by more than threshold
    if (abs(currentAngle - lastSentAngle) > THRESHOLD) {
      
      lastSentAngle = currentAngle;
      myData.servoAngle = currentAngle;
      sendCount++;
      
      // Send data via ESP-NOW
      esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
      
      if (result != ESP_OK) {
        Serial.print("✗ ERROR CODE: ");
        Serial.print(result);
        Serial.print(" | Attempting to send angle: ");
        Serial.println(currentAngle);
      }
      
      lastSendTime = millis();
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
