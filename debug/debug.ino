void setup() {
  Serial.begin(115200);
  
  // Wait for Serial to be ready (ESP32-C6 USB CDC)
  delay(2000);
  while (!Serial && millis() < 5000) {
    delay(10);
  }
  
  Serial.println("\n\n\n=================================");
  Serial.println("Quiz Buzzer Base Station");
  Serial.println("================================= V4.1 DEBUG");
  Serial.flush();
  
  // Debug: Print what we're getting from secret.h
  Serial.println("\n--- Secret.h Values ---");
  Serial.print("WIFI_SSID defined as: ");
  Serial.println(WIFI_SSID);
  Serial.print("WIFI_PASSWORD defined as: ");
  Serial.println(WIFI_PASSWORD);
  Serial.print("ssid_str variable: ");
  Serial.println(ssid_str);
  Serial.print("password_str variable: ");
  Serial.println(password_str);
  Serial.print("SSID length: ");
  Serial.println(ssid_str.length());
  Serial.print("Password length: ");
  Serial.println(password_str.length());
  Serial.println("----------------------");
  Serial.flush();
  
  // Initialize I2C with ESP32-C6 pins
  Serial.println("\n--- Initializing I2C and Display ---");
  Serial.flush();
  Wire.begin(6, 7);
  Serial.println("I2C initialized (SDA=GPIO6, SCL=GPIO7)");
  Serial.flush();
  
  // Initialize display
  Serial.println("Attempting to initialize OLED display...");
  Serial.flush();
  
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("WARNING: OLED initialization failed!");
    Serial.println("Display will not work, but system will continue");
    Serial.flush();
  } else {
    Serial.println("OLED display initialized successfully");
    Serial.flush();
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Initializing...");
    display.display();
  }
  Serial.println("---------------------------------------");
  Serial.flush();
  
  // CHECKPOINT 1: SD Card (OPTIONAL - won't crash if it fails)
  Serial.println("\n*** CHECKPOINT 1: About to initialize SD Card ***");
  Serial.flush();
  delay(500); // Longer delay before SD card attempt
  
  // Initialize SD card with detailed debugging
  Serial.println("\n--- SD Card Initialization (Optional) ---");
  Serial.print("CS Pin: GPIO");
  Serial.println(SD_CS);
  Serial.flush();
  
  // Try to init SPI and SD card, but don't let it crash the system
  bool sdAvailable = false;
  Serial.println("Attempting SPI initialization...");
  Serial.flush();
  
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  Serial.println("SPI initialized");
  Serial.flush();
  
  delay(200); // Give SPI time to settle
  
  Serial.println("Attempting SD.begin()...");
  Serial.flush();
  
  // Wrap in try-catch equivalent (checking result)
  sdAvailable = SD.begin(SD_CS);
  
  if (!sdAvailable) {
    Serial.println("WARNING: SD Card not available");
    Serial.println("System will continue without persistent storage");
    Serial.println("Configuration will reset on power cycle");
    Serial.flush();
  } else {
    Serial.println("SUCCESS: SD Card initialized");
    Serial.flush();
    
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.print("SD Card Size: ");
    Serial.print(cardSize);
    Serial.println(" MB");
    Serial.flush();
    
    Serial.println("Loading configuration from SD card...");
    Serial.flush();
    loadConfig();
    Serial.println("loadConfig() completed");
    Serial.flush();
  }
  Serial.println("------------------------------");
  Serial.flush();
  
  // CHECKPOINT 2: WiFi
  Serial.println("\n*** CHECKPOINT 2: About to initialize WiFi ***");
  Serial.flush();
  delay(100);
  
  // Set up WiFi
  Serial.println("\n--- Setting up WiFi Access Point ---");
  
  // Validate SSID and password
  if (ssid_str.length() < 1 || ssid_str.length() > 32) {
    Serial.println("ERROR: SSID length invalid (must be 1-32 characters)");
    Serial.print("Current length: ");
    Serial.println(ssid_str.length());
    ssid_str = "QuizBuzzer";
    Serial.println("Using default SSID: QuizBuzzer");
  }
  
  if (password_str.length() < 8 || password_str.length() > 63) {
    Serial.println("WARNING: Password length invalid (must be 8-63 characters)");
    Serial.print("Current length: ");
    Serial.println(password_str.length());
    password_str = "buzzer123";
    Serial.println("Using default password: buzzer123");
  }
  
  Serial.print("Final SSID to use: '");
  Serial.print(ssid_str);
  Serial.println("'");
  Serial.print("Final Password to use: '");
  Serial.print(password_str);
  Serial.println("'");
  Serial.flush();
  
  WiFi.mode(WIFI_AP_STA);
  Serial.println("WiFi mode set to WIFI_AP_STA");
  Serial.flush();
  
  bool apResult = WiFi.softAP(ssid_str.c_str(), password_str.c_str());
  
  Serial.print("WiFi.softAP returned: ");
  Serial.println(apResult ? "SUCCESS" : "FAILED");
  Serial.flush();
  
  delay(1000); // Give WiFi time to start
  
  Serial.print("Actual broadcasting SSID: ");
  Serial.println(WiFi.softAPSSID());
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());
  Serial.println("--------------------------------------");
  Serial.flush();
  
  // CHECKPOINT 3: ESP-NOW
  Serial.println("\n*** CHECKPOINT 3: About to initialize ESP-NOW ***");
  Serial.flush();
  delay(100);
  
  Serial.println("Calling esp_now_init()...");
  Serial.flush();
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("ERROR: ESP-NOW initialization failed!");
    Serial.flush();
    return;
  }
  Serial.println("ESP-NOW initialized successfully");
  Serial.flush();
  
  esp_now_register_recv_cb(onDataRecv);
  Serial.println("ESP-NOW callback registered");
  Serial.flush();
  
  // CHECKPOINT 4: Web Server
  Serial.println("\n*** CHECKPOINT 4: About to start Web Server ***");
  Serial.flush();
  delay(100);
  
  Serial.println("Registering web server handlers...");
  Serial.flush();
  
  server.on("/", handleRoot);
  server.on("/config", HTTP_POST, handleConfig);
  server.on("/start", handleStart);
  server.on("/reset", handleReset);
  server.on("/stop", handleStop);
  
  Serial.println("Starting web server...");
  Serial.flush();
  server.begin();
  
  Serial.println("Web server started successfully");
  Serial.flush();
  
  // CHECKPOINT 5: Update Display
  Serial.println("\n*** CHECKPOINT 5: About to update display ***");
  Serial.flush();
  delay(100);
  
  updateDisplay();
  Serial.println("Display updated");
  Serial.flush();
  
  // Print final status
  Serial.println("\n=================================");
  Serial.println("BASE STATION READY!");
  Serial.println("=================================");
  Serial.println("\nBase Station MAC Address: " + WiFi.macAddress());
  Serial.println("Copy this into your button code's baseStationMAC array");
  Serial.println("\nWeb Interface:");
  Serial.println("  URL: http://192.168.4.1");
  Serial.println("  SSID: " + ssid_str);
  Serial.println("  Password: " + password_str);
  if (!sdAvailable) {
    Serial.println("\n*** WARNING: SD Card not available ***");
    Serial.println("Configuration will NOT persist after power cycle");
  }
  Serial.println("\n=================================\n");
  Serial.flush();
}