# Base Station Setup Guide

The base station is the central hub of the quiz buzzer system. It receives button presses via ESP-NOW, displays the winner on an OLED screen, logs all events to an SD card, and provides a web interface for configuration.

## 📋 Hardware Components

| Component | Quantity | Notes |
|-----------|----------|-------|
| ESP32-C6-DevKitC-1 | 1 | Main microcontroller |
| 128x64 OLED Display (I2C) | 1 | SSD1306 chip, 0x3C address |
| MicroSD Card Module | 1 | SPI interface |
| MicroSD Card | 1 | Any size, formatted FAT32 |
| Momentary Push Buttons | 2 | For Start/Stop and Reset controls |
| USB Cable | 1 | For power (5V) |
| Jumper Wires | Various | For connections |

### Optional Components
| Component | Quantity | Notes |
|-----------|----------|-------|
| WS2812B LED Strip | 1 | For visual effects |
| Logic Level Converter | 1 | 3.3V to 5V, for LED strip |
| 330Ω Resistor | 1 | For LED data line |
| External 5V Power Supply | 1 | For LED strip |

## 🔌 Wiring Diagram

### OLED Display (I2C)
```
ESP32-C6 Pin    →    OLED Pin
GPIO6 (SDA)     →    SDA
GPIO7 (SCL)     →    SCL
3.3V            →    VCC
GND             →    GND
```

⚠️ **CRITICAL**: Use 3.3V, NOT 5V! Most OLED displays are damaged by 5V.

### SD Card Reader (SPI)
```
ESP32-C6 Pin    →    SD Card Pin
GPIO23 (MOSI)   →    MOSI/DI
GPIO19 (MISO)   →    MISO/DO
GPIO18 (SCK)    →    SCK/CLK
GPIO5 (CS)      →    CS
3.3V            →    VCC
GND             →    GND
```

### Physical Control Buttons
```
ESP32-C6 Pin    →    Button Connection
GPIO10          →    Start/Stop Button (one side)
GPIO1           →    Reset Button (one side)
GND             →    Both buttons (other side)
```

**Button Wiring Details:**
- Both buttons use internal pull-up resistors (no external resistor needed)
- Wire one side of each button to its respective GPIO pin
- Wire the other side of both buttons to any ESP32 GND pin
- Buttons are active LOW (pressed = LOW, released = HIGH)
- Built-in debouncing with 250ms delay

**Button Functions:**
- **Start/Stop (GPIO10)**: Toggles between game active/stopped states
- **Reset (GPIO1)**: Resets the current round (clears winner)

⚠️ **Note**: Avoid GPIO0 (boot mode), GPIO3 (JTAG), and GPIO9 (boot strapping) for buttons as they have special functions on ESP32 boards.

### WS2812B LED Strip (Optional)
```
Component                      →    Connection
External 5V Supply (+)         →    LED Strip 5V
External 5V Supply (-)         →    LED Strip GND
ESP32 GPIO4                    →    Logic Level Converter (Low Side Input)
Logic Level Converter Output   →    330Ω Resistor → LED Strip Data In
ESP32 GND                      →    External Supply GND (COMMON GROUND!)
```

### Power
```
USB 5V → ESP32 USB-C Port
```

## 📚 Required Arduino Libraries

### base-station/base-secret.h
```cpp
#define WIFI_SSID "QuizBuzzer-Setup"
#define WIFI_PASSWORD "buzzer123"
```

Install these libraries via Arduino IDE Library Manager:

1. **ESP32 Board Support**
   - Go to File → Preferences
   - Add to Additional Board Manager URLs:
     ```
     https://espressif.github.io/arduino-esp32/package_esp32_index.json
     ```
   - Tools → Board → Board Manager → Search "esp32" → Install

2. **Adafruit GFX Library** (by Adafruit)
3. **Adafruit SSD1306** (by Adafruit)
4. **Adafruit BusIO** (dependency, should auto-install)

Built-in libraries (no installation needed):
- WiFi
- WebServer
- esp_now
- FS
- SD
- SPI
- Wire

## ⚙️ Board Settings

In Arduino IDE, select:
- **Board**: "ESP32C6 Dev Module"
- **Upload Speed**: 921600
- **Flash Size**: "4MB (32Mb)"
- **Partition Scheme**: "Default 4MB with spiffs"
- **Port**: Select your USB port

## 🚀 Installation Steps

### 1. Flash the Base Station Code

1. Open `base-station.ino` in Arduino IDE
2. Select the correct board settings (above)
3. Click **Upload**
4. Open Serial Monitor (115200 baud)
5. **IMPORTANT**: Copy and save the MAC address shown (format: `XX:XX:XX:XX:XX:XX`)

Example output:
```
Base Station MAC Address: AA:BB:CC:DD:EE:FF
```

### 2. Prepare the SD Card

1. Format a microSD card as **FAT32**
2. Insert into the SD card module
3. The system will automatically create these files:
   - `config.txt` - Team names and button MAC addresses
   - `buzzer_log.txt` - Event log with timestamps

### 3. Initial Configuration

1. Power on the base station
2. The OLED should display:
   ```
   Quiz Buzzer System

   WiFi: QuizBuzzer-Setup
   IP: 192.168.4.1

   Teams (9):
   1. Team 1
   2. Team 2
   3. Team 3
   ...
   ```

3. Connect to WiFi:
   - **SSID**: QuizBuzzer-Setup
   - **Password**: buzzer123

4. Open browser and navigate to: **http://192.168.4.1**

5. Configure teams:
   - Set number of teams (1-9)
   - Enter team names
   - Enter each button's MAC address (you'll get these when you flash the buttons)
   - Click "Save Configuration"

6. **Configuration saved!** On next power-up, the display will show:
   ```
   Quiz Buzzer System
   [Config Loaded]

   WiFi: QuizBuzzer-Setup
   IP: 192.168.4.1

   Teams (9):
   1. Team Alpha
   2. Team Beta
   3. Team Gamma
   4. Team Delta
   ...
   ```

## 🎮 Operating the System

You can control the base station using **either** the web interface or the physical buttons.

### Web Interface Controls

The web interface at `192.168.4.1` provides these controls:

- **START GAME** - Activates buzzer system, OLED shows "READY!"
- **RESET ROUND** - Clears current winner, ready for next question
- **STOP GAME** - Deactivates buzzer system
- **Mute Toggle** - Each team has individual mute control to silence their button's audio feedback
- **Save Configuration** - Saves team names, MAC addresses, and mute settings to SD card

### Physical Button Controls

Two physical buttons are wired to the base station for quick game control:

- **Start/Stop Button (GPIO10)** - Toggles game between active and stopped states
  - Press when stopped → Starts the game
  - Press when active → Stops the game
- **Reset Button (GPIO1)** - Resets the current round
  - Clears the winner
  - Resets response timer
  - Only sends updates when game is active

Both control methods work identically and can be used interchangeably during gameplay.

### Game Flow

1. **Start the game** (web or physical button)
2. OLED displays "READY!" in large text
3. First button press:
   - Button LED stays on
   - OLED displays "WINNER: [Team Name]"
   - Other buttons are locked out (LEDs off)
4. **Reset round** to clear and play again (web or physical button)
5. **Stop game** when finished (web or physical button)

## 📊 Data Logging

### Buzzer Log Format
File: `buzzer_log.txt`
```
12543,Team Alpha
15892,Team Beta
23401,Team Alpha
```
Format: `timestamp_ms,team_name`

### Configuration File Format
File: `config.txt`

The system automatically saves and loads your configuration on every boot. When you save settings via the web interface, they're written to `config.txt`:

```
9
Team Alpha
A4,B2,31,F0,12,34
1
0
Team Beta
A4,B2,31,F0,12,35
1
0
Team Gamma
A4,B2,31,F0,12,36
1
0
...
```

Format per team:
- Line 1: Team name
- Line 2: MAC address (comma-separated hex bytes)
- Line 3: Configuration status (1 = configured, 0 = not configured)
- Line 4: Mute status (1 = muted, 0 = not muted)

**Auto-Load Feature:**
- Configuration is automatically loaded on every power-up
- Team names and MAC addresses are restored
- Display shows `[Config Loaded]` when successful
- If no SD card or config file exists, defaults are used (Team 1, Team 2, etc.)

**Benefits:**
- No need to reconfigure after power cycles
- Portable - move system between locations without losing setup
- Backup your config by copying `config.txt` from the SD card

## 🔧 Troubleshooting

### OLED Display Issues

**Symptom**: Blank display
- **Check I2C wiring**: Verify SDA→GPIO6, SCL→GPIO7
- **Check voltage**: Must be 3.3V, not 5V
- **Run I2C Scanner**: Use the scanner code from main README
- **Try different address**: Change `0x3C` to `0x3D` in code (line 296)

**Symptom**: Display shows garbage/random pixels
- **Check power**: Ensure stable 3.3V supply
- **Check wiring**: Loose connections cause corruption

### SD Card Issues

**Symptom**: "SD Card failed" in Serial Monitor
- **Format**: Must be FAT32 (not exFAT or NTFS)
- **Check wiring**: Verify SPI connections
- **Try different card**: Some cards are incompatible
- **Check CS pin**: Should be GPIO5

**Symptom**: Configuration doesn't persist
- SD card may not be inserted or formatted correctly
- System will work without SD card, but won't save settings
- Check Serial Monitor for "Configuration file loaded successfully" message
- Verify `config.txt` exists on SD card after saving

**Symptom**: Display shows default team names instead of saved names
- Config file may be corrupted
- Try removing and reinserting SD card
- Check Serial Monitor for load errors
- Manually verify `config.txt` format on SD card
- Re-save configuration via web interface

### WiFi/Web Interface Issues

**Symptom**: Can't connect to QuizBuzzer-Setup
- **Restart ESP32**: Unplug and replug USB
- **Check Serial Monitor**: Verify AP started successfully
- **Try different device**: Some phones/laptops have issues with ESP32 APs

**Symptom**: Can't access 192.168.4.1
- **Verify WiFi connection**: Must be connected to QuizBuzzer-Setup
- **Try http://192.168.4.1**: Don't use https://
- **Clear browser cache**: Sometimes helps

### Button Communication Issues

**Symptom**: Buttons not registering
- **Check MAC addresses**: Must match exactly in config
- **Start the game**: Buttons won't work unless game is started
- **Check Serial Monitor**: Look for ESP-NOW errors
- **Restart both devices**: Sometimes ESP-NOW needs a reset

## 🔐 Security Note

The default WiFi password is `buzzer123`. For public events, consider changing it in the code (line 18):

```cpp
const char* ap_password = "your_secure_password_here";
```

## 📝 Customization

### Change Number of Teams
Default is now 9 teams. To change this, edit `MAX_TEAMS` in the code (base-station.ino:54):
```cpp
#define MAX_TEAMS 9  // Change to any value 1-9
```
You don't need to use all 9 slots - simply configure only the teams you need via the web interface.

### Change WiFi SSID
Edit line 17:
```cpp
const char* ap_ssid = "YourCustomName";
```

### Adjust OLED Brightness
The SSD1306 has built-in brightness control. Add this after display initialization:
```cpp
display.ssd1306_command(SSD1306_SETCONTRAST);
display.ssd1306_command(128); // 0-255, default is 207
```

## 💾 Code Structure

Key functions:
- `setup()` - Initializes hardware, WiFi, and loads saved configuration
- `loop()` - Handles web server
- `onDataRecv()` - ESP-NOW callback for button presses
- `updateDisplay()` - Updates OLED with current state and team names
- `handleRoot()` - Generates web interface HTML
- `saveConfig()` - Writes configuration to SD card
- `loadConfig()` - Automatically loads configuration on boot

## 🆘 Getting Help

If you encounter issues:
1. Check the Serial Monitor output for error messages
2. Verify all wiring matches the diagrams exactly
3. Test components individually (I2C scanner, SD card test, etc.)
4. Open an issue on GitHub with Serial Monitor output

---

**Next Step**: Set up your button units! See [button/README.md](../button/README.md)