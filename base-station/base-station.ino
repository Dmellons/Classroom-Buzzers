#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define I2C_SDA 6
#define I2C_SCL 7

void setup() {
  Serial.begin(115200);
  delay(3000);
  
  Serial.println("\n=== DIAGNOSTIC TEST ===");
  
  // Test 1: I2C
  Serial.println("Step 1: Initializing I2C...");
  Wire.begin(I2C_SDA, I2C_SCL);
  delay(500);
  Serial.println("I2C OK");
  
  // Test 2: Display
  Serial.println("Step 2: Initializing Display...");
  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Display OK!");
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Test 1: OK");
    display.display();
    delay(1000);
  } else {
    Serial.println("Display FAILED!");
  }
  
  // Test 3: WiFi
  Serial.println("Step 3: Initializing WiFi...");
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("WiFi Test...");
  display.display();
  
  WiFi.mode(WIFI_AP);
  delay(100);
  Serial.println("WiFi mode set");
  
  WiFi.softAP("TestBuzzer", "test1234");
  delay(1000);
  Serial.println("WiFi AP started");
  
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("WiFi: TestBuzzer");
  display.println("IP: ");
  display.println(WiFi.softAPIP());
  display.display();
  
  Serial.println("\n=== ALL TESTS COMPLETE ===");
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());
  Serial.println("MAC: " + WiFi.macAddress());
}

void loop() {
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 2000) {
    Serial.println("Loop running...");
    lastPrint = millis();
  }
  delay(10);
}