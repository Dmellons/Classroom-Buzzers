void setup() {
  Serial.begin(115200);
  delay(2000);  // Simple delay, no waiting for Serial Monitor
  
  Serial.println("\n\n\n=================================");
  Serial.println("Quiz Buzzer Base Station");
  Serial.println("================================= V4.2");
  Serial.flush();
  
  // Initialize I2C with ESP32-C6 pins
  Wire.begin(6, 7);
  Serial.println("I2C initialized (SDA=GPIO6, SCL=GPIO7)");
  
  // Initialize display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("WARNING: OLED initialization failed!");
  } else {
    Serial.println("OLED display initialized successfully");
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Initializing...");
    display.display();
  }
  
  // Initialize SD card
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  delay(200);
  
  if (!SD.begin(SD_CS)) {
    Serial.println("WARNING: SD Card not available");
  } else {
    Serial.println("SD Card initialized");
    loadConfig();
  }
  
  // Set up WiFi
  WiFi.mode(WIFI_AP_STA);
  delay(100);
  
  WiFi.softAP(ssid_str.c_str(), password_str.c_str());
  delay(500);
  
  Serial.print("Broadcasting SSID: ");
  Serial.println(WiFi.softAPSSID());
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());
  
  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ERROR: ESP-NOW init failed!");
    return;
  }
  esp_now_register_recv_cb(onDataRecv);
  
  // Start web server
  server.on("/", handleRoot);
  server.on("/config", HTTP_POST, handleConfig);
  server.on("/start", handleStart);
  server.on("/reset", handleReset);
  server.on("/stop", handleStop);
  server.begin();
  
  updateDisplay();
  
  Serial.println("\n=================================");
  Serial.println("BASE STATION READY!");
  Serial.println("MAC: " + WiFi.macAddress());
  Serial.println("=================================\n");
}