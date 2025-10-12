#include <esp_now.h>
#include <WiFi.h>

// Button configuration
#define BUTTON_PIN 15        // Button input pin
#define LED_PIN 2            // Built-in LED control pin (for boost converter enable)
#define BOOST_EN_PIN 4       // Boost converter enable pin

// Base station MAC address - UPDATE THIS WITH YOUR BASE STATION'S MAC!
// Get this from base station's Serial Monitor output
// Format: Copy from "AA:BB:CC:DD:EE:FF" to {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}
uint8_t baseStationMAC[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Button state
volatile bool buttonPressed = false;
bool ledState = false;
unsigned long lastPressTime = 0;
const unsigned long debounceDelay = 50;

// Game state
enum ButtonState {
  WAITING,      // Waiting for game to start
  READY,        // Game active, ready to buzz
  WINNER,       // This button buzzed first
  LOCKED_OUT    // Another button buzzed first
};

ButtonState currentState = WAITING;

// Interrupt handler for button press
void IRAM_ATTR buttonISR() {
  buttonPressed = true;
}

// ESP-NOW callback for receiving data from base station (ESP32-C6 compatible)
void onDataRecv(const esp_now_recv_info *recv_info, const uint8_t *incomingData, int len) {
  if (len < 1) return;
  
  uint8_t response = incomingData[0];
  
  switch (response) {
    case 0: // Locked out
      currentState = LOCKED_OUT;
      setLED(false); // Turn off LED
      break;
      
    case 1: // Winner!
      currentState = WINNER;
      blinkLED(5); // Blink 5 times to celebrate
      setLED(true); // Keep LED on
      break;
      
    case 2: // Reset
      currentState = READY;
      setLED(false);
      break;
  }
}

void onDataSent(const uint8_t *mac, esp_now_send_status_t status) {
  Serial.print("Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void setLED(bool state) {
  ledState = state;
  digitalWrite(BOOST_EN_PIN, state ? HIGH : LOW);
  digitalWrite(LED_PIN, state ? HIGH : LOW);
}

void blinkLED(int times) {
  for (int i = 0; i < times; i++) {
    setLED(true);
    delay(100);
    setLED(false);
    delay(100);
  }
}

void sendBuzzer() {
  uint8_t data = 1; // Simple buzz signal
  esp_err_t result = esp_now_send(baseStationMAC, &data, sizeof(data));
  
  if (result == ESP_OK) {
    Serial.println("Buzz sent successfully");
  } else {
    Serial.println("Error sending buzz");
  }
}

void setup() {
  Serial.begin(115200);
  
  // Configure button pin with pull-down
  pinMode(BUTTON_PIN, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, RISING);
  
  // Configure LED control pins
  pinMode(LED_PIN, OUTPUT);
  pinMode(BOOST_EN_PIN, OUTPUT);
  setLED(false);
  
  // Set device as Wi-Fi station
  WiFi.mode(WIFI_STA);
  
  // Print MAC address for registration with base station
  Serial.print("Button MAC Address: ");
  Serial.println(WiFi.macAddress());
  
  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }
  
  // Register callbacks
  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataRecv);
  
  // Register base station as peer
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, baseStationMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  
  // Initial state
  currentState = WAITING;
  
  // Blink LED twice to show ready
  blinkLED(2);
  
  Serial.println("Button Ready");
}

void loop() {
  // Check for button press with debouncing
  if (buttonPressed) {
    unsigned long currentTime = millis();
    
    if (currentTime - lastPressTime > debounceDelay) {
      lastPressTime = currentTime;
      
      // Only send buzz if in READY state
      if (currentState == READY) {
        Serial.println("Button pressed - sending buzz");
        sendBuzzer();
        
        // Brief LED flash to acknowledge press
        setLED(true);
        delay(50);
        setLED(false);
      } else if (currentState == WAITING) {
        // Flash LED to indicate game hasn't started
        blinkLED(1);
      }
    }
    
    buttonPressed = false;
  }
  
  // Small delay to prevent watchdog issues
  delay(10);
}

// Optional: Add deep sleep for power saving when not in use
void enterDeepSleep() {
  Serial.println("Entering deep sleep...");
  setLED(false);
  esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_PIN, HIGH);
  esp_deep_sleep_start();
}