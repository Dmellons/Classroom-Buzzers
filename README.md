# ESP32 Quiz Buzzer System

A professional quiz buzzer system built with ESP32-C6 microcontrollers, featuring wireless button communication via ESP-NOW, web-based configuration, and real-time OLED display feedback.

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-ESP32--C6-green.svg)

## 🎯 Features

- **Up to 9 Teams**: Support for 1-9 wireless buzzer buttons
- **ESP-NOW Communication**: Ultra-low latency (<10ms) wireless protocol
- **Web Configuration Interface**: Easy setup via WiFi hotspot
- **OLED Display**: Real-time game status, team names, and winner display
- **SD Card Logging**: Permanent record of all buzzer events with timestamps
- **Auto-Load Configuration**: Team names and settings automatically restored on power-up
- **Built-in LED Feedback**: Arcade buttons with integrated illumination
- **Battery Powered Buttons**: Rechargeable 18650 Li-ion batteries with TP4056 charging
- **Persistent Configuration**: Team names and settings saved to SD card

## 📸 Demo

*[Add photos of your completed system here]*

## 🛠️ Hardware Requirements

### Base Station (1x)
- ESP32-C6-DevKitC-1 development board
- 128x64 OLED Display (I2C, SSD1306)
- MicroSD Card Reader Module (SPI)
- MicroSD Card (formatted FAT32)
- USB cable for power
- Jumper wires

### Button Units (1-9x, one per team)
- ESP32-C6-DevKitC-1 development board
- 60mm LED Arcade Button
- 18650 Li-ion Battery (3.7V, protected recommended)
- TP4056 Battery Charging Module
- MT3608 DC-DC Boost Converter (x2 per button)
- 10kΩ Resistor
- 1kΩ Resistor (for transistor base)
- 2N2222 or similar NPN transistor (for LED control)
- Toggle Switch (power switch)
- 18650 Battery Holder
- Jumper wires

### Optional
- WS2812B LED Strip (for visual effects on base station)
- Logic Level Converter (3.3V to 5V, if using LED strip)
- External 5V power supply (for LED strip)

## 📁 Repository Structure

```
esp32-quiz-buzzer/
├── base-station/           # Base station code
│   ├── base-station.ino
│   ├── secret.h
│   └── README.md
├── button/                 # Button unit code
│   ├── button.ino
│   ├── secret.h
│   └── README.md
├── wiring-diagrams/        # Detailed wiring guides
│   └── diagrams.md
├── LICENSE
└── README.md              # This file
```

## secret.h

This file contains sensitive information, such as WiFi credentials and MAC addresses. It should not be committed to version control.

You will need to create 2 files called `secret.h` and `secret.h` in the `base-station` and `button` directories, respectively. These files should contain the following lines:

### base-station/secret.h
```cpp
#define WIFI_SSID "QuizBuzzer-Setup"
#define WIFI_PASSWORD "buzzer123"
```

### button/secret.h
```cpp
#define BASE_STATION_MAC {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
```

## 🚀 Quick Start

### 1. Flash the Base Station
1. Open `base-station/base-station.ino` in Arduino IDE
2. Select **Board: "ESP32C6 Dev Module"**
3. Install required libraries (see base-station/README.md)
4. Upload the code
5. Note the MAC address from Serial Monitor

### 2. Flash the Buttons
1. Open `button/button.ino` in Arduino IDE
2. Update `baseStationMAC` with your base station's MAC address
3. Upload to each button ESP32
4. Note each button's MAC address from Serial Monitor

### 3. Configure the System
1. Power on the base station
2. Connect to WiFi network: **QuizBuzzer-Setup** (password: `buzzer123`)
3. Navigate to **192.168.4.1** in your browser
4. Enter team names and button MAC addresses
5. Click "Save Configuration"

### 4. Start Playing!
1. Click "START GAME" on the web interface
2. Press any button to test
3. The first buzzer lights up and displays on the OLED
4. Click "RESET ROUND" to play again

## 📚 Documentation

- **[Base Station Setup](base-station/README.md)** - Detailed wiring, code setup, and troubleshooting
- **[Button Setup](button/README.md)** - Button assembly, battery management, and configuration
- **[Wiring Diagrams](wiring-diagrams/diagrams.md)** - Complete schematics for all components

## 🔧 Troubleshooting

### Display Not Working
- Check I2C wiring (SDA=GPIO6, SCL=GPIO7 for ESP32-C6)
- Verify display is powered by 3.3V (NOT 5V)
- Run I2C scanner to confirm address (usually 0x3C)

### Button Not Registering
- Verify button MAC address is correctly entered in base station config
- Check that game has been started via web interface
- Ensure button battery is charged

### ESP-NOW Connection Issues
- Make sure both base station and buttons use the same WiFi channel
- Check that base station MAC is correctly set in button code
- Verify all grounds are connected in button circuit

## 🤝 Contributing

Contributions are welcome! Please feel free to submit a Pull Request. For major changes, please open an issue first to discuss what you would like to change.

## 📝 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- Built on the ESP32 Arduino Core
- Uses Adafruit GFX and SSD1306 libraries for display
- Inspired by classic quiz show buzzer systems

## 📧 Contact

For questions or support, please open an issue on GitHub.

---

**Made with ❤️ for quiz enthusiasts, trivia nights, and game shows everywhere!**