#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "button-secret.h" // Commented out as requested

// Display configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Pin definitions for ESP32-C6
#define I2C_SDA 6
#define I2C_SCL 7
#define BUTTON_PIN 15 // Button input pin
#define LED_PIN 2 // Built-in LED control pin (for boost converter enable)
#define BOOST_EN_PIN 4 // Boost converter enable pin

// Base station MAC address
// Example Mac: 40:4c:ca:57:97:f0
// uint8_t baseStationMAC[] = {0x40, 0x4c, 0xca, 0x57, 0x97, 0xf0}; // ACTIVE MAC for testing
uint8_t baseStationMAC[] = BASE_STATION_MAC; // UNUSED

bool ledState = false;
unsigned long lastPressTime = 0;
const unsigned long debounceDelay = 50;

// FIX: Declare the flag used in the Interrupt Service Routine (ISR)
volatile bool buttonPressed = false;

// Battery management
unsigned long lastActivity = 0;
const unsigned long sleepTimeout = 300000; // 5 minutes of inactivity
const unsigned long lowBatteryCheck = 60000; // Check battery every minute
unsigned long lastBatteryCheck = 0; 

// Game state
enum ButtonState {
  WAITING, // Waiting for game to start
  READY, // Game active, ready to buzz
  WINNER, // This button buzzed first
  LOCKED_OUT // Another button buzzed first
};

ButtonState currentState = WAITING;

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  
  // Show MAC address (last 3 bytes for identification)
  String mac = WiFi.macAddress();
  display.printf("ID: %s\n", mac.substring(9).c_str());
  
  // Show battery voltage
  float voltage = getBatteryVoltage();
  display.printf("Bat: %.2fV\n", voltage);
  
  // Show current state
  display.setTextSize(1);
  switch (currentState) {
    case WAITING:
      display.println("Status: WAITING");
      display.println("Game not started");
      break;
      
    case READY:
      display.setTextSize(2);
      display.println("READY!");
      display.setTextSize(1);
      display.println("Press to buzz");
      break;
      
    case WINNER:
      display.setTextSize(2);
      display.println("WINNER!");
      display.setTextSize(1);
      display.println("You got it!");
      break;
      
    case LOCKED_OUT:
      display.println("Status: LOCKED");
      display.println("Too late!");
      break;
  }
  
  display.display();
}

// LED patterns for different states
void showStatePattern() {
  switch (currentState) {
    case WAITING:
      // Slow pulse every 3 seconds
      if (millis() % 3000 < 100) {
        setLED(true);
      } else {
        setLED(false);
      }
      break;
      
    case READY:
      // Fast blink to show ready
      if (millis() % 500 < 250) {
        setLED(true);
      } else {
        setLED(false);
      }
      break;
      
    case WINNER:
      // Solid on
      setLED(true);
      break;
      
    case LOCKED_OUT:
      // Off
      setLED(false);
      break;
  }
}

// Interrupt handler for button press
void IRAM_ATTR buttonISR() {
  buttonPressed = true;
}

// ESP-NOW callback for receiving data from base station (ESP32-C6 compatible)
void onDataRecv(const esp_now_recv_info *recv_info, const uint8_t *incomingData, int len) {
  if (len < 2) return;
  
  uint8_t response = incomingData[0];
  
  switch (response) {
    case 0: // Game stopped
      currentState = WAITING;
      setLED(false);
      updateDisplay();
      break;
      
    case 1: // Winner!
      currentState = WINNER;
      blinkLED(5); // Blink 5 times to celebrate
      setLED(true); // Keep LED on
      updateDisplay();
      break;
      
    case 2: // Locked out (another team won)
      currentState = LOCKED_OUT;
      setLED(false);
      updateDisplay();
      break;
      
    case 3: // Game active, ready to buzz
      currentState = READY;
      setLED(false);
      updateDisplay();
      break;
  }
}

// FIXED: Updated callback signature for ESP32 Arduino Core 3.x
void onDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
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
  uint8_t data[2];
  data[0] = 1; // Button press message type
  data[1] = 0; // Reserved for future use
  
  esp_err_t result = esp_now_send(baseStationMAC, data, sizeof(data));
  
  if (result == ESP_OK) {
    Serial.println("Buzz sent successfully");
  } else {
    Serial.println("Error sending buzz");
  }
}

void sendHeartbeat() {
  uint8_t data[2];
  data[0] = 2; // Heartbeat message type
  data[1] = 0; // Reserved for future use
  
  esp_err_t result = esp_now_send(baseStationMAC, data, sizeof(data));
  
  if (result == ESP_OK) {
    Serial.println("Heartbeat sent");
  } else {
    Serial.println("Error sending heartbeat");
  }
}

float getBatteryVoltage() {
  // Read battery voltage from ADC
  // ESP32-C6 has built-in voltage divider on GPIO7 for battery monitoring
  int adcValue = analogRead(A0); // Use A0 for battery voltage
  float voltage = (adcValue / 4095.0) * 3.3 * 2; // Convert to actual battery voltage
  return voltage;
}

void checkBatteryLevel() {
  float voltage = getBatteryVoltage();
  Serial.printf("Battery: %.2fV\n", voltage);
  
  if (voltage < 3.2) { // Low battery warning
    Serial.println("Low battery warning!");
    // Flash LED pattern for low battery
    for (int i = 0; i < 3; i++) {
      setLED(true);
      delay(100);
      setLED(false);
      delay(100);
    }
  }
  
  if (voltage < 3.0) { // Critical battery - enter sleep
    Serial.println("Critical battery - entering deep sleep");
    enterDeepSleep();
  }
}

void setup() {
  Serial.begin(115200);
  
  // Initialize I2C
  Wire.begin(I2C_SDA, I2C_SCL);
  
  // Initialize display
  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Button Starting...");
    display.display();
    delay(1000);
  } else {
    Serial.println("OLED display failed to initialize");
  }
  
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
  lastActivity = millis(); // Initialize activity timer
  
  // Blink LED twice to show ready
  blinkLED(2);
  
  Serial.println("Button Ready");
  Serial.printf("Initial battery: %.2fV\\n", getBatteryVoltage());
  
  // Update display to show ready state
  updateDisplay();
}

// Heartbeat timing
unsigned long lastHeartbeat = 0;
const unsigned long heartbeatInterval = 10000; // Send heartbeat every 10 seconds

void loop() {
  unsigned long currentTime = millis();
  
  // Check for button press with debouncing
  if (buttonPressed) {
    if (currentTime - lastPressTime > debounceDelay) {
      lastPressTime = currentTime;
      
      // Only send buzz if in READY state
      if (currentState == READY) {
        Serial.println("Button pressed - sending buzz");
        sendBuzzer();
        lastActivity = currentTime; // Reset activity timer
        
        // Brief LED flash to acknowledge press
        setLED(true);
        delay(50);
        setLED(false);
      } else if (currentState == WAITING) {
        // Flash LED to indicate game hasn't started
        blinkLED(1);
        Serial.println("Game not active - button press ignored");
      } else if (currentState == LOCKED_OUT) {
        // Brief red flash to show locked out
        blinkLED(1);
        Serial.println("Locked out - button press ignored");
      }
    }
    
    // Reset the flag
    buttonPressed = false;
  }
  
  // Send periodic heartbeat to base station
  if (currentTime - lastHeartbeat > heartbeatInterval) {
    sendHeartbeat();
    lastHeartbeat = currentTime;
    lastActivity = currentTime; // Reset activity timer on heartbeat
  }
  
  // Check battery level periodically
  if (currentTime - lastBatteryCheck > lowBatteryCheck) {
    checkBatteryLevel();
    lastBatteryCheck = currentTime;
  }
  
  // Check for sleep timeout (no activity for 5 minutes)
  if (currentTime - lastActivity > sleepTimeout) {
    Serial.println("Entering sleep due to inactivity");
    enterDeepSleep();
  }
  
  // Update LED pattern based on current state
  static unsigned long lastLedUpdate = 0;
  static unsigned long lastDisplayUpdate = 0;
  
  if (currentTime - lastLedUpdate > 50) { // Update LED every 50ms
    if (currentState != WINNER) { // Don't override winner LED
      showStatePattern();
    }
    lastLedUpdate = currentTime;
  }
  
  // Update display every 5 seconds to refresh battery voltage
  if (currentTime - lastDisplayUpdate > 5000) {
    updateDisplay();
    lastDisplayUpdate = currentTime;
  }
  
  // Small delay to prevent watchdog issues
  delay(10);
}

// FIXED: Deep sleep function for ESP32-C6
// Note: ESP32-C6 uses GPIO wakeup instead of ext0
void enterDeepSleep() {
  Serial.println("Entering deep sleep...");
  setLED(false);
  
  // Turn off boost converter to save power
  digitalWrite(BOOST_EN_PIN, LOW);
  
  // Configure GPIO wakeup (ESP32-C6 compatible)
  esp_sleep_enable_gpio_wakeup();
  gpio_wakeup_enable((gpio_num_t)BUTTON_PIN, GPIO_INTR_HIGH_LEVEL);
  
  // Also wake up after 30 minutes to send heartbeat
  esp_sleep_enable_timer_wakeup(30 * 60 * 1000000); // 30 minutes in microseconds
  
  Serial.println("Good night!");
  delay(100); // Allow serial to finish
  
  esp_deep_sleep_start();
}