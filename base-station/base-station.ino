#include <WiFi.h>
#include <WebServer.h>
#include <esp_now.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include "base-secret.h" // Contains WIFI_SSID and WIFI_PASSWORD definitions

// OLED Display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// SD Card pins (adjust based on your module)
#define SD_CS 5
#define SD_MOSI 23
#define SD_MISO 19
#define SD_SCK 18

// WS2812B LED Strip pin
#define LED_PIN 4
#define NUM_LEDS 60

// Access Point credentials from secret.h
const char* ap_ssid = WIFI_SSID;
const char* ap_password = WIFI_PASSWORD;

// Web server
WebServer server(80);

// Game state
struct TeamData {
  String name;
  bool active;
  uint8_t buttonMAC[6];
};

TeamData teams[9];
int numTeams = 4;
bool gameActive = false;
int firstBuzzer = -1;
unsigned long buzzerTime = 0;
bool configLoaded = false;

// Helper function to parse MAC address from string
bool parseMACAddress(const String &macStr, uint8_t *mac) {
  int values[6];
  int count = sscanf(macStr.c_str(), "%x:%x:%x:%x:%x:%x", 
                     &values[0], &values[1], &values[2], 
                     &values[3], &values[4], &values[5]);
  
  if (count == 6) {
    for (int i = 0; i < 6; i++) {
      mac[i] = (uint8_t)values[i];
    }
    return true;
  }
  return false;
}

// ESP-NOW callback - receives button presses (ESP32-C6 compatible)
void onDataRecv(const esp_now_recv_info *recv_info, const uint8_t *incomingData, int len) {
  if (!gameActive || firstBuzzer != -1) {
    return;
  }
  
  const uint8_t *mac = recv_info->src_addr;
  
  // Find which team this MAC belongs to
  for (int i = 0; i < numTeams; i++) {
    if (memcmp(mac, teams[i].buttonMAC, 6) == 0 && teams[i].active) {
      firstBuzzer = i;
      buzzerTime = millis();
      
      // Log to SD card
      logBuzzer(i);
      
      // Update display
      updateDisplay();
      
      // Send acknowledgment back to button
      sendAckToButton(mac, true);
      
      // Send "locked out" to other buttons
      for (int j = 0; j < numTeams; j++) {
        if (j != i && teams[j].active) {
          sendAckToButton(teams[j].buttonMAC, false);
        }
      }
      break;
    }
  }
}

void sendAckToButton(const uint8_t *mac, bool winner) {
  uint8_t data = winner ? 1 : 0;
  esp_now_send(mac, &data, sizeof(data));
}

void logBuzzer(int teamNum) {
  if (!SD.begin(SD_CS)) {
    return;
  }
  
  File logFile = SD.open("/buzzer_log.txt", FILE_APPEND);
  if (logFile) {
    logFile.print(millis());
    logFile.print(",");
    logFile.print(teams[teamNum].name);
    logFile.println();
    logFile.close();
  }
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  if (!gameActive) {
    // Show setup info
    display.setCursor(0, 0);
    display.println("Quiz Buzzer System");
    
    if (configLoaded) {
      display.println("[Config Loaded]");
    }
    display.println();
    
    display.print("WiFi: ");
    display.println(ap_ssid);
    display.print("IP: ");
    display.println(WiFi.softAPIP());
    display.println();
    
    display.print("Teams (");
    display.print(numTeams);
    display.println("):");
    
    for (int i = 0; i < min(numTeams, 3); i++) {
      display.print(i + 1);
      display.print(". ");
      display.println(teams[i].name);
    }
    
    if (numTeams > 3) {
      display.println("...");
    }
  } else if (firstBuzzer == -1) {
    // Waiting for buzz
    display.setTextSize(2);
    display.setCursor(0, 20);
    display.println("READY!");
  } else {
    // Show winner
    display.setTextSize(2);
    display.setCursor(0, 10);
    display.println("WINNER:");
    display.setCursor(0, 35);
    display.println(teams[firstBuzzer].name);
  }
  
  display.display();
}

// Web interface HTML with MAC address inputs
String getHTML() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body{font-family:Arial;margin:20px;background:#f0f0f0;}";
  html += ".container{max-width:800px;margin:0 auto;background:white;padding:20px;border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,0.1);}";
  html += "h1{color:#333;border-bottom:2px solid #4CAF50;padding-bottom:10px;}";
  html += "h2{color:#555;margin-top:30px;}";
  html += "input,select,button{padding:10px;margin:5px;font-size:16px;border:1px solid #ddd;border-radius:4px;}";
  html += "input[type='text']{width:200px;}";
  html += ".team{border:1px solid #ddd;padding:15px;margin:10px 0;background:#f9f9f9;border-radius:4px;}";
  html += ".team label{display:inline-block;width:120px;font-weight:bold;}";
  html += "button{background:#4CAF50;color:white;border:none;cursor:pointer;font-weight:bold;}";
  html += "button:hover{background:#45a049;}";
  html += ".status{padding:10px;background:#e3f2fd;border-left:4px solid #2196F3;margin:10px 0;}";
  html += ".controls{margin:20px 0;}";
  html += ".controls button{margin-right:10px;}";
  html += ".mac-input{font-family:monospace;}";
  html += ".help{font-size:12px;color:#666;font-style:italic;}";
  html += "</style></head><body>";
  html += "<div class='container'>";
  html += "<h1>Quiz Buzzer Setup</h1>";
  
  // Status section
  html += "<div class='status'>";
  html += "<strong>Status:</strong> Game is " + String(gameActive ? "<span style='color:green'>ACTIVE</span>" : "<span style='color:red'>STOPPED</span>");
  if (firstBuzzer != -1) {
    html += " | First Buzzer: <strong>" + teams[firstBuzzer].name + "</strong>";
  }
  html += "</div>";
  
  // Game controls
  html += "<h2>Game Controls</h2>";
  html += "<div class='controls'>";
  html += "<button onclick=\"fetch('/start').then(()=>location.reload())\">START GAME</button>";
  html += "<button onclick=\"fetch('/reset').then(()=>location.reload())\">RESET ROUND</button>";
  html += "<button onclick=\"fetch('/stop').then(()=>location.reload())\">STOP GAME</button>";
  html += "</div>";
  
  // Team configuration form
  html += "<h2>Team Configuration</h2>";
  html += "<form action='/config' method='POST'>";
  
  html += "<div style='margin-bottom:20px;'>";
  html += "<label for='numteams'><strong>Number of Teams:</strong></label> ";
  html += "<select name='numteams' id='numteams'>";
  for (int i = 1; i <= 9; i++) {
    html += "<option value='" + String(i) + "'";
    if (i == numTeams) html += " selected";
    html += ">" + String(i) + "</option>";
  }
  html += "</select>";
  html += "</div>";
  
  // Team entries
  for (int i = 0; i < 9; i++) {
    html += "<div class='team'>";
    html += "<h3 style='margin-top:0;'>Team " + String(i + 1) + "</h3>";
    
    html += "<div style='margin-bottom:10px;'>";
    html += "<label>Name:</label>";
    html += "<input type='text' name='team" + String(i) + "' value='" + teams[i].name + "' placeholder='Team Name'>";
    html += "</div>";
    
    html += "<div>";
    html += "<label>MAC Address:</label>";
    html += "<input type='text' name='mac" + String(i) + "' value='" + macToString(teams[i].buttonMAC) + "' ";
    html += "class='mac-input' placeholder='AA:BB:CC:DD:EE:FF' pattern='[A-Fa-f0-9:]+' title='Format: AA:BB:CC:DD:EE:FF'>";
    html += "<br><span class='help'>Format: AA:BB:CC:DD:EE:FF (get from button's Serial Monitor)</span>";
    html += "</div>";
    
    html += "</div>";
  }
  
  html += "<button type='submit' style='font-size:18px;padding:15px 30px;margin-top:20px;'>Save Configuration</button>";
  html += "</form>";
  
  // Base station MAC info
  html += "<h2>Base Station Info</h2>";
  html += "<div class='status'>";
  html += "<strong>Base Station MAC:</strong> <code style='font-size:16px;'>" + WiFi.macAddress() + "</code><br>";
  html += "<span class='help'>Copy this MAC address into your button code (baseStationMAC array)</span>";
  html += "</div>";
  
  html += "</div>"; // container
  html += "</body></html>";
  
  return html;
}

String macToString(const uint8_t *mac) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(macStr);
}

void handleRoot() {
  server.send(200, "text/html", getHTML());
}

void handleConfig() {
  // Update number of teams
  if (server.hasArg("numteams")) {
    numTeams = server.arg("numteams").toInt();
  }
  
  // Update team names and MAC addresses
  for (int i = 0; i < 9; i++) {
    if (server.hasArg("team" + String(i))) {
      teams[i].name = server.arg("team" + String(i));
      teams[i].active = (i < numTeams);
    }
    
    // Parse MAC address
    if (server.hasArg("mac" + String(i))) {
      String macStr = server.arg("mac" + String(i));
      if (macStr.length() > 0) {
        if (parseMACAddress(macStr, teams[i].buttonMAC)) {
          Serial.println("Updated MAC for " + teams[i].name + ": " + macStr);
        } else {
          Serial.println("Failed to parse MAC for " + teams[i].name + ": " + macStr);
        }
      }
    }
  }
  
  saveConfig();
  updateDisplay();
  
  // Redirect back to main page
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleStart() {
  gameActive = true;
  firstBuzzer = -1;
  updateDisplay();
  
  // Send ready signal to all buttons
  for (int i = 0; i < numTeams; i++) {
    if (teams[i].active) {
      uint8_t readyData = 2;
      esp_now_send(teams[i].buttonMAC, &readyData, sizeof(readyData));
    }
  }
  
  server.send(200, "text/plain", "Game Started");
}

void handleReset() {
  firstBuzzer = -1;
  updateDisplay();
  
  // Send reset signal to all buttons
  for (int i = 0; i < numTeams; i++) {
    if (teams[i].active) {
      uint8_t resetData = 2;
      esp_now_send(teams[i].buttonMAC, &resetData, sizeof(resetData));
    }
  }
  
  server.send(200, "text/plain", "Round Reset");
}

void handleStop() {
  gameActive = false;
  firstBuzzer = -1;
  updateDisplay();
  server.send(200, "text/plain", "Game Stopped");
}

void saveConfig() {
  Serial.println("\n--- Attempting to save configuration ---");
  
  if (!SD.begin(SD_CS)) {
    Serial.println("ERROR: SD Card failed - config not saved");
    return;
  }
  
  Serial.println("SD Card available, opening config.txt for writing...");
  
  File configFile = SD.open("/config.txt", FILE_WRITE);
  if (configFile) {
    Serial.println("Config file opened for writing");
    Serial.print("Saving number of teams: ");
    Serial.println(numTeams);
    
    configFile.println(numTeams);
    for (int i = 0; i < 9; i++) {
      Serial.print("Saving Team ");
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(teams[i].name);
      Serial.print(" - MAC: ");
      
      configFile.println(teams[i].name);
      for (int j = 0; j < 6; j++) {
        configFile.print(teams[i].buttonMAC[j]);
        Serial.print(teams[i].buttonMAC[j], HEX);
        if (j < 5) {
          configFile.print(",");
          Serial.print(":");
        }
      }
      configFile.println();
      Serial.println();
    }
    configFile.close();
    Serial.println("Configuration saved successfully to SD card");
  } else {
    Serial.println("ERROR: Failed to open config.txt for writing");
  }
  Serial.println("---------------------------------------\n");
}

void loadConfig() {
  Serial.println("\n--- Attempting to load configuration ---");
  
  if (!SD.begin(SD_CS)) {
    Serial.println("ERROR: SD Card not available or failed to initialize");
    Serial.println("Check SD card is inserted and wired correctly");
    return;
  }
  
  Serial.println("SD Card initialized successfully");
  
  if (!SD.exists("/config.txt")) {
    Serial.println("WARNING: config.txt does not exist on SD card");
    Serial.println("Using default configuration");
    // Initialize with default values
    for (int i = 0; i < 9; i++) {
      teams[i].name = "Team " + String(i + 1);
      teams[i].active = (i < numTeams);
      // Initialize MAC to zeros
      for (int j = 0; j < 6; j++) {
        teams[i].buttonMAC[j] = 0;
      }
    }
    configLoaded = false;
    return;
  }
  
  Serial.println("Found config.txt, attempting to read...");
  
  File configFile = SD.open("/config.txt");
  if (configFile) {
    Serial.println("Config file opened successfully");
    
    numTeams = configFile.parseInt();
    Serial.print("Number of teams: ");
    Serial.println(numTeams);
    
    configFile.readStringUntil('\n');
    
    for (int i = 0; i < 9; i++) {
      teams[i].name = configFile.readStringUntil('\n');
      teams[i].name.trim();
      if (teams[i].name == "") {
        teams[i].name = "Team " + String(i + 1);
      }
      teams[i].active = (i < numTeams);
      
      Serial.print("Team ");
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(teams[i].name);
      Serial.print(" - MAC: ");
      
      // Read MAC address
      for (int j = 0; j < 6; j++) {
        teams[i].buttonMAC[j] = configFile.parseInt();
        Serial.print(teams[i].buttonMAC[j], HEX);
        if (j < 5) {
          Serial.print(":");
          configFile.read();
        }
      }
      Serial.println();
      configFile.readStringUntil('\n');
    }
    configFile.close();
    configLoaded = true;
    Serial.println("Configuration loaded successfully!");
  } else {
    Serial.println("ERROR: Failed to open config.txt for reading");
    // Initialize with default values
    for (int i = 0; i < 9; i++) {
      teams[i].name = "Team " + String(i + 1);
      teams[i].active = (i < numTeams);
      for (int j = 0; j < 6; j++) {
        teams[i].buttonMAC[j] = 0;
      }
    }
    configLoaded = false;
  }
  Serial.println("---------------------------------------\n");
}

void setup() {
  Serial.begin(115200);
  
  // Wait for Serial to be ready (ESP32-C6 USB CDC)
  delay(2000);
  while (!Serial && millis() < 5000) {
    delay(10);
  }
  
  // Print base station MAC address
  Serial.println("\n\n\n=================================");
  Serial.println("Quiz Buzzer Base Station");
  Serial.println("=================================");
  
  // Debug: Print what we're getting from secret.h
  Serial.println("\n--- Secret.h Values ---");
  Serial.print("WIFI_SSID defined as: ");
  Serial.println(WIFI_SSID);
  Serial.print("WIFI_PASSWORD defined as: ");
  Serial.println(WIFI_PASSWORD);
  Serial.print("ap_ssid variable: ");
  Serial.println(ap_ssid);
  Serial.print("ap_password variable: ");
  Serial.println(ap_password);
  Serial.println("----------------------");
  
  // Initialize I2C with ESP32-C6 pins
  Wire.begin(6, 7);
  
  // Initialize display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED initialization failed!");
    while (1);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Initializing...");
  display.display();
  
  // Initialize SD card
  if (!SD.begin(SD_CS)) {
    Serial.println("SD Card failed - configuration will not persist");
  } else {
    Serial.println("SD Card initialized");
    loadConfig();
  }
  
  // Set up WiFi
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ap_ssid, ap_password);
  
  // Print WiFi details
  Serial.println("\n--- WiFi Configuration ---");
  Serial.print("SSID: ");
  Serial.println(ap_ssid);
  Serial.print("Password: ");
  Serial.println(ap_password);
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());
  Serial.println("-------------------------");
  
  // Print MAC address for button configuration
  Serial.println("\nBase Station MAC Address: " + WiFi.macAddress());
  Serial.println("Copy this into your button code's baseStationMAC array");
  Serial.println("=================================\n");
  
  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW initialization failed!");
    return;
  }
  esp_now_register_recv_cb(onDataRecv);
  Serial.println("ESP-NOW initialized");
  
  // Set up web server
  server.on("/", handleRoot);
  server.on("/config", HTTP_POST, handleConfig);
  server.on("/start", handleStart);
  server.on("/reset", handleReset);
  server.on("/stop", handleStop);
  server.begin();
  
  Serial.println("Web server started at http://192.168.4.1");
  Serial.println("WiFi SSID: " + String(ap_ssid));
  Serial.println("WiFi Password: " + String(ap_password));
  
  updateDisplay();
  Serial.println("\nBase Station Ready!");
}

void loop() {
  server.handleClient();
  delay(10);
}