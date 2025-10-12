#include <WiFi.h>
#include <WebServer.h>
#include <esp_now.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include "secret.h" // Contains SSID and PASSWORD definitions

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

// Access Point credentials - Change these for your setup
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
bool configLoaded = false; // Track if config was loaded from SD

// ESP-NOW callback - receives button presses (ESP32-C6 compatible)
void onDataRecv(const esp_now_recv_info *recv_info, const uint8_t *incomingData, int len) {
  if (!gameActive || firstBuzzer != -1) {
    return; // Game not active or someone already buzzed
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
    
    // Show config status
    if (configLoaded) {
      display.println("[Config Loaded]");
    }
    display.println();
    
    display.print("WiFi: ");
    display.println(ap_ssid);
    display.print("IP: ");
    display.println(WiFi.softAPIP());
    display.println();
    
    // Show team names from config
    display.print("Teams (");
    display.print(numTeams);
    display.println("):");
    
    // Display up to 3 team names on screen
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

// Web interface HTML
String getHTML() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>body{font-family:Arial;margin:20px;}";
  html += "input,select,button{padding:10px;margin:5px;font-size:16px;}";
  html += ".team{border:1px solid #ccc;padding:10px;margin:10px 0;}";
  html += "button{background:#4CAF50;color:white;border:none;cursor:pointer;}";
  html += "button:hover{background:#45a049;}</style></head><body>";
  html += "<h1>Quiz Buzzer Setup</h1>";
  
  html += "<h2>Game Controls</h2>";
  html += "<button onclick=\"fetch('/start')\">START GAME</button>";
  html += "<button onclick=\"fetch('/reset')\">RESET ROUND</button>";
  html += "<button onclick=\"fetch('/stop')\">STOP GAME</button>";
  
  html += "<h2>Team Configuration</h2>";
  html += "<form action='/config' method='POST'>";
  html += "Number of Teams: <select name='numteams'>";
  for (int i = 1; i <= 9; i++) {
    html += "<option value='" + String(i) + "'";
    if (i == numTeams) html += " selected";
    html += ">" + String(i) + "</option>";
  }
  html += "</select><br>";
  
  for (int i = 0; i < 9; i++) {
    html += "<div class='team'>";
    html += "Team " + String(i + 1) + ": ";
    html += "<input type='text' name='team" + String(i) + "' value='" + teams[i].name + "'>";
    html += " MAC: " + macToString(teams[i].buttonMAC);
    html += "</div>";
  }
  
  html += "<button type='submit'>Save Configuration</button>";
  html += "</form>";
  
  html += "<h2>Status</h2>";
  html += "<p>Game Active: " + String(gameActive ? "YES" : "NO") + "</p>";
  if (firstBuzzer != -1) {
    html += "<p>First Buzzer: " + teams[firstBuzzer].name + "</p>";
  }
  
  html += "<script>setInterval(()=>location.reload(),5000);</script>";
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
  if (server.hasArg("numteams")) {
    numTeams = server.arg("numteams").toInt();
  }
  
  for (int i = 0; i < 9; i++) {
    if (server.hasArg("team" + String(i))) {
      teams[i].name = server.arg("team" + String(i));
      teams[i].active = (i < numTeams);
    }
  }
  
  saveConfig();
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleStart() {
  gameActive = true;
  firstBuzzer = -1;
  updateDisplay();
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
  if (!SD.begin(SD_CS)) return;
  
  File configFile = SD.open("/config.txt", FILE_WRITE);
  if (configFile) {
    configFile.println(numTeams);
    for (int i = 0; i < 9; i++) {
      configFile.println(teams[i].name);
      for (int j = 0; j < 6; j++) {
        configFile.print(teams[i].buttonMAC[j]);
        if (j < 5) configFile.print(",");
      }
      configFile.println();
    }
    configFile.close();
  }
}

void loadConfig() {
  if (!SD.begin(SD_CS)) return;
  
  File configFile = SD.open("/config.txt");
  if (configFile) {
    numTeams = configFile.parseInt();
    configFile.readStringUntil('\n'); // consume newline
    
    for (int i = 0; i < 9; i++) {
      teams[i].name = configFile.readStringUntil('\n');
      teams[i].name.trim();
      if (teams[i].name == "") {
        teams[i].name = "Team " + String(i + 1);
      }
      teams[i].active = (i < numTeams);
      
      // Read MAC address
      for (int j = 0; j < 6; j++) {
        teams[i].buttonMAC[j] = configFile.parseInt();
        if (j < 5) configFile.read(); // consume comma
      }
      configFile.readStringUntil('\n'); // consume newline
    }
    configFile.close();
    configLoaded = true; // Set flag that config was loaded
    Serial.println("Configuration file loaded successfully");
  } else {
    Serial.println("No configuration file found, using defaults");
    configLoaded = false;
  }
}

void setup() {
  Serial.begin(115200);
  
  // Initialize I2C with ESP32-C6 pins
  Wire.begin(6, 7); // SDA=GPIO6, SCL=GPIO7 for ESP32-C6
  
  // Initialize display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED failed");
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
    Serial.println("SD Card initialized successfully");
    // Load previously saved configuration if it exists
    loadConfig();
    Serial.println("Configuration loaded from SD card");
  }
  
  // Set up Access Point
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ap_ssid, ap_password);
  
  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW failed");
    return;
  }
  esp_now_register_recv_cb(onDataRecv);
  
  // Set up web server
  server.on("/", handleRoot);
  server.on("/config", HTTP_POST, handleConfig);
  server.on("/start", handleStart);
  server.on("/reset", handleReset);
  server.on("/stop", handleStop);
  server.begin();
  
  updateDisplay();
  Serial.println("Base Station Ready");
}

void loop() {
  server.handleClient();
  delay(10);
}