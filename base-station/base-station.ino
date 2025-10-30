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

// WiFi credentials from secret file
const char* ap_ssid = WIFI_SSID;
const char* ap_password = WIFI_PASSWORD;

// Web server
WebServer server(80);

// Game state
bool gameActive = false;
int winnerTeam = -1;
unsigned long gameStartTime = 0;
unsigned long winnerTime = 0;

// Audio settings
bool audioMuted = false;

// Team configuration
#define MAX_TEAMS 4
struct Team {
  String name;
  uint8_t mac[6];
  bool isConfigured;
  bool isOnline;
  unsigned long lastSeen;
  bool isMuted;
};

Team teams[MAX_TEAMS];

// SD card available flag
bool sdCardAvailable = false;

void setup() {
  Serial.begin(115200);
  Serial.println("Base Station Starting...");
  
  // Initialize team structure with defaults
  initializeTeams();
  
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
  
  if (WiFi.softAP(ap_ssid, ap_password)) {
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
  server.on("/start", handleStart);
  server.on("/reset", handleReset);
  server.on("/stop", handleStop);
  server.on("/save", handleSave);
  server.on("/mute", handleMute);
  server.on("/unmute", handleUnmute);
  server.on("/mute-team", handleMuteTeam);
  server.on("/unmute-team", handleUnmuteTeam);
  server.begin();
  
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
  
  // Check for button timeouts (mark offline if not seen recently)
  unsigned long currentTime = millis();
  for (int i = 0; i < MAX_TEAMS; i++) {
    if (teams[i].isConfigured && teams[i].isOnline) {
      if (currentTime - teams[i].lastSeen > 30000) { // 30 second timeout
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
  
  if (teamIndex == -1) return; // Unknown button
  
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

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  
  if (gameActive) {
    if (winnerTeam >= 0) {
      display.setTextSize(2);
      display.println("WINNER:");
      display.println(teams[winnerTeam].name);
      display.setTextSize(1);
      display.printf("Time: %lu ms\n", winnerTime);
    } else {
      display.setTextSize(2);
      display.println("READY!");
      display.setTextSize(1);
      display.println("Waiting for buzz...");
    }
  } else {
    if (sdCardAvailable) {
      display.println("Quiz Buzzer System");
      display.println("[Config Saved]");
    } else {
      display.println("Quiz Buzzer System");
      display.println("[No SD Card]");
    }
    
    display.println();
    display.printf("WiFi: %s\n", ap_ssid);
    display.printf("IP: %s\n", WiFi.softAPIP().toString().c_str());
    display.println();
    
    display.println("Teams:");
    for (int i = 0; i < MAX_TEAMS; i++) {
      display.printf("%d. %s", i + 1, teams[i].name.c_str());
      if (teams[i].isConfigured && teams[i].isOnline) {
        display.print(" *");
      }
      display.println();
    }
  }
  
  display.display();
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head><title>Quiz Buzzer</title>";
  html += "<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>body{font-family:Arial;margin:20px;} button{padding:15px 30px;margin:10px;font-size:18px;background:#007bff;color:white;border:none;border-radius:5px;cursor:pointer;} ";
  html += "button:hover{background:#0056b3;} .winner{color:green;} .active{color:blue;} input{padding:8px;margin:5px;border:1px solid #ccc;border-radius:3px;} ";
  html += ".config{background:#f8f9fa;padding:15px;border-radius:5px;margin:10px 0;}</style></head><body>";
  html += "<h1>Quiz Buzzer System</h1>";
  html += "<p><strong>WiFi:</strong> " + String(ap_ssid) + "</p>";
  html += "<p><strong>IP:</strong> " + WiFi.softAPIP().toString() + "</p>";
  html += "<p><strong>MAC:</strong> " + WiFi.macAddress() + "</p>";
  html += "<hr>";
  
  if (gameActive) {
    if (winnerTeam >= 0) {
      html += "<h2 class='winner'>WINNER: " + teams[winnerTeam].name + "!</h2>";
      html += "<p><strong>Response Time:</strong> " + String(winnerTime) + " ms</p>";
      html += "<button onclick=\"location.href='/reset'\">RESET ROUND</button>";
    } else {
      html += "<h2 class='active'>Game Active - Ready for Buzzes!</h2>";
      html += "<button onclick=\"location.href='/reset'\">RESET ROUND</button>";
    }
    html += "<button onclick=\"location.href='/stop'\">STOP GAME</button>";
  } else {
    html += "<h2>Game Stopped</h2>";
    html += "<button onclick=\"location.href='/start'\">START GAME</button>";
  }
  
  // Add audio control
  html += "<hr><h3>Audio Settings</h3>";
  if (audioMuted) {
    html += "<p>🔇 Audio: MUTED</p>";
    html += "<button onclick=\"location.href='/unmute'\">UNMUTE ALL BUTTONS</button>";
  } else {
    html += "<p>🔊 Audio: ENABLED</p>";
    html += "<button onclick=\"location.href='/mute'\">MUTE ALL BUTTONS</button>";
  }
  
  // Add ESP-NOW status
  html += "<hr><h3>ESP-NOW Status</h3>";
  html += "<p><strong>Base Station MAC:</strong> " + WiFi.macAddress() + "</p>";
  html += "<p><strong>Active Connections:</strong> ";
  int activeCount = 0;
  for (int i = 0; i < MAX_TEAMS; i++) {
    if (teams[i].isConfigured && teams[i].isOnline) activeCount++;
  }
  html += String(activeCount) + " / " + String(MAX_TEAMS) + "</p>";
  
  html += "<hr><div class='config'><h3>Team Configuration</h3>";
  html += "<form method='GET' action='/save'>";
  
  for (int i = 0; i < MAX_TEAMS; i++) {
    html += "<div style='background:#f0f0f0;padding:10px;margin:10px 0;border-radius:5px;'>";
    html += "<h4>Team " + String(i + 1) + "</h4>";
    
    html += "<p>Name: <input type='text' name='team" + String(i) + "' value='" + teams[i].name + "' maxlength='20' style='width:200px;'></p>";
    
    html += "<p>Button MAC: <input type='text' name='mac" + String(i) + "' value='";
    for (int j = 0; j < 6; j++) {
      if (j > 0) html += ":";
      if (teams[i].mac[j] < 16) html += "0";
      String hexByte = String(teams[i].mac[j], HEX);
      hexByte.toUpperCase();
      html += hexByte;
    }
    html += "' placeholder='AA:BB:CC:DD:EE:FF' style='width:200px;'></p>";
    
    html += "<p>Status: ";
    if (teams[i].isConfigured) {
      if (teams[i].isOnline) {
        html += "<span style='color:green'>●</span> Online";
      } else {
        html += "<span style='color:orange'>●</span> Configured (Not seen recently)";
      }
    } else {
      html += "<span style='color:red'>●</span> Not configured";
    }
    html += "</p></div>";
  }
  
  html += "<button type='submit'>Save Configuration</button>";
  html += "</form></div>";
  
  html += "<hr><h3>Current Teams:</h3>";
  html += "<table style='border-collapse:collapse;width:100%;'>";
  html += "<tr style='background:#f0f0f0;'><th style='border:1px solid #ccc;padding:8px;'>Team</th><th style='border:1px solid #ccc;padding:8px;'>Name</th><th style='border:1px solid #ccc;padding:8px;'>Status</th><th style='border:1px solid #ccc;padding:8px;'>Audio</th></tr>";
  
  for (int i = 0; i < MAX_TEAMS; i++) {
    html += "<tr><td style='border:1px solid #ccc;padding:8px;'>" + String(i + 1) + "</td>";
    html += "<td style='border:1px solid #ccc;padding:8px;'>" + teams[i].name + "</td>";
    html += "<td style='border:1px solid #ccc;padding:8px;'>";
    
    if (teams[i].isConfigured) {
      if (teams[i].isOnline) {
        html += "<span style='color:green'>●</span> Online";
      } else {
        html += "<span style='color:orange'>●</span> Offline";
      }
    } else {
      html += "<span style='color:red'>●</span> Not configured";
    }
    
    html += "</td><td style='border:1px solid #ccc;padding:8px;'>";
    
    // Individual mute controls
    if (teams[i].isConfigured) {
      if (teams[i].isMuted || audioMuted) {
        html += "🔇 ";
        if (!audioMuted) { // Only show unmute if not globally muted
          html += "<a href='/unmute-team?id=" + String(i) + "' style='font-size:12px;'>Unmute</a>";
        } else {
          html += "<span style='font-size:12px;color:gray;'>Global Mute</span>";
        }
      } else {
        html += "🔊 <a href='/mute-team?id=" + String(i) + "' style='font-size:12px;'>Mute</a>";
      }
    } else {
      html += "-";
    }
    
    html += "</td></tr>";
  }
  html += "</table>";
  
  html += "<hr><p><strong>SD Card:</strong> ";
  if (sdCardAvailable) {
    html += "<span style='color:green'>Available - Settings will be saved</span>";
  } else {
    html += "<span style='color:red'>Not Available - Settings will be lost on restart</span>";
  }
  html += "</p>";
  
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleStart() {
  Serial.println("=== STARTING GAME ===");
  gameActive = true;
  winnerTeam = -1;
  gameStartTime = millis();
  winnerTime = 0;
  
  // Send start signal to all configured buttons
  Serial.println("Sending game start messages to all configured teams...");
  sendButtonResponses();
  
  updateDisplay();
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "Game Started!");
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
      server.sendHeader("Location", "/");
      server.send(302, "text/plain", teams[teamId].name + " Muted!");
      return;
    }
  }
  
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "Invalid team!");
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
      server.sendHeader("Location", "/");
      server.send(302, "text/plain", teams[teamId].name + " Unmuted!");
      return;
    }
  }
  
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "Invalid team!");
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
  
  // Redirect back to main page
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "Configuration saved!");
}

void saveConfig() {
  if (!sdCardAvailable) return;
  
  File configFile = SD.open("/config.txt", FILE_WRITE);
  if (configFile) {
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
    }
    configFile.close();
  }
}

void loadConfig() {
  if (!sdCardAvailable) return;
  
  File configFile = SD.open("/config.txt");
  if (configFile) {
    int numTeams = configFile.readStringUntil('\n').toInt();
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
      }
    }
    configFile.close();
  }
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
  if (teamIndex < 0 || teamIndex >= MAX_TEAMS || !teams[teamIndex].isConfigured) {
    return;
  }
  
  uint8_t message[32]; // Message with team name
  memset(message, 0, sizeof(message)); // Clear message buffer
  
  // Determine current game status for this team
  if (!gameActive) {
    message[0] = 0; // Game stopped
  } else if (winnerTeam == -1) {
    message[0] = 3; // Game active, ready
  } else if (winnerTeam == teamIndex) {
    message[0] = 1; // Winner
  } else {
    message[0] = 2; // Locked out
  }
  
  // Add individual mute flag at byte 1 (individual team mute OR global mute)
  message[1] = (audioMuted || teams[teamIndex].isMuted) ? 1 : 0;
  
  // Add team name starting at byte 2 (max 29 chars + null terminator)
  String teamName = teams[teamIndex].name;
  if (teamName.length() > 29) teamName = teamName.substring(0, 29);
  strcpy((char*)&message[2], teamName.c_str());
  
  // Debug: Print what we're about to send
  Serial.printf("Heartbeat response - Team %d (%s): Sending message %d\n",
    teamIndex + 1, teams[teamIndex].name.c_str(), message[0]);
  
  // Add button as peer if not already added
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, teams[teamIndex].mac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  
  if (!esp_now_is_peer_exist(teams[teamIndex].mac)) {
    esp_err_t addResult = esp_now_add_peer(&peerInfo);
    Serial.printf("Adding peer: %s\n", (addResult == ESP_OK) ? "SUCCESS" : "FAILED");
  }
  
  // Send message
  esp_err_t sendResult = esp_now_send(teams[teamIndex].mac, message, sizeof(message));
  Serial.printf("Team status update result: %s\n", (sendResult == ESP_OK) ? "OK" : "ERROR");
}

void sendButtonResponses() {
  for (int i = 0; i < MAX_TEAMS; i++) {
    if (teams[i].isConfigured) {
      uint8_t message[32]; // Increased size for team name
      memset(message, 0, sizeof(message)); // Clear message buffer
      
      if (!gameActive) {
        message[0] = 0; // Game stopped
      } else if (winnerTeam == -1) {
        message[0] = 3; // Game active, ready
      } else if (winnerTeam == i) {
        message[0] = 1; // Winner
      } else {
        message[0] = 2; // Locked out
      }
      
      // Add individual mute flag at byte 1 (individual team mute OR global mute)
      message[1] = (audioMuted || teams[i].isMuted) ? 1 : 0;
      
      // Add team name starting at byte 2 (max 29 chars + null terminator)
      String teamName = teams[i].name;
      if (teamName.length() > 29) teamName = teamName.substring(0, 29);
      strcpy((char*)&message[2], teamName.c_str());
      
      // Debug: Print what we're about to send
      Serial.printf("Team %d (%s): Sending message %d to %02X:%02X:%02X:%02X:%02X:%02X\n",
        i + 1, teams[i].name.c_str(), message[0],
        teams[i].mac[0], teams[i].mac[1], teams[i].mac[2],
        teams[i].mac[3], teams[i].mac[4], teams[i].mac[5]);
      
      // Add button as peer if not already added
      esp_now_peer_info_t peerInfo;
      memcpy(peerInfo.peer_addr, teams[i].mac, 6);
      peerInfo.channel = 0;
      peerInfo.encrypt = false;
      
      if (!esp_now_is_peer_exist(teams[i].mac)) {
        esp_err_t addResult = esp_now_add_peer(&peerInfo);
        Serial.printf("Adding peer: %s\n", (addResult == ESP_OK) ? "SUCCESS" : "FAILED");
      }
      
      // Send message
      esp_err_t sendResult = esp_now_send(teams[i].mac, message, sizeof(message));
      Serial.printf("esp_now_send result: %s\n", (sendResult == ESP_OK) ? "OK" : "ERROR");
    }
  }
}

void logBuzzerEvent(int teamIndex, unsigned long responseTime) {
  if (!sdCardAvailable) return;
  
  File logFile = SD.open("/buzzer_log.txt", FILE_APPEND);
  if (logFile) {
    logFile.printf("%lu,%s,%lu\n", millis(), teams[teamIndex].name.c_str(), responseTime);
    logFile.close();
  }
}

