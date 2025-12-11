#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
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
#define BATTERY_PIN 1 // ADC pin for battery voltage monitoring (GPIO1/A0)

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
uint16_t responseTimeMs = 0; // Response time in ms (received from base station for winner)

// Debug counters for OLED display
int buttonPressCount = 0;
int messagesSent = 0;

// Track state changes to prevent repeating sounds
ButtonState lastState = WAITING;

// Audio settings
bool audioMuted = false;

// Battery monitoring
float batteryVoltage = 0.0;
int batteryPercent = 0;
unsigned long lastBatteryRead = 0;
const unsigned long batteryReadInterval = 30000; // Read battery every 30 seconds

// Interrupt handler for button press
void IRAM_ATTR buttonISR() {
  buttonPressed = true;
}

// ESP-NOW callback for receiving data from base station
// Message format:
//   Byte 0: Status code (0=stopped, 1=winner, 2=locked, 3=ready)
//   Byte 1: Mute flag (0=unmuted, 1=muted)
//   Bytes 2-3: Response time in ms (uint16_t, little-endian)
//   Bytes 4-31: Team name (up to 27 chars + null terminator)
void onDataRecv(const esp_now_recv_info *recv_info, const uint8_t *incomingData, int len) {
  if (len < 1) return;

  uint8_t response = incomingData[0];
  messagesReceived++; // Count messages received

  // Extract mute flag, response time, and team name if message is long enough
  if (len >= 4) {
    audioMuted = (incomingData[1] == 1);

    // Extract response time (uint16_t, little-endian)
    responseTimeMs = incomingData[2] | (incomingData[3] << 8);

    // Team name starts at byte 4
    teamName = String((char*)&incomingData[4]);
    teamName.trim(); // Remove any trailing whitespace
    Serial.printf("Received team name: '%s', muted: %s, responseTime: %dms\n",
                  teamName.c_str(), audioMuted ? "YES" : "NO", responseTimeMs);
  } else if (len > 2) {
    // Backwards compatibility with old message format (just mute + team name)
    audioMuted = (incomingData[1] == 1);
    teamName = String((char*)&incomingData[2]);
    teamName.trim();
    Serial.printf("Received team name (old format): '%s', muted: %s\n", teamName.c_str(), audioMuted ? "YES" : "NO");
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
      // No automatic lockout sound - only play when button pressed while locked
      updateDisplay();
      Serial.println("Locked out");
      break;
      
    case 3: // Game active, ready to buzz
      // Only play ready sound if this is a new state change
      if (currentState != READY) {
        playReadySound(); // Quick ready beep only on state change
      }
      currentState = READY;
      setStatusLED(false);
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

// Draw battery icon in top-right corner
void drawBatteryIcon(int x, int y, int percent) {
  // Battery outline (12x6 pixels)
  display.drawRect(x, y, 10, 6, SSD1306_WHITE); // Main body
  display.drawRect(x + 10, y + 2, 2, 2, SSD1306_WHITE); // Terminal

  // Fill battery based on percentage
  if (percent > 75) {
    display.fillRect(x + 1, y + 1, 8, 4, SSD1306_WHITE); // Full
  } else if (percent > 50) {
    display.fillRect(x + 1, y + 1, 6, 4, SSD1306_WHITE); // 75%
  } else if (percent > 25) {
    display.fillRect(x + 1, y + 1, 4, 4, SSD1306_WHITE); // 50%
  } else if (percent > 10) {
    display.fillRect(x + 1, y + 1, 2, 4, SSD1306_WHITE); // 25%
  }
  // If <= 10%, show empty battery (outline only)

  // Show percentage text next to battery
  display.setCursor(x - 18, y);
  display.setTextSize(1);
  if (percent < 100) {
    display.print(" ");
  }
  display.print(percent);
  display.print("%");
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  // Draw battery indicator in top-right corner
  drawBatteryIcon(110, 0, batteryPercent);

  // Show current state
  display.setCursor(0, 0);
  switch (currentState) {
    case WAITING:
      if (teamName.length() > 0) {
        // Connected - show team name prominently
        display.setCursor(0, 10); // Move down to avoid battery icon
        display.setTextSize(2);
        display.println(teamName.c_str());
        display.setTextSize(1);
        display.println("Ready to play!");
      } else {
        // Not connected - show connection info and debug
        String mac = WiFi.macAddress();
        mac.toUpperCase();
        display.setCursor(0, 8); // Move down to avoid battery icon
        display.printf("MAC:%s\n", mac.c_str());
        display.printf("HB:%s M:%d\n", heartbeatSuccess ? "OK" : "X", messagesReceived);
        display.printf("Btn:%d Sent:%d\n", buttonPressCount, messagesSent);
      }
      break;
      
    case READY:
      // Show team name and ready status
      display.setCursor(0, 8); // Move down to avoid battery icon
      display.setTextSize(1);
      if (teamName.length() > 0) {
        display.println(teamName.c_str());
      }
      display.setTextSize(2);
      display.println("READY!");
      display.setTextSize(1);
      display.println("Press now!");
      break;

    case WINNER:
      // Show team name, winner status, and response time
      display.setCursor(0, 8); // Move down to avoid battery icon
      display.setTextSize(1);
      if (teamName.length() > 0) {
        display.println(teamName.c_str());
      }
      display.setTextSize(2);
      display.println("WINNER!");
      display.setTextSize(1);
      if (responseTimeMs > 0) {
        display.printf("Time: %d ms", responseTimeMs);
      } else {
        display.println("You got it!");
      }
      break;

    case LOCKED_OUT:
      // Show team name and locked status
      display.setCursor(0, 8); // Move down to avoid battery icon
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
  if (!audioMuted && frequency > 0) {
    tone(SPEAKER_PIN, frequency, duration);
  }
  delay(duration);
  if (!audioMuted) {
    noTone(SPEAKER_PIN);
  }
}

void playBuzzSound() {
  // Aggressive buzzer sound - descending notes
  playTone(800, 150);
  playTone(600, 150);
  playTone(400, 200);
}

void playWinnerSound() {
  // Simple victory sound
  playTone(800, 300);
  playTone(1000, 400);
}

void playLockoutSound() {
  // Sad descending tone
  playTone(400, 300);
  playTone(300, 300);
  playTone(200, 500);
}

void playReadySound() {
  // Single ready beep
  playTone(600, 200);
}

void playStartupSound() {
  // Boot-up melody
  playTone(262, 150); // C
  playTone(330, 150); // E
  playTone(392, 150); // G
  playTone(523, 200); // C (high)
}

// Battery monitoring constants
#define BATTERY_SAMPLE_COUNT 10   // Number of ADC samples to average
#define BATTERY_SAMPLE_DELAY 5    // Delay between samples in ms

// Battery monitoring functions
float readBatteryVoltage() {
  // Average multiple ADC readings for more stable results
  long rawSum = 0;
  for (int i = 0; i < BATTERY_SAMPLE_COUNT; i++) {
    rawSum += analogRead(BATTERY_PIN);
    delay(BATTERY_SAMPLE_DELAY);
  }
  int rawValue = rawSum / BATTERY_SAMPLE_COUNT;

  // Convert ADC reading to voltage
  // ESP32-C6 ADC: 12-bit (0-4095), reference voltage ~3.3V
  // With 11dB attenuation, max measurable voltage is ~3.3V
  float adcVoltage = (rawValue / 4095.0) * 3.3;

  // Voltage divider uses two 47kΩ resistors (divides by 2)
  // So actual battery voltage is double the ADC voltage
  float batteryVoltage = adcVoltage * 2.0;

  return batteryVoltage;
}

int calculateBatteryPercent(float voltage) {
  // LiPo battery voltage range:
  // 4.2V = 100% (fully charged)
  // 3.7V = 50% (nominal)
  // 3.0V = 0% (empty, cutoff)

  const float maxVoltage = 4.2;
  const float minVoltage = 3.0;

  // Clamp voltage to valid range
  if (voltage >= maxVoltage) return 100;
  if (voltage <= minVoltage) return 0;

  // Calculate percentage
  float percent = ((voltage - minVoltage) / (maxVoltage - minVoltage)) * 100.0;

  return (int)percent;
}

void updateBatteryLevel() {
  batteryVoltage = readBatteryVoltage();
  batteryPercent = calculateBatteryPercent(batteryVoltage);

  // Debug output
  Serial.printf("Battery: %.2fV (%d%%)\n", batteryVoltage, batteryPercent);
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
  messagesSent++;
  
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

  // Configure battery monitoring pin
  pinMode(BATTERY_PIN, INPUT);
  analogReadResolution(12); // Set ADC resolution to 12 bits (0-4095)
  analogSetAttenuation(ADC_11db); // Set attenuation for full 3.3V range

  // Set device as Wi-Fi station on channel 1 (must match base station)
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

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

  // Register base station as peer (channel 1 to match AP)
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, baseStationMAC, 6);
  peerInfo.channel = 1;  // Must match base station AP channel
  peerInfo.encrypt = false;
  peerInfo.ifidx = WIFI_IF_STA;  // Use STA interface

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

  // Read initial battery level
  updateBatteryLevel();

  Serial.println("Button Ready");
  Serial.println("Battery powered version with voltage monitoring");
  
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
      
      // Count button press for debugging
      buttonPressCount++;
      
      // Only send buzz if in READY state
      if (currentState == READY) {
        Serial.println("Button pressed - sending buzz");
        sendBuzzer(); // Send immediately for lowest latency
        messagesSent++;
        playBuzzSound(); // Play sound after sending (local feedback only)
        
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
        // Brief flash to show locked out and play lockout sound
        blinkStatusLED(1);
        playLockoutSound(); // Play lockout sound only when pressed while locked
        Serial.println("Locked out - button press ignored");
      }
      
      // Update display immediately to show button press registered
      updateDisplay();
    }
    
    // Reset the flag
    buttonPressed = false;
  }
  
  // Send periodic heartbeat to base station
  if (currentTime - lastHeartbeat > heartbeatInterval) {
    sendHeartbeat();
    lastHeartbeat = currentTime;
  }

  // Read battery level periodically
  if (currentTime - lastBatteryRead > batteryReadInterval) {
    updateBatteryLevel();
    lastBatteryRead = currentTime;
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