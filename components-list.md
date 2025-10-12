# Quiz Buzzer System - Wiring Diagrams & Components

## BASE STATION (ESP32 #1)

### Components Needed:
- 1x ESP32 DevKit board
- 1x 128x64 OLED Display (I2C - SSD1306)
- 1x TF/SD Card Reader Module (SPI)
- 1x WS2812B LED Strip (optional for visual effects)
- 1x USB Cable for power
- Jumper wires

### Base Station Wiring:

```
ESP32 BASE STATION CONNECTIONS
================================

OLED Display (I2C):
  ESP32 GPIO21 (SDA) → OLED SDA
  ESP32 GPIO22 (SCL) → OLED SCL
  ESP32 3.3V → OLED VCC
  ESP32 GND → OLED GND

SD Card Reader (SPI):
  ESP32 GPIO23 (MOSI) → SD Card MOSI/DI
  ESP32 GPIO19 (MISO) → SD Card MISO/DO
  ESP32 GPIO18 (SCK) → SD Card SCK/CLK
  ESP32 GPIO5 (CS) → SD Card CS
  ESP32 3.3V → SD Card VCC
  ESP32 GND → SD Card GND

WS2812B LED Strip (Optional):
  ESP32 GPIO4 → LED Strip Data In (via 330Ω resistor)
  External 5V Supply → LED Strip 5V
  External 5V Supply GND → LED Strip GND
  ESP32 GND → External 5V Supply GND (COMMON GROUND!)

Power:
  USB 5V → ESP32 VIN or Micro-USB port
```

### Base Station Pin Summary:
| Function | ESP32 Pin |
|----------|-----------|
| OLED SDA | GPIO21 |
| OLED SCL | GPIO22 |
| SD MOSI | GPIO23 |
| SD MISO | GPIO19 |
| SD SCK | GPIO18 |
| SD CS | GPIO5 |
| LED Strip | GPIO4 |
| Power | USB 5V |

---

## BATTERY BUTTON (ESP32 #2-10) - ONE PER TEAM

### Components Needed (per button):
- 1x ESP32 DevKit board
- 1x Arcade Button (60mm LED dome button)
- 1x 18650 Li-ion Battery (3.7V)
- 1x TP4056 Battery Charging Module
- 1x MT3608 DC-DC Boost Converter (3.7V → 12V)
- 1x 10kΩ Resistor
- 1x Toggle Switch (power switch)
- Jumper wires
- Battery holder

### Button Unit Wiring:

```
BATTERY BUTTON UNIT CONNECTIONS
=================================

Arcade Button Connection:
  Button NO (Normally Open) → ESP32 3.3V
  Button COM (Common) → ESP32 GPIO15
  10kΩ Resistor → Between GPIO15 and GND
  
Button LED Power (12V from boost converter):
  Boost Converter OUT+ → Button LED + (via MOSFET/transistor)
  ESP32 GPIO4 → MOSFET Gate (LED control)
  MOSFET Drain → Button LED -
  MOSFET Source → Boost Converter GND

Power System:
  18650 Battery + → TP4056 BAT+
  18650 Battery - → TP4056 BAT-
  
  TP4056 OUT+ → Power Switch → Boost Converter IN+
  TP4056 OUT- → Boost Converter IN-
  
  Boost Converter OUT+ (set to 5V) → ESP32 VIN
  Boost Converter GND → ESP32 GND
  
  Second Boost Converter (for LED):
  Boost Converter IN+ → TP4056 OUT+ (after switch)
  Boost Converter IN- → TP4056 OUT-
  Boost Converter OUT+ (set to 12V) → Button LED circuit
  
Charging:
  USB 5V → TP4056 IN+
  USB GND → TP4056 IN-

Common Ground:
  ALL GROUNDS MUST BE CONNECTED TOGETHER!
  - 18650 Battery -
  - TP4056 GND
  - Boost Converter GND
  - ESP32 GND
  - Button circuit GND
```

### Button Unit Pin Summary:
| Function | ESP32 Pin |
|----------|-----------|
| Button Input | GPIO15 |
| LED Control | GPIO4 |
| Boost Enable | GPIO2 (optional) |
| Power | VIN (5V from boost) |
| Ground | GND (all connected) |

---

## SIMPLIFIED BUTTON WIRING (Using Built-in LED)

If your arcade button has a built-in LED that runs on 5V or 12V:

```
SIMPLE BUTTON LED CONTROL
==========================

Option 1: Direct 5V LED (if button LED is 5V):
  Boost Converter OUT (5V) → Transistor Collector
  Transistor Emitter → Button LED +
  Button LED - → GND
  ESP32 GPIO4 → Transistor Base (via 1kΩ resistor)

Option 2: 12V LED (if button LED is 12V):
  Use separate MT3608 boost converter
  Adjust output to 12V
  Connect as above with transistor control
```

---

## BOOST CONVERTER SETUP

### For ESP32 Power (3.7V → 5V):
1. Connect multimeter to output
2. Adjust potentiometer to 5.0V
3. Mark with label "ESP32 - 5V"

### For Button LED (3.7V → 12V):
1. Connect multimeter to output
2. Adjust potentiometer to 12V (or voltage required by your LED)
3. Mark with label "LED - 12V"

---

## ASSEMBLY TIPS

### Base Station:
1. Flash code first before assembling
2. Test OLED display independently
3. Test SD card module independently
4. Connect all grounds together
5. Power from USB cable or 5V wall adapter

### Button Units:
1. Flash code to ESP32 first
2. Note the MAC address from Serial Monitor
3. Test button input with serial monitor
4. Adjust boost converters to correct voltages
5. Install TP4056 charging port accessible from outside
6. Add power switch between battery and ESP32
7. Test battery life (should last several hours)

---

## INITIAL SETUP PROCEDURE

1. **Flash Base Station:**
   - Upload base station code
   - Note the base station MAC address from Serial Monitor
   - Insert SD card (formatted FAT32)

2. **Flash All Buttons:**
   - Upload button code to each ESP32
   - Note each button's MAC address
   - Update the `baseStationMAC` array in button code with base station's MAC

3. **Configure System:**
   - Power on base station
   - Connect phone/laptop to "QuizBuzzer-Setup" WiFi
   - Password: "buzzer123"
   - Navigate to 192.168.4.1
   - Enter team names
   - Enter each button's MAC address for corresponding team
   - Save configuration

4. **Test:**
   - Click "START GAME" on web interface
   - Press each button to verify it registers
   - Check OLED display shows correct winner
   - Check SD card logs buzzes

---

## TROUBLESHOOTING

**Button not registering:**
- Check MAC address is correctly entered in base station config
- Verify ESP-NOW is initialized on both devices
- Check common ground connection

**LED not lighting:**
- Verify boost converter voltage with multimeter
- Check transistor connections
- Ensure GPIO4 goes HIGH when button pressed

**Display not working:**
- Check I2C address (usually 0x3C)
- Verify SDA/SCL connections
- Try I2C scanner sketch

**SD card not working:**
- Format card as FAT32
- Check SPI wiring
- Verify CS pin matches code (GPIO5)

**Battery drains quickly:**
- Implement deep sleep mode (code included)
- Check for short circuits
- Reduce LED brightness/duration