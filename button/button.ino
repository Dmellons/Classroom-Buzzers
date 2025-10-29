#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "button-secret.h"

// Display configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Pin definitions for ESP32-C6 (Breadboard Version)
#define I2C_SDA 6
#define I2C_SCL 7
#define BUTTON_PIN 15 // Button input pin
#define STATUS_LED_PIN 2 // Built-in LED for status (optional)
#define SPEAKER_PIN 8 // PWM pin for 3W speaker

// Base station MAC address
uint8_t baseStationMAC[] = BASE_STATION_MAC;

bool ledState = false;
unsigned long lastPressTime = 0;
const unsigned long debounceDelay = 50;

// Declare the flag used in the Interrupt Service Routine (ISR)
volatile bool buttonPressed = false; 

// Global debug variables
unsigned long lastHeartbeatTime = 0;
bool heartbeatSuccess = false;
int messagesReceived = 0;

// Game state
enum ButtonState {
  WAITING, // Waiting for game to start
  READY, // Game active, ready to buzz
  WINNER, // This button buzzed first
  LOCKED_OUT // Another button buzzed first
};

ButtonState currentState = WAITING;
String teamName = ""; // Team name received from base station

// Interrupt handler for button press
void IRAM_ATTR buttonISR() {
  buttonPressed = true;
}

// ESP-NOW callback for receiving data from base station
void onDataRecv(const esp_now_recv_info *recv_info, const uint8_t *incomingData, int len) {
  if (len < 1) return;
  
  uint8_t response = incomingData[0];
  messagesReceived++; // Count messages received
  
  // Extract team name if message is long enough
  if (len > 1) {
    teamName = String((char*)&incomingData[1]);
    teamName.trim(); // Remove any trailing whitespace
    Serial.printf("Received team name: '%s'\n", teamName.c_str());
  }
  
  // Debug: Print received message
  Serial.printf("Received ESP-NOW message: %d\n", response);
  
  switch (response) {
    case 0: // Game stopped
      currentState = WAITING;
      setStatusLED(false);
      updateDisplay();
      Serial.println("Game stopped");
      break;
      
    case 1: // Winner!
      currentState = WINNER;
      blinkStatusLED(5); // Blink 5 times to celebrate
      setStatusLED(true); // Keep LED on
      playWinnerSound(); // Victory fanfare
      updateDisplay();
      Serial.println("We won!");
      break;
      
    case 2: // Locked out (another team won)
      currentState = LOCKED_OUT;
      setStatusLED(false);
      playLockoutSound(); // Sad sound
      updateDisplay();
      Serial.println("Locked out");
      break;
      
    case 3: // Game active, ready to buzz
      currentState = READY;
      setStatusLED(false);
      playReadySound(); // Quick ready beep
      updateDisplay();
      Serial.println("Game active - ready!");
      break;
  }
}

// Updated callback signature for ESP32 Arduino Core 3.x
void onDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
  heartbeatSuccess = (status == ESP_NOW_SEND_SUCCESS);
  Serial.print("Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  
  // Show current state
  switch (currentState) {
    case WAITING:
      if (teamName.length() > 0) {
        // Connected - show team name prominently
        display.setTextSize(2);
        display.println(teamName.c_str());
        display.setTextSize(1);
        display.println("");
        display.println("Ready to play!");
      } else {
        // Not connected - show connection info
        String mac = WiFi.macAddress();
        mac.toUpperCase();
        display.printf("MAC:%s\n", mac.c_str());
        display.printf("HB:%s Msg:%d\n", heartbeatSuccess ? "OK" : "FAIL", messagesReceived);
        display.println("Connecting...");
      }
      break;
      
    case READY:
      // Show team name and ready status
      display.setTextSize(1);
      if (teamName.length() > 0) {
        display.println(teamName.c_str());
      }
      display.setTextSize(2);
      display.println("READY!");
      display.setTextSize(1);
      display.println("Press to buzz");
      break;
      
    case WINNER:
      // Show team name and winner status
      display.setTextSize(1);
      if (teamName.length() > 0) {
        display.println(teamName.c_str());
      }
      display.setTextSize(2);
      display.println("WINNER!");
      display.setTextSize(1);
      display.println("You got it!");
      break;
      
    case LOCKED_OUT:
      // Show team name and locked status
      display.setTextSize(1);
      if (teamName.length() > 0) {
        display.println(teamName.c_str());
      }
      display.setTextSize(2);
      display.println("LOCKED");
      display.setTextSize(1);
      display.println("Too late!");
      break;
  }
  
  display.display();
}

void setStatusLED(bool state) {
  ledState = state;
  digitalWrite(STATUS_LED_PIN, state ? HIGH : LOW);
}

void blinkStatusLED(int times) {
  for (int i = 0; i < times; i++) {
    setStatusLED(true);
    delay(100);
    setStatusLED(false);
    delay(100);
  }
}

// Speaker audio feedback functions
void playTone(int frequency, int duration) {
  if (frequency > 0) {
    tone(SPEAKER_PIN, frequency, duration);
  }
  delay(duration);
  noTone(SPEAKER_PIN);
}

void playBuzzSound() {
  // Aggressive buzzer sound - descending notes
  playTone(800, 150);
  playTone(600, 150);
  playTone(400, 200);
}

void playWinnerSound() {
  // Victory fanfare - ascending notes
  playTone(523, 200); // C
  playTone(659, 200); // E  
  playTone(784, 200); // G
  playTone(1047, 400); // C (high)
}

void playLockoutSound() {
  // Sad descending tone
  playTone(400, 300);
  playTone(300, 300);
  playTone(200, 500);
}

void playReadySound() {
  // Quick ascending beep
  playTone(500, 100);
  playTone(700, 100);
}

void playStartupSound() {
  // Boot-up melody
  playTone(262, 150); // C
  playTone(330, 150); // E
  playTone(392, 150); // G
  playTone(523, 200); // C (high)
}

// LED patterns for different states (using built-in LED)
void showStatePattern() {
  switch (currentState) {
    case WAITING:
      // Slow pulse every 3 seconds
      if (millis() % 3000 < 100) {
        setStatusLED(true);
      } else {
        setStatusLED(false);
      }
      break;
      
    case READY:
      // Fast blink to show ready
      if (millis() % 500 < 250) {
        setStatusLED(true);
      } else {
        setStatusLED(false);
      }
      break;
      
    case WINNER:
      // Solid on
      setStatusLED(true);
      break;
      
    case LOCKED_OUT:
      // Off
      setStatusLED(false);
      break;
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
    Serial.println("Heartbeat sent successfully");
  } else {
    Serial.printf("Error sending heartbeat: %d\n", result);
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
  
  // Configure status LED pin
  pinMode(STATUS_LED_PIN, OUTPUT);
  setStatusLED(false);
  
  // Configure speaker pin
  pinMode(SPEAKER_PIN, OUTPUT);
  
  // Set device as Wi-Fi station
  WiFi.mode(WIFI_STA);
  
  // Print MAC address for registration with base station
  Serial.print("Button MAC Address: ");
  String macForSerial = WiFi.macAddress();
  macForSerial.toUpperCase();
  Serial.println(macForSerial);
  
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
  blinkStatusLED(2);
  
  // Play startup sound
  playStartupSound();
  
  Serial.println("Button Ready");
  Serial.println("Breadboard version - USB powered with 3W speaker");
  
  // Print base station MAC we're trying to connect to
  Serial.print("Base Station MAC: ");
  for (int i = 0; i < 6; i++) {
    if (i > 0) Serial.print(":");
    if (baseStationMAC[i] < 16) Serial.print("0");
    Serial.print(baseStationMAC[i], HEX);
  }
  Serial.println();
  
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
        playBuzzSound(); // Immediate audio feedback
        sendBuzzer();
        
        // Brief LED flash to acknowledge press
        setStatusLED(true);
        delay(50);
        setStatusLED(false);
      } else if (currentState == WAITING) {
        // Flash LED to indicate game hasn't started
        blinkStatusLED(1);
        playTone(200, 200); // Low error tone
        Serial.println("Game not active - button press ignored");
      } else if (currentState == LOCKED_OUT) {
        // Brief flash to show locked out
        blinkStatusLED(1);
        playTone(150, 300); // Very low error tone
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
  
  // Update display every 5 seconds (less frequent than battery version)
  if (currentTime - lastDisplayUpdate > 5000) {
    updateDisplay();
    lastDisplayUpdate = currentTime;
  }
  
  // Small delay to prevent watchdog issues
  delay(10);
}