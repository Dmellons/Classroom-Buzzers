#include <WiFi.h>
#include <WebServer.h>
#include <esp_now.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include "base-secret.h"

// Display configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Pin definitions for ESP32-C6
#define I2C_SDA 6
#define I2C_SCL 7
#define SD_MOSI 23
#define SD_MISO 19
#define SD_SCK 18
#define SD_CS 5

// Physical button pins
#define BTN_START_STOP 10  // GPIO10 - Start/Stop game (with internal pull-up)
#define BTN_RESET 1        // GPIO1 - Reset round (with internal pull-up)

// Timing constants
#define HEARTBEAT_TIMEOUT_MS 30000      // Mark button offline after 30 seconds
#define DISPLAY_CYCLE_INTERVAL_MS 2000  // Cycle through teams every 2 seconds
#define DEBOUNCE_DELAY_MS 250           // Button debounce delay

// Message protocol constants
#define MESSAGE_SIZE 32                 // ESP-NOW message size
#define TEAM_NAME_MAX_LENGTH 29         // Max team name in message (+ null terminator)

// Game status codes (sent to buttons)
#define STATUS_GAME_STOPPED 0
#define STATUS_WINNER 1
#define STATUS_LOCKED_OUT 2
#define STATUS_GAME_READY 3

// WiFi credentials (defaults from secret file, can be changed via settings)
String ap_ssid = WIFI_SSID;
String ap_password = WIFI_PASSWORD;
bool wifiSettingsChanged = false;  // Flag to show reboot needed

// Web server
WebServer server(80);

// Game state
bool gameActive = false;
int winnerTeam = -1;
unsigned long gameStartTime = 0;
unsigned long winnerTime = 0;

// Countdown settings (saved to SD card)
bool countdownEnabled = false;        // Off by default
int countdownSeconds = 3;             // Configurable: 1-5 seconds

// Countdown state (runtime only)
bool countdownActive = false;
unsigned long countdownStartTime = 0;
int countdownStep = 0;                // Current step: 3, 2, 1, 0 (BUZZ!)

// Audio settings
bool audioMuted = false;

// Button debounce variables
unsigned long lastBtnStartStopPress = 0;
unsigned long lastBtnResetPress = 0;
bool lastBtnStartStopState = HIGH;
bool lastBtnResetState = HIGH;

// Team configuration
#define MAX_TEAMS 9  // Maximum number of teams (can be 1-9)
struct Team {
  String name;
  uint8_t mac[6];
  bool isConfigured;
  bool isOnline;
  unsigned long lastSeen;
  bool isMuted;
};

Team teams[MAX_TEAMS];

// Pending buttons (auto-discovered but not yet configured)
#define MAX_PENDING 5
struct PendingButton {
  uint8_t mac[6];
  unsigned long lastSeen;
  bool active;
};

PendingButton pendingButtons[MAX_PENDING];

// SD card available flag
bool sdCardAvailable = false;

// HTML escape function to prevent XSS attacks
String escapeHtml(String input) {
  input.replace("&", "&amp;");
  input.replace("<", "&lt;");
  input.replace(">", "&gt;");
  input.replace("\"", "&quot;");
  input.replace("'", "&#39;");
  return input;
}

// Initialize pending buttons array
void initializePendingButtons() {
  for (int i = 0; i < MAX_PENDING; i++) {
    pendingButtons[i].active = false;
    pendingButtons[i].lastSeen = 0;
    memset(pendingButtons[i].mac, 0, 6);
  }
}

// Check if MAC is already a configured team
bool isConfiguredTeam(const uint8_t* mac) {
  for (int i = 0; i < MAX_TEAMS; i++) {
    if (teams[i].isConfigured) {
      bool match = true;
      for (int j = 0; j < 6; j++) {
        if (teams[i].mac[j] != mac[j]) {
          match = false;
          break;
        }
      }
      if (match) return true;
    }
  }
  return false;
}

// Check if MAC is already in pending list
int findPendingButton(const uint8_t* mac) {
  for (int i = 0; i < MAX_PENDING; i++) {
    if (pendingButtons[i].active) {
      bool match = true;
      for (int j = 0; j < 6; j++) {
        if (pendingButtons[i].mac[j] != mac[j]) {
          match = false;
          break;
        }
      }
      if (match) return i;
    }
  }
  return -1;
}

// Add button to pending list (auto-discovery)
void addToPending(const uint8_t* mac) {
  // Already in pending?
  int existing = findPendingButton(mac);
  if (existing >= 0) {
    pendingButtons[existing].lastSeen = millis();
    return;
  }

  // Find empty slot
  for (int i = 0; i < MAX_PENDING; i++) {
    if (!pendingButtons[i].active) {
      memcpy(pendingButtons[i].mac, mac, 6);
      pendingButtons[i].lastSeen = millis();
      pendingButtons[i].active = true;
      Serial.printf("Auto-discovered new button: %02X:%02X:%02X:%02X:%02X:%02X\n",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
      return;
    }
  }

  // No empty slot - replace oldest
  int oldest = 0;
  for (int i = 1; i < MAX_PENDING; i++) {
    if (pendingButtons[i].lastSeen < pendingButtons[oldest].lastSeen) {
      oldest = i;
    }
  }
  memcpy(pendingButtons[oldest].mac, mac, 6);
  pendingButtons[oldest].lastSeen = millis();
  pendingButtons[oldest].active = true;
}

// Format MAC address as string
String formatMac(const uint8_t* mac) {
  char buf[18];
  sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(buf);
}

// Helper function to build and send status message to a specific team
// Message format:
//   Byte 0: Status code (0=stopped, 1=winner, 2=locked, 3=ready)
//   Byte 1: Mute flag (0=unmuted, 1=muted)
//   Bytes 2-3: Response time in ms (uint16_t, little-endian) - only valid for winner
//   Bytes 4-31: Team name (up to 27 chars + null terminator)
void sendStatusMessageToTeam(int teamIndex) {
  if (teamIndex < 0 || teamIndex >= MAX_TEAMS || !teams[teamIndex].isConfigured) {
    return;
  }

  uint8_t message[MESSAGE_SIZE];
  memset(message, 0, sizeof(message));

  // Determine current game status for this team
  if (!gameActive) {
    message[0] = STATUS_GAME_STOPPED;
  } else if (winnerTeam == -1) {
    message[0] = STATUS_GAME_READY;
  } else if (winnerTeam == teamIndex) {
    message[0] = STATUS_WINNER;
  } else {
    message[0] = STATUS_LOCKED_OUT;
  }

  // Add individual mute flag at byte 1 (individual team mute OR global mute)
  message[1] = (audioMuted || teams[teamIndex].isMuted) ? 1 : 0;

  // Add response time at bytes 2-3 (uint16_t, little-endian)
  // Only meaningful for winner, but we send it anyway for simplicity
  uint16_t responseTimeMs = (winnerTime > 65535) ? 65535 : (uint16_t)winnerTime;
  message[2] = responseTimeMs & 0xFF;         // Low byte
  message[3] = (responseTimeMs >> 8) & 0xFF;  // High byte

  // Add team name starting at byte 4 (max 27 chars + null terminator)
  String teamName = teams[teamIndex].name;
  if (teamName.length() > 27) {
    teamName = teamName.substring(0, 27);
  }
  strcpy((char*)&message[4], teamName.c_str());

  // Debug: Print what we're about to send
  Serial.printf("Sending to Team %d (%s): status=%d, muted=%d\n",
    teamIndex + 1, teams[teamIndex].name.c_str(), message[0], message[1]);

  // Add button as peer if not already added
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, teams[teamIndex].mac, 6);
  peerInfo.channel = 1;  // Must match AP channel
  peerInfo.encrypt = false;
  peerInfo.ifidx = WIFI_IF_AP;  // Use AP interface

  if (!esp_now_is_peer_exist(teams[teamIndex].mac)) {
    esp_err_t addResult = esp_now_add_peer(&peerInfo);
    Serial.printf("Adding peer: %s\n", (addResult == ESP_OK) ? "SUCCESS" : "FAILED");
  }

  // Send message
  esp_err_t sendResult = esp_now_send(teams[teamIndex].mac, message, sizeof(message));
  Serial.printf("Send result: %s\n", (sendResult == ESP_OK) ? "OK" : "ERROR");
}

void setup() {
  Serial.begin(115200);
  Serial.println("Base Station Starting...");

  // Initialize team structure with defaults
  initializeTeams();
  initializePendingButtons();

  // Initialize I2C
  Wire.begin(I2C_SDA, I2C_SCL);
  
  // Initialize display
  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Starting...");
    display.display();
    delay(1000);
  }
  
  // Initialize SD card
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  delay(200);
  
  if (SD.begin(SD_CS)) {
    sdCardAvailable = true;
    loadConfig(); // Load team configuration from SD card
    
    // Update display to show config loaded
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Config Loaded!");
    display.display();
    delay(1000);
  } else {
    sdCardAvailable = false;
    
    // Update display to show no SD card
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("No SD Card");
    display.println("Using defaults");
    display.display();
    delay(1000);
  }
  
  // Start WiFi Access Point with ESP-NOW support
  WiFi.mode(WIFI_AP_STA);  // Need STA mode for ESP-NOW
  delay(100);

  // Set WiFi AP on channel 1 for ESP-NOW compatibility
  if (WiFi.softAP(ap_ssid.c_str(), ap_password.c_str(), 1)) {
    // WiFi started successfully
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("WiFi OK");
    display.display();
    delay(500);
  } else {
    // WiFi failed - show error on display
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("WiFi Failed!");
    display.display();
  }
  
  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("ESP-NOW Failed!");
    display.display();
    delay(2000);
  } else {
    // Register callbacks for receiving and sending data
    esp_now_register_recv_cb(onDataRecv);
    esp_now_register_send_cb(onDataSent);
    
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("ESP-NOW Ready");
    display.display();
    delay(500);
  }
  
  // Setup web server
  server.on("/", handleRoot);
  server.on("/settings", handleSettings);
  server.on("/start", handleStart);
  server.on("/reset", handleReset);
  server.on("/stop", handleStop);
  server.on("/save", handleSave);
  server.on("/mute", handleMute);
  server.on("/unmute", handleUnmute);
  server.on("/mute-team", handleMuteTeam);
  server.on("/unmute-team", handleUnmuteTeam);
  server.on("/add-pending", handleAddPending);
  server.on("/remove-team", handleRemoveTeam);
  server.on("/save-settings", handleSaveSettings);
  server.on("/reboot", handleReboot);
  server.on("/api/status", handleApiStatus);
  server.begin();
  
  // Initialize physical buttons with pull-up resistors
  pinMode(BTN_START_STOP, INPUT_PULLUP);
  pinMode(BTN_RESET, INPUT_PULLUP);

  // Read initial button states to prevent false triggers at boot
  delay(50); // Small delay to let pins settle
  lastBtnStartStopState = digitalRead(BTN_START_STOP);
  lastBtnResetState = digitalRead(BTN_RESET);

  // Blink LED to show ready
  pinMode(LED_BUILTIN, OUTPUT);
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(200);
    digitalWrite(LED_BUILTIN, LOW);
    delay(200);
  }

  // Show normal interface after initialization
  updateDisplay();
}

void loop() {
  server.handleClient();

  // Process countdown timer
  if (countdownActive) {
    unsigned long elapsed = millis() - countdownStartTime;
    int newStep = countdownSeconds - (elapsed / 1000);

    if (newStep != countdownStep) {
      countdownStep = newStep;
      Serial.printf("Countdown: %d\n", countdownStep);
      updateDisplay();
    }

    // Countdown finished - start the game
    if (elapsed >= (countdownSeconds * 1000)) {
      Serial.println("Countdown complete - starting game!");
      startGameNow();
    }
  }

  // Handle physical button presses
  handlePhysicalButtons();

  // Check for button timeouts (mark offline if not seen recently)
  unsigned long currentTime = millis();
  for (int i = 0; i < MAX_TEAMS; i++) {
    if (teams[i].isConfigured && teams[i].isOnline) {
      if (currentTime - teams[i].lastSeen > HEARTBEAT_TIMEOUT_MS) {
        teams[i].isOnline = false;
      }
    }
  }

  delay(2);
}

// ESP-NOW callback for send status
void onDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
  Serial.printf("ESP-NOW Send Status: %s\n", (status == ESP_NOW_SEND_SUCCESS) ? "SUCCESS" : "FAIL");
}

// ESP-NOW callback function for receiving button presses
void onDataRecv(const esp_now_recv_info *recv_info, const uint8_t *incomingData, int len) {
  if (len < 1) return;

  // Find which team this MAC address belongs to
  int teamIndex = -1;
  for (int i = 0; i < MAX_TEAMS; i++) {
    if (teams[i].isConfigured) {
      bool macMatch = true;
      for (int j = 0; j < 6; j++) {
        if (teams[i].mac[j] != recv_info->src_addr[j]) {
          macMatch = false;
          break;
        }
      }
      if (macMatch) {
        teamIndex = i;
        teams[i].isOnline = true;
        teams[i].lastSeen = millis();
        break;
      }
    }
  }

  // Unknown button - add to pending list for auto-discovery
  if (teamIndex == -1) {
    addToPending(recv_info->src_addr);
    return;
  }

  uint8_t messageType = incomingData[0];

  if (messageType == 1) { // Button press
    handleButtonPress(teamIndex);
  } else if (messageType == 2) { // Heartbeat/status
    // Send current status and team name back to this specific button
    sendTeamStatusUpdate(teamIndex);
  }
}

void handleButtonPress(int teamIndex) {
  if (!gameActive) return; // Game not active
  if (winnerTeam >= 0) return; // Already have a winner
  
  // This team wins!
  winnerTeam = teamIndex;
  winnerTime = millis() - gameStartTime;
  
  // Log to SD card
  logBuzzerEvent(teamIndex, winnerTime);
  
  // Send responses to all configured buttons
  sendButtonResponses();
  
  // Update display
  updateDisplay();
}

// Display cycling variables
unsigned long lastDisplayUpdate = 0;
int currentDisplayTeam = 0;

// Helper: Get team status character for scoreboard
char getTeamStatusChar(int idx) {
  if (!teams[idx].isConfigured) return '-';
  if (winnerTeam == idx) return '*';  // Star for winner
  if (teams[idx].isOnline) return 'O';  // Filled circle (will be ●)
  return 'o';  // Empty circle (will be ○)
}

// Helper: Count configured teams
int countConfiguredTeams() {
  int count = 0;
  for (int i = 0; i < MAX_TEAMS; i++) {
    if (teams[i].isConfigured) count++;
  }
  return count;
}

// Helper: Count online teams
int countOnlineTeams() {
  int count = 0;
  for (int i = 0; i < MAX_TEAMS; i++) {
    if (teams[i].isConfigured && teams[i].isOnline) count++;
  }
  return count;
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  // --- COUNTDOWN MODE ---
  if (countdownActive) {
    display.setTextSize(4);  // Large countdown number
    String text;
    if (countdownStep > 0) {
      text = String(countdownStep);
    } else {
      text = "GO!";
    }
    // Center the text
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(text.c_str(), 0, 0, &x1, &y1, &w, &h);
    display.setCursor((SCREEN_WIDTH - w) / 2, (SCREEN_HEIGHT - h) / 2);
    display.print(text);
    display.display();
    return;
  }

  // --- GAME ACTIVE MODE ---
  if (gameActive) {
    if (winnerTeam >= 0) {
      // Enhanced winner display
      display.setCursor(0, 0);
      display.setTextSize(1);
      display.println("    *** WINNER ***");
      display.println();

      // Team name large and centered
      display.setTextSize(2);
      String name = teams[winnerTeam].name;
      if (name.length() > 10) name = name.substring(0, 10);  // Truncate if needed
      int16_t x1, y1;
      uint16_t w, h;
      display.getTextBounds(name.c_str(), 0, 0, &x1, &y1, &w, &h);
      display.setCursor((SCREEN_WIDTH - w) / 2, 20);
      display.println(name);

      // Response time
      display.setTextSize(1);
      display.setCursor(0, 44);
      display.printf("    Response: %lu ms", winnerTime);

      // Reset instruction
      display.setCursor(0, 56);
      display.print(" Press RESET for next Q");

    } else {
      // Live scoreboard - show all teams with status
      display.setTextSize(1);
      display.printf("GAME ON     Teams:%d/%d", countOnlineTeams(), countConfiguredTeams());
      display.setCursor(0, 10);

      // Draw 2-column grid of configured teams
      int col = 0;
      int row = 0;
      for (int i = 0; i < MAX_TEAMS; i++) {
        if (teams[i].isConfigured) {
          int x = col * 64;
          int y = 10 + (row * 9);

          display.setCursor(x, y);
          // Team number and truncated name
          String shortName = teams[i].name.substring(0, 6);
          display.printf("%d.%s", i + 1, shortName.c_str());

          // Status indicator
          display.setCursor(x + 56, y);
          display.print(getTeamStatusChar(i));

          col++;
          if (col >= 2) {
            col = 0;
            row++;
          }
        }
      }

      // Status bar at bottom
      display.setCursor(0, 56);
      display.print(">>> WAITING FOR BUZZ <<<");
    }

  // --- GAME STOPPED MODE ---
  } else {
    int connectedCount = countOnlineTeams();

    // WiFi info header
    display.printf("SSID: %s\n", ap_ssid.c_str());
    display.printf("IP: %s\n", WiFi.softAPIP().toString().c_str());
    display.printf("Teams: %d/%d\n", connectedCount, MAX_TEAMS);
    display.println();

    if (connectedCount > 0) {
      // Cycle through connected teams
      unsigned long currentTime = millis();
      if (currentTime - lastDisplayUpdate >= DISPLAY_CYCLE_INTERVAL_MS) {
        lastDisplayUpdate = currentTime;

        // Find next connected team
        int startTeam = currentDisplayTeam;
        do {
          currentDisplayTeam = (currentDisplayTeam + 1) % MAX_TEAMS;
        } while (!teams[currentDisplayTeam].isConfigured || !teams[currentDisplayTeam].isOnline);

        // Prevent infinite loop if no teams connected
        if (currentDisplayTeam == startTeam && (!teams[currentDisplayTeam].isConfigured || !teams[currentDisplayTeam].isOnline)) {
          currentDisplayTeam = 0;
        }
      }

      // Display current team info
      if (teams[currentDisplayTeam].isConfigured && teams[currentDisplayTeam].isOnline) {
        display.setTextSize(2);
        display.printf("Team %d\n", currentDisplayTeam + 1);
        display.println(teams[currentDisplayTeam].name);
        display.setTextSize(1);
        if (teams[currentDisplayTeam].isMuted) {
          display.println("[MUTED]");
        }
      }
    } else {
      display.println("No teams connected");
      display.println("Configure teams via");
      display.println("web interface");
    }
  }

  display.display();
}

// CSS styles shared between pages
String getStyles() {
  String css = "<style>";
  css += "*{box-sizing:border-box;margin:0;padding:0;}";
  css += "body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:#1a1a2e;color:#eee;min-height:100vh;}";
  css += ".container{max-width:600px;margin:0 auto;padding:20px;}";
  css += "h1{text-align:center;font-size:1.8em;margin-bottom:10px;color:#fff;}";
  css += ".subtitle{text-align:center;color:#888;margin-bottom:20px;font-size:0.9em;}";
  css += ".card{background:#16213e;border-radius:16px;padding:20px;margin-bottom:16px;box-shadow:0 4px 6px rgba(0,0,0,0.3);}";
  css += ".btn{display:block;width:100%;padding:20px;font-size:1.3em;font-weight:bold;border:none;border-radius:12px;cursor:pointer;margin:10px 0;transition:transform 0.1s,box-shadow 0.1s;}";
  css += ".btn:active{transform:scale(0.98);}";
  css += ".btn-start{background:linear-gradient(135deg,#00b894,#00cec9);color:#fff;}";
  css += ".btn-stop{background:linear-gradient(135deg,#e74c3c,#c0392b);color:#fff;}";
  css += ".btn-reset{background:linear-gradient(135deg,#f39c12,#e67e22);color:#fff;}";
  css += ".btn-settings{background:#2d3748;color:#a0aec0;font-size:1em;padding:15px;}";
  css += ".btn-add{background:linear-gradient(135deg,#667eea,#764ba2);color:#fff;font-size:1em;padding:12px;}";
  css += ".btn-mute{background:#4a5568;color:#fff;font-size:0.9em;padding:10px 15px;display:inline-block;width:auto;}";
  css += ".status-bar{display:flex;justify-content:space-around;margin-bottom:20px;}";
  css += ".status-item{text-align:center;}";
  css += ".status-num{font-size:2em;font-weight:bold;color:#00b894;}";
  css += ".status-label{font-size:0.8em;color:#888;}";
  css += ".winner-display{background:linear-gradient(135deg,#00b894,#00cec9);border-radius:16px;padding:30px;text-align:center;margin-bottom:16px;}";
  css += ".winner-title{font-size:1.2em;opacity:0.9;}";
  css += ".winner-name{font-size:2.5em;font-weight:bold;margin:10px 0;}";
  css += ".winner-time{font-size:1.1em;opacity:0.9;}";
  css += ".ready-display{background:linear-gradient(135deg,#667eea,#764ba2);border-radius:16px;padding:40px;text-align:center;margin-bottom:16px;}";
  css += ".ready-text{font-size:2em;font-weight:bold;}";
  css += ".ready-sub{font-size:1em;opacity:0.8;margin-top:10px;}";
  css += ".stopped-display{background:#2d3748;border-radius:16px;padding:30px;text-align:center;margin-bottom:16px;}";
  css += ".stopped-text{font-size:1.5em;color:#888;}";
  css += ".team-list{list-style:none;}";
  css += ".team-item{display:flex;align-items:center;padding:12px;border-bottom:1px solid #2d3748;}";
  css += ".team-item:last-child{border-bottom:none;}";
  css += ".team-status{width:12px;height:12px;border-radius:50%;margin-right:12px;}";
  css += ".team-status.online{background:#00b894;}";
  css += ".team-status.offline{background:#e67e22;}";
  css += ".team-status.none{background:#636e72;}";
  css += ".team-name{flex:1;font-size:1.1em;}";
  css += ".team-mute{color:#888;font-size:0.8em;}";
  css += ".pending-item{background:#2d3748;border-radius:12px;padding:15px;margin:10px 0;display:flex;align-items:center;justify-content:space-between;}";
  css += ".pending-badge{background:#e74c3c;color:#fff;padding:4px 12px;border-radius:20px;font-size:0.8em;font-weight:bold;}";
  css += ".input{width:100%;padding:12px;font-size:1em;border:2px solid #2d3748;border-radius:8px;background:#1a1a2e;color:#fff;margin:8px 0;}";
  css += ".input:focus{border-color:#667eea;outline:none;}";
  css += ".nav{display:flex;gap:10px;margin-bottom:20px;}";
  css += ".nav a{flex:1;text-align:center;padding:12px;background:#2d3748;color:#a0aec0;text-decoration:none;border-radius:8px;}";
  css += ".nav a.active{background:#667eea;color:#fff;}";
  css += ".mute-toggle{display:flex;align-items:center;justify-content:space-between;padding:15px;background:#2d3748;border-radius:12px;margin:10px 0;}";
  css += ".form-group{margin:15px 0;}";
  css += ".form-label{display:block;color:#888;margin-bottom:5px;font-size:0.9em;}";
  css += ".remove-btn{color:#e74c3c;background:none;border:none;cursor:pointer;padding:5px 10px;}";
  css += "</style>";
  return css;
}

// Main game control page (teacher-friendly)
void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<title>Quiz Buzzer</title>";
  html += "<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<meta http-equiv='refresh' content='3'>";  // Auto-refresh every 3 seconds
  html += getStyles();
  html += "</head><body><div class='container'>";

  html += "<h1>Quiz Buzzer</h1>";

  // Count teams
  int onlineCount = 0;
  int configuredCount = 0;
  for (int i = 0; i < MAX_TEAMS; i++) {
    if (teams[i].isConfigured) {
      configuredCount++;
      if (teams[i].isOnline) onlineCount++;
    }
  }

  // Count pending buttons
  int pendingCount = 0;
  for (int i = 0; i < MAX_PENDING; i++) {
    if (pendingButtons[i].active) pendingCount++;
  }

  html += "<p class='subtitle'>" + String(onlineCount) + " of " + String(configuredCount) + " teams connected</p>";

  // Game Status Display
  if (gameActive) {
    if (winnerTeam >= 0) {
      // Winner!
      html += "<div class='winner-display'>";
      html += "<div class='winner-title'>WINNER!</div>";
      html += "<div class='winner-name'>" + escapeHtml(teams[winnerTeam].name) + "</div>";
      html += "<div class='winner-time'>" + String(winnerTime) + " ms</div>";
      html += "</div>";
      html += "<button class='btn btn-reset' onclick=\"location.href='/reset'\">NEXT QUESTION</button>";
      html += "<button class='btn btn-stop' onclick=\"location.href='/stop'\">END GAME</button>";
    } else {
      // Waiting for buzz
      html += "<div class='ready-display'>";
      html += "<div class='ready-text'>READY!</div>";
      html += "<div class='ready-sub'>Waiting for buzz...</div>";
      html += "</div>";
      html += "<button class='btn btn-reset' onclick=\"location.href='/reset'\">RESET</button>";
      html += "<button class='btn btn-stop' onclick=\"location.href='/stop'\">STOP GAME</button>";
    }
  } else {
    // Game stopped
    html += "<div class='stopped-display'>";
    html += "<div class='stopped-text'>Game Stopped</div>";
    html += "</div>";
    html += "<button class='btn btn-start' onclick=\"location.href='/start'\">START GAME</button>";
  }

  // Teams list (simplified)
  html += "<div class='card'>";
  html += "<ul class='team-list'>";
  for (int i = 0; i < MAX_TEAMS; i++) {
    if (teams[i].isConfigured) {
      html += "<li class='team-item'>";
      html += "<span class='team-status " + String(teams[i].isOnline ? "online" : "offline") + "'></span>";
      html += "<span class='team-name'>" + escapeHtml(teams[i].name) + "</span>";
      if (teams[i].isMuted || audioMuted) {
        html += "<span class='team-mute'>muted</span>";
      }
      html += "</li>";
    }
  }
  if (configuredCount == 0) {
    html += "<li class='team-item'><span class='team-name' style='color:#888;'>No teams configured yet</span></li>";
  }
  html += "</ul></div>";

  // Pending buttons alert
  if (pendingCount > 0) {
    html += "<div class='card' style='border:2px solid #e74c3c;'>";
    html += "<div style='display:flex;align-items:center;margin-bottom:10px;'>";
    html += "<span class='pending-badge'>" + String(pendingCount) + " NEW</span>";
    html += "<span style='margin-left:10px;'>New buttons detected!</span>";
    html += "</div>";
    html += "<button class='btn btn-add' onclick=\"location.href='/settings'\">Add Buttons</button>";
    html += "</div>";
  }

  // Sound toggle
  html += "<div class='mute-toggle'>";
  html += "<span>Sound Effects</span>";
  if (audioMuted) {
    html += "<button class='btn-mute' onclick=\"location.href='/unmute'\">UNMUTE</button>";
  } else {
    html += "<button class='btn-mute' onclick=\"location.href='/mute'\">MUTE</button>";
  }
  html += "</div>";

  // Settings link
  html += "<button class='btn btn-settings' onclick=\"location.href='/settings'\">Settings</button>";

  html += "</div></body></html>";
  server.send(200, "text/html", html);
}

// Settings page (for configuration)
void handleSettings() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<title>Settings - Quiz Buzzer</title>";
  html += "<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += getStyles();
  html += "</head><body><div class='container'>";

  html += "<h1>Settings</h1>";
  html += "<div class='nav'>";
  html += "<a href='/'>Game</a>";
  html += "<a href='/settings' class='active'>Settings</a>";
  html += "</div>";

  // Count pending buttons
  int pendingCount = 0;
  for (int i = 0; i < MAX_PENDING; i++) {
    if (pendingButtons[i].active) pendingCount++;
  }

  // Pending buttons section (auto-discovery)
  if (pendingCount > 0) {
    html += "<div class='card' style='border:2px solid #667eea;'>";
    html += "<h3 style='margin-bottom:15px;'>New Buttons Detected</h3>";
    html += "<p style='color:#888;margin-bottom:15px;font-size:0.9em;'>These buttons are powered on but not configured yet. Give them a name to add them.</p>";

    for (int i = 0; i < MAX_PENDING; i++) {
      if (pendingButtons[i].active) {
        html += "<div class='pending-item'>";
        html += "<form method='GET' action='/add-pending' style='display:flex;align-items:center;width:100%;gap:10px;'>";
        html += "<input type='hidden' name='mac' value='" + formatMac(pendingButtons[i].mac) + "'>";
        html += "<input type='text' name='name' placeholder='Team name' class='input' style='flex:1;margin:0;' required>";
        html += "<button type='submit' class='btn btn-add' style='width:auto;margin:0;'>Add</button>";
        html += "</form>";
        html += "</div>";
      }
    }
    html += "</div>";
  }

  // Configured teams
  html += "<div class='card'>";
  html += "<h3 style='margin-bottom:15px;'>Configured Teams</h3>";

  int configuredCount = 0;
  for (int i = 0; i < MAX_TEAMS; i++) {
    if (teams[i].isConfigured) {
      configuredCount++;
      html += "<div style='display:flex;align-items:center;padding:12px 0;border-bottom:1px solid #2d3748;'>";
      html += "<span class='team-status " + String(teams[i].isOnline ? "online" : "offline") + "'></span>";
      html += "<span style='flex:1;'>" + escapeHtml(teams[i].name) + "</span>";

      // Mute toggle
      if (teams[i].isMuted) {
        html += "<a href='/unmute-team?id=" + String(i) + "' style='color:#888;margin-right:10px;'>unmute</a>";
      } else {
        html += "<a href='/mute-team?id=" + String(i) + "' style='color:#888;margin-right:10px;'>mute</a>";
      }

      html += "<a href='/remove-team?id=" + String(i) + "' class='remove-btn'>Remove</a>";
      html += "</div>";
    }
  }

  if (configuredCount == 0) {
    html += "<p style='color:#888;text-align:center;padding:20px;'>No teams configured yet. Power on a button to detect it automatically!</p>";
  }

  html += "</div>";

  // Manual add section (collapsed by default)
  html += "<div class='card'>";
  html += "<details>";
  html += "<summary style='cursor:pointer;color:#888;'>Manual Configuration (Advanced)</summary>";
  html += "<div style='margin-top:15px;'>";
  html += "<p style='color:#666;font-size:0.85em;margin-bottom:15px;'>Only use this if auto-discovery isn't working. You'll need the MAC address from the button's display.</p>";
  html += "<form method='GET' action='/save'>";

  for (int i = 0; i < MAX_TEAMS; i++) {
    html += "<div class='form-group'>";
    html += "<label class='form-label'>Team " + String(i + 1) + "</label>";
    html += "<input type='text' name='team" + String(i) + "' value='" + escapeHtml(teams[i].name) + "' placeholder='Team name' class='input'>";
    html += "<input type='text' name='mac" + String(i) + "' value='" + formatMac(teams[i].mac) + "' placeholder='AA:BB:CC:DD:EE:FF' pattern='([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}' class='input'>";
    html += "</div>";
  }

  html += "<button type='submit' class='btn btn-settings'>Save All</button>";
  html += "</form>";
  html += "</div></details></div>";

  // Countdown settings section
  html += "<div class='card'>";
  html += "<h3 style='margin-bottom:15px;'>Game Settings</h3>";
  html += "<form method='GET' action='/save-settings'>";

  // Countdown toggle
  html += "<div style='display:flex;align-items:center;margin-bottom:15px;'>";
  html += "<input type='checkbox' name='countdown' id='countdown' value='1'";
  if (countdownEnabled) html += " checked";
  html += " style='width:20px;height:20px;margin-right:10px;'>";
  html += "<label for='countdown' style='color:#ccc;'>Enable countdown before game starts</label>";
  html += "</div>";

  // Countdown duration
  html += "<div style='display:flex;align-items:center;margin-bottom:15px;'>";
  html += "<label style='color:#888;margin-right:10px;'>Countdown duration:</label>";
  html += "<select name='countdown_secs' style='background:#2d3748;color:#fff;border:1px solid #4a5568;padding:8px;border-radius:6px;'>";
  for (int s = 1; s <= 5; s++) {
    html += "<option value='" + String(s) + "'";
    if (countdownSeconds == s) html += " selected";
    html += ">" + String(s) + " second" + (s > 1 ? "s" : "") + "</option>";
  }
  html += "</select>";
  html += "</div>";

  // WiFi Settings section
  html += "<h3 style='margin:20px 0 15px 0;border-top:1px solid #4a5568;padding-top:20px;'>WiFi Settings</h3>";

  // Show reboot warning if settings changed
  if (wifiSettingsChanged) {
    html += "<div style='background:#744210;padding:12px;border-radius:6px;margin-bottom:15px;'>";
    html += "<strong>Reboot required!</strong> WiFi settings have changed. ";
    html += "<a href='/reboot' style='color:#fbd38d;'>Click here to reboot</a>";
    html += "</div>";
  }

  // SSID input
  html += "<div style='margin-bottom:15px;'>";
  html += "<label style='color:#888;display:block;margin-bottom:5px;'>Network Name (SSID):</label>";
  html += "<input type='text' name='wifi_ssid' value='" + escapeHtml(ap_ssid) + "' ";
  html += "maxlength='32' style='background:#2d3748;color:#fff;border:1px solid #4a5568;padding:10px;border-radius:6px;width:100%;' required>";
  html += "</div>";

  // Password input
  html += "<div style='margin-bottom:15px;'>";
  html += "<label style='color:#888;display:block;margin-bottom:5px;'>Password (min 8 characters):</label>";
  html += "<input type='text' name='wifi_pass' value='" + escapeHtml(ap_password) + "' ";
  html += "minlength='8' maxlength='63' style='background:#2d3748;color:#fff;border:1px solid #4a5568;padding:10px;border-radius:6px;width:100%;' required>";
  html += "<p style='color:#666;font-size:0.8em;margin-top:5px;'>Changes take effect after reboot</p>";
  html += "</div>";

  html += "<button type='submit' class='btn btn-settings'>Save Settings</button>";
  html += "</form>";
  html += "</div>";

  // System info
  html += "<div class='card'>";
  html += "<h3 style='margin-bottom:15px;'>System Info</h3>";
  html += "<p style='color:#888;font-size:0.9em;'>WiFi: " + String(ap_ssid) + "</p>";
  html += "<p style='color:#888;font-size:0.9em;'>IP: " + WiFi.softAPIP().toString() + "</p>";
  html += "<p style='color:#888;font-size:0.9em;'>Base MAC: <code style='background:#2d3748;padding:2px 6px;border-radius:3px;'>" + WiFi.macAddress() + "</code></p>";
  html += "<p style='color:#888;font-size:0.9em;'>SD Card: " + String(sdCardAvailable ? "Connected" : "Not available") + "</p>";
  html += "</div>";

  html += "<button class='btn btn-settings' onclick=\"location.href='/'\">Back to Game</button>";

  html += "</div></body></html>";
  server.send(200, "text/html", html);
}

// Add a pending button as a configured team
void handleAddPending() {
  String mac = server.arg("mac");
  String name = server.arg("name");

  if (mac.length() == 0 || name.length() == 0) {
    server.sendHeader("Location", "/settings");
    server.send(302);
    return;
  }

  // Parse MAC address
  uint8_t macBytes[6];
  int values[6];
  if (sscanf(mac.c_str(), "%x:%x:%x:%x:%x:%x",
      &values[0], &values[1], &values[2], &values[3], &values[4], &values[5]) == 6) {
    for (int i = 0; i < 6; i++) macBytes[i] = (uint8_t)values[i];
  } else {
    server.sendHeader("Location", "/settings");
    server.send(302);
    return;
  }

  // Find empty team slot
  int slot = -1;
  for (int i = 0; i < MAX_TEAMS; i++) {
    if (!teams[i].isConfigured) {
      slot = i;
      break;
    }
  }

  if (slot == -1) {
    // No empty slots - find slot with matching MAC or oldest offline
    for (int i = 0; i < MAX_TEAMS; i++) {
      bool match = true;
      for (int j = 0; j < 6; j++) {
        if (teams[i].mac[j] != macBytes[j]) { match = false; break; }
      }
      if (match) { slot = i; break; }
    }
  }

  if (slot >= 0) {
    teams[slot].name = name;
    memcpy(teams[slot].mac, macBytes, 6);
    teams[slot].isConfigured = true;
    teams[slot].isOnline = false;
    teams[slot].isMuted = false;

    // Remove from pending list
    int pendingIdx = findPendingButton(macBytes);
    if (pendingIdx >= 0) {
      pendingButtons[pendingIdx].active = false;
    }

    saveConfig();
    Serial.printf("Added team '%s' with MAC %s\n", name.c_str(), mac.c_str());
  }

  server.sendHeader("Location", "/settings");
  server.send(302);
}

// Remove a configured team
void handleRemoveTeam() {
  if (server.hasArg("id")) {
    int id = server.arg("id").toInt();
    if (id >= 0 && id < MAX_TEAMS && teams[id].isConfigured) {
      Serial.printf("Removing team '%s'\n", teams[id].name.c_str());
      teams[id].name = "Team " + String(id + 1);
      memset(teams[id].mac, 0, 6);
      teams[id].isConfigured = false;
      teams[id].isOnline = false;
      teams[id].isMuted = false;
      saveConfig();
    }
  }
  server.sendHeader("Location", "/settings");
  server.send(302);
}

// Save game settings (countdown, etc.)
void handleSaveSettings() {
  // Countdown setting - checkbox only present when checked
  countdownEnabled = server.hasArg("countdown");

  // Countdown duration
  if (server.hasArg("countdown_secs")) {
    int secs = server.arg("countdown_secs").toInt();
    if (secs >= 1 && secs <= 5) {
      countdownSeconds = secs;
    }
  }

  // WiFi SSID
  if (server.hasArg("wifi_ssid")) {
    String newSsid = server.arg("wifi_ssid");
    newSsid.trim();
    if (newSsid.length() > 0 && newSsid.length() <= 32) {
      if (newSsid != ap_ssid) {
        ap_ssid = newSsid;
        wifiSettingsChanged = true;
      }
    }
  }

  // WiFi Password
  if (server.hasArg("wifi_pass")) {
    String newPass = server.arg("wifi_pass");
    newPass.trim();
    if (newPass.length() >= 8 && newPass.length() <= 63) {
      if (newPass != ap_password) {
        ap_password = newPass;
        wifiSettingsChanged = true;
      }
    }
  }

  Serial.printf("Settings saved: countdown=%s, duration=%d, SSID=%s\n",
                countdownEnabled ? "enabled" : "disabled", countdownSeconds, ap_ssid.c_str());

  saveConfig();
  server.sendHeader("Location", "/settings");
  server.send(302, "text/plain", "Settings saved!");
}

// Reboot the device
void handleReboot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Rebooting...</title>";
  html += "<style>body{background:#1a1a2e;color:#fff;font-family:sans-serif;display:flex;justify-content:center;align-items:center;height:100vh;margin:0;}";
  html += ".msg{text-align:center;}</style></head><body>";
  html += "<div class='msg'><h1>Rebooting...</h1>";
  html += "<p>Please reconnect to the WiFi network:</p>";
  html += "<p style='font-size:1.5em;color:#667eea;'>" + escapeHtml(ap_ssid) + "</p>";
  html += "<p>Then navigate to <strong>192.168.4.1</strong></p>";
  html += "</div></body></html>";
  server.send(200, "text/html", html);

  delay(1000);  // Give time for response to send
  ESP.restart();
}

// JSON API for status (for potential future AJAX updates)
void handleApiStatus() {
  String json = "{";
  json += "\"gameActive\":" + String(gameActive ? "true" : "false") + ",";
  json += "\"winnerTeam\":" + String(winnerTeam) + ",";
  json += "\"winnerTime\":" + String(winnerTime) + ",";
  if (winnerTeam >= 0) {
    json += "\"winnerName\":\"" + escapeHtml(teams[winnerTeam].name) + "\",";
  }
  json += "\"audioMuted\":" + String(audioMuted ? "true" : "false") + ",";

  // Teams
  json += "\"teams\":[";
  for (int i = 0; i < MAX_TEAMS; i++) {
    if (i > 0) json += ",";
    json += "{";
    json += "\"name\":\"" + escapeHtml(teams[i].name) + "\",";
    json += "\"configured\":" + String(teams[i].isConfigured ? "true" : "false") + ",";
    json += "\"online\":" + String(teams[i].isOnline ? "true" : "false") + ",";
    json += "\"muted\":" + String(teams[i].isMuted ? "true" : "false");
    json += "}";
  }
  json += "],";

  // Pending count
  int pendingCount = 0;
  for (int i = 0; i < MAX_PENDING; i++) {
    if (pendingButtons[i].active) pendingCount++;
  }
  json += "\"pendingCount\":" + String(pendingCount);
  json += "}";

  server.send(200, "application/json", json);
}

void handleStart() {
  Serial.println("=== STARTING GAME ===");

  if (countdownEnabled && countdownSeconds > 0) {
    // Start countdown instead of immediate game start
    Serial.printf("Starting countdown: %d seconds\n", countdownSeconds);
    countdownActive = true;
    countdownStartTime = millis();
    countdownStep = countdownSeconds;
    updateDisplay();
  } else {
    // Immediate game start (no countdown)
    startGameNow();
  }

  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "Game Started!");
}

// Helper function to actually start the game (called after countdown or immediately)
void startGameNow() {
  gameActive = true;
  winnerTeam = -1;
  gameStartTime = millis();
  winnerTime = 0;
  countdownActive = false;

  // Send start signal to all configured buttons
  Serial.println("Sending game start messages to all configured teams...");
  sendButtonResponses();

  updateDisplay();
}

void handleReset() {
  winnerTeam = -1;
  gameStartTime = millis();
  winnerTime = 0;
  
  // Send reset signal to all configured buttons
  if (gameActive) {
    sendButtonResponses();
  }
  
  updateDisplay();
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "Round Reset!");
}

void handleStop() {
  gameActive = false;
  countdownActive = false;  // Cancel any active countdown
  winnerTeam = -1;
  winnerTime = 0;

  // Send stop signal to all configured buttons
  sendButtonResponses();

  updateDisplay();
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "Game Stopped!");
}

void handleMute() {
  audioMuted = true;
  
  // Send updated status to all buttons to apply mute setting
  sendButtonResponses();
  
  updateDisplay();
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "All Buttons Muted!");
}

void handleUnmute() {
  audioMuted = false;
  
  // Send updated status to all buttons to apply unmute setting
  sendButtonResponses();
  
  updateDisplay();
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "All Buttons Unmuted!");
}

void handleMuteTeam() {
  if (server.hasArg("id")) {
    int teamId = server.arg("id").toInt();
    if (teamId >= 0 && teamId < MAX_TEAMS && teams[teamId].isConfigured) {
      teams[teamId].isMuted = true;

      // Send updated status to this specific team
      sendTeamStatusUpdate(teamId);

      // Save configuration to persist mute state
      saveConfig();

      updateDisplay();
      server.sendHeader("Location", "/settings");
      server.send(302);
      return;
    }
  }

  server.sendHeader("Location", "/settings");
  server.send(302);
}

void handleUnmuteTeam() {
  if (server.hasArg("id")) {
    int teamId = server.arg("id").toInt();
    if (teamId >= 0 && teamId < MAX_TEAMS && teams[teamId].isConfigured) {
      teams[teamId].isMuted = false;

      // Send updated status to this specific team
      sendTeamStatusUpdate(teamId);

      // Save configuration to persist mute state
      saveConfig();

      updateDisplay();
      server.sendHeader("Location", "/settings");
      server.send(302);
      return;
    }
  }

  server.sendHeader("Location", "/settings");
  server.send(302);
}

void handleSave() {
  // Update team names and MAC addresses from form data
  for (int i = 0; i < MAX_TEAMS; i++) {
    // Update team name
    String teamParam = "team" + String(i);
    if (server.hasArg(teamParam)) {
      String newName = server.arg(teamParam);
      newName.trim();
      if (newName.length() > 0 && newName.length() <= 20) {
        teams[i].name = newName;
      }
    }
    
    // Update MAC address
    String macParam = "mac" + String(i);
    if (server.hasArg(macParam)) {
      String macStr = server.arg(macParam);
      macStr.trim();
      macStr.toUpperCase(); // Capitalize all letters
      if (macStr.length() > 0) {
        parseMacAddress(macStr, teams[i].mac);
        teams[i].isConfigured = true;
      }
    }
  }
  
  // Save to SD card if available
  if (sdCardAvailable) {
    saveConfig();
  }

  // Update display with new team names
  updateDisplay();

  // Redirect back to settings page
  server.sendHeader("Location", "/settings");
  server.send(302);
}

void saveConfig() {
  if (!sdCardAvailable) return;

  Serial.println("=== Saving Configuration to SD Card ===");

  // Delete old config file to ensure clean write (not append)
  if (SD.exists("/config.txt")) {
    Serial.println("Deleting old config.txt...");
    SD.remove("/config.txt");
  }

  // Open file in write mode
  File configFile = SD.open("/config.txt", FILE_WRITE);
  if (!configFile) {
    Serial.println("ERROR: Failed to open config.txt for writing!");
    return;
  }

  Serial.println("Writing configuration data...");
  configFile.println(MAX_TEAMS);
  for (int i = 0; i < MAX_TEAMS; i++) {
    configFile.println(teams[i].name);
    // Save MAC address as hex values separated by commas
    for (int j = 0; j < 6; j++) {
      configFile.print(teams[i].mac[j], HEX);
      if (j < 5) configFile.print(",");
    }
    configFile.println();
    configFile.println(teams[i].isConfigured ? "1" : "0");
    configFile.println(teams[i].isMuted ? "1" : "0");

    Serial.printf("  Team %d: %s (%s)\n", i + 1, teams[i].name.c_str(),
                  teams[i].isConfigured ? "configured" : "not configured");
  }

  // Save countdown settings
  configFile.println(countdownEnabled ? "1" : "0");
  configFile.println(countdownSeconds);
  Serial.printf("  Countdown: %s, %d seconds\n",
                countdownEnabled ? "enabled" : "disabled", countdownSeconds);

  // Save WiFi settings
  configFile.println(ap_ssid);
  configFile.println(ap_password);
  Serial.printf("  WiFi SSID: %s\n", ap_ssid.c_str());

  // CRITICAL: Flush data to SD card before closing
  configFile.flush();
  Serial.println("Flushed data to SD card");

  configFile.close();
  Serial.println("=== Configuration Saved Successfully ===");
}

void loadConfig() {
  if (!sdCardAvailable) {
    Serial.println("SD card not available, cannot load config");
    return;
  }

  Serial.println("=== Loading Configuration from SD Card ===");

  if (!SD.exists("/config.txt")) {
    Serial.println("config.txt does not exist - using defaults");
    return;
  }

  File configFile = SD.open("/config.txt", FILE_READ);
  if (!configFile) {
    Serial.println("ERROR: Failed to open config.txt for reading!");
    return;
  }

  int numTeams = configFile.readStringUntil('\n').toInt();
  Serial.printf("Config file has %d teams, MAX_TEAMS = %d\n", numTeams, MAX_TEAMS);

  if (numTeams == MAX_TEAMS) {
    for (int i = 0; i < MAX_TEAMS; i++) {
      // Load team name
      String teamName = configFile.readStringUntil('\n');
      teamName.trim();
      if (teamName.length() > 0 && teamName.length() <= 20) {
        teams[i].name = teamName;
      }

      // Load MAC address
      String macLine = configFile.readStringUntil('\n');
      macLine.trim();
      if (macLine.length() > 0) {
        int macIndex = 0;
        int startPos = 0;
        for (int j = 0; j < 6 && macIndex < 6; j++) {
          int commaPos = macLine.indexOf(',', startPos);
          if (commaPos == -1 && j == 5) commaPos = macLine.length();

          if (commaPos != -1) {
            String byteStr = macLine.substring(startPos, commaPos);
            teams[i].mac[macIndex] = strtol(byteStr.c_str(), NULL, 16);
            macIndex++;
            startPos = commaPos + 1;
          }
        }
      }

      // Load configuration status
      String configStr = configFile.readStringUntil('\n');
      teams[i].isConfigured = (configStr.toInt() == 1);

      // Load mute flag (if available, default to false for older config files)
      if (configFile.available()) {
        String muteStr = configFile.readStringUntil('\n');
        teams[i].isMuted = (muteStr.toInt() == 1);
      } else {
        teams[i].isMuted = false;
      }

      Serial.printf("  Loaded Team %d: %s, MAC: %02X:%02X:%02X:%02X:%02X:%02X (%s)\n",
                    i + 1, teams[i].name.c_str(),
                    teams[i].mac[0], teams[i].mac[1], teams[i].mac[2],
                    teams[i].mac[3], teams[i].mac[4], teams[i].mac[5],
                    teams[i].isConfigured ? "configured" : "not configured");
    }

    // Load countdown settings (if available, for backward compatibility)
    if (configFile.available()) {
      String countdownEnabledStr = configFile.readStringUntil('\n');
      countdownEnabled = (countdownEnabledStr.toInt() == 1);
    }
    if (configFile.available()) {
      String countdownSecondsStr = configFile.readStringUntil('\n');
      int secs = countdownSecondsStr.toInt();
      if (secs >= 1 && secs <= 5) {
        countdownSeconds = secs;
      }
    }
    Serial.printf("  Countdown: %s, %d seconds\n",
                  countdownEnabled ? "enabled" : "disabled", countdownSeconds);

    // Load WiFi settings (if available)
    if (configFile.available()) {
      String ssid = configFile.readStringUntil('\n');
      ssid.trim();
      if (ssid.length() > 0 && ssid.length() <= 32) {
        ap_ssid = ssid;
      }
    }
    if (configFile.available()) {
      String pass = configFile.readStringUntil('\n');
      pass.trim();
      if (pass.length() >= 8 && pass.length() <= 63) {
        ap_password = pass;
      }
    }
    Serial.printf("  WiFi SSID: %s\n", ap_ssid.c_str());

    Serial.println("=== Configuration Loaded Successfully ===");
  } else {
    Serial.println("ERROR: Team count mismatch! Config file may be corrupted.");
  }

  configFile.close();
}

void initializeTeams() {
  for (int i = 0; i < MAX_TEAMS; i++) {
    teams[i].name = "Team " + String(i + 1);
    for (int j = 0; j < 6; j++) {
      teams[i].mac[j] = 0;
    }
    teams[i].isConfigured = false;
    teams[i].isOnline = false;
    teams[i].lastSeen = 0;
    teams[i].isMuted = false;
  }
}

void parseMacAddress(String macStr, uint8_t* mac) {
  macStr.replace(":", "");
  macStr.toUpperCase();
  
  for (int i = 0; i < 6; i++) {
    if (i * 2 + 1 < macStr.length()) {
      String byteStr = macStr.substring(i * 2, i * 2 + 2);
      mac[i] = strtol(byteStr.c_str(), NULL, 16);
    } else {
      mac[i] = 0;
    }
  }
}

void sendTeamStatusUpdate(int teamIndex) {
  // Uses the common helper function to send status to a specific team
  sendStatusMessageToTeam(teamIndex);
}

void sendButtonResponses() {
  // Send status update to all configured teams using the common helper
  for (int i = 0; i < MAX_TEAMS; i++) {
    if (teams[i].isConfigured) {
      sendStatusMessageToTeam(i);
    }
  }
}

void logBuzzerEvent(int teamIndex, unsigned long responseTime) {
  if (!sdCardAvailable) return;

  File logFile = SD.open("/buzzer_log.txt", FILE_APPEND);
  if (logFile) {
    logFile.printf("%lu,%s,%lu\n", millis(), teams[teamIndex].name.c_str(), responseTime);
    logFile.flush(); // Ensure data is written to SD card immediately
    logFile.close();
    Serial.printf("Logged buzzer event: Team %s, Time: %lu ms\n", teams[teamIndex].name.c_str(), responseTime);
  } else {
    Serial.println("ERROR: Failed to open buzzer_log.txt for writing!");
  }
}

void handlePhysicalButtons() {
  unsigned long currentTime = millis();

  // Read current button states (active LOW with pull-up)
  bool btnStartStopState = digitalRead(BTN_START_STOP);
  bool btnResetState = digitalRead(BTN_RESET);

  // Handle Start/Stop button
  if (btnStartStopState == LOW && lastBtnStartStopState == HIGH) {
    // Button just pressed (falling edge)
    if (currentTime - lastBtnStartStopPress > DEBOUNCE_DELAY_MS) {
      lastBtnStartStopPress = currentTime;

      // Toggle game state
      if (gameActive) {
        // Stop the game
        Serial.println("Physical button: STOP pressed");
        gameActive = false;
        winnerTeam = -1;
        winnerTime = 0;
        sendButtonResponses();
      } else {
        // Start the game
        Serial.println("Physical button: START pressed");
        gameActive = true;
        winnerTeam = -1;
        gameStartTime = millis();
        winnerTime = 0;
        sendButtonResponses();
      }
      updateDisplay();
    }
  }
  lastBtnStartStopState = btnStartStopState;

  // Handle Reset button
  if (btnResetState == LOW && lastBtnResetState == HIGH) {
    // Button just pressed (falling edge)
    if (currentTime - lastBtnResetPress > DEBOUNCE_DELAY_MS) {
      lastBtnResetPress = currentTime;

      // Reset round
      Serial.println("Physical button: RESET pressed");
      winnerTeam = -1;
      gameStartTime = millis();
      winnerTime = 0;

      // Only send responses if game is active
      if (gameActive) {
        sendButtonResponses();
      }
      updateDisplay();
    }
  }
  lastBtnResetState = btnResetState;
}

