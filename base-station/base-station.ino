#include <WiFi.h>
#include <WebServer.h>
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

// Team configuration
#define MAX_TEAMS 4
struct Team {
  String name;
  uint8_t mac[6];
  bool isConfigured;
  bool isOnline;
  unsigned long lastSeen;
};

Team teams[MAX_TEAMS];

// SD card available flag
bool sdCardAvailable = false;

void setup() {
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
  
  // Start WiFi Access Point
  WiFi.mode(WIFI_AP);
  delay(100);
  
  if (WiFi.softAP(ap_ssid, ap_password)) {
    // WiFi started successfully
    updateDisplay();
  } else {
    // WiFi failed - show error on display
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("WiFi Failed!");
    display.display();
  }
  
  // Setup web server
  server.on("/", handleRoot);
  server.on("/start", handleStart);
  server.on("/reset", handleReset);
  server.on("/stop", handleStop);
  server.on("/save", handleSave);
  server.begin();
  
  // Blink LED to show ready
  pinMode(LED_BUILTIN, OUTPUT);
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(200);
    digitalWrite(LED_BUILTIN, LOW);
    delay(200);
  }
}

void loop() {
  server.handleClient();
  delay(2);
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  
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
  
  if (gameActive) {
    if (winnerTeam >= 0) {
      display.setTextSize(2);
      display.println("WINNER:");
      display.println(teams[winnerTeam].name);
    } else {
      display.setTextSize(2);
      display.println("READY!");
    }
  } else {
    display.println("Teams:");
    for (int i = 0; i < MAX_TEAMS; i++) {
      display.printf("%d. %s\n", i + 1, teams[i].name.c_str());
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
      html += String(teams[i].mac[j], HEX);
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
  html += "<tr style='background:#f0f0f0;'><th style='border:1px solid #ccc;padding:8px;'>Team</th><th style='border:1px solid #ccc;padding:8px;'>Name</th><th style='border:1px solid #ccc;padding:8px;'>Status</th></tr>";
  
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
  gameActive = true;
  winnerTeam = -1;
  updateDisplay();
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "Game Started!");
}

void handleReset() {
  winnerTeam = -1;
  updateDisplay();
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "Round Reset!");
}

void handleStop() {
  gameActive = false;
  winnerTeam = -1;
  updateDisplay();
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "Game Stopped!");
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

