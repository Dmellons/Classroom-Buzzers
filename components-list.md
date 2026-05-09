# Quiz Buzzer System - Complete Components List & Wiring

## System Overview

This is an ESP32-C6 based quiz buzzer system with:
- **Base Station**: Central hub with OLED display, SD logging, WiFi config, and physical controls
- **Button Units**: Battery-powered wireless buzzers with OLED display, speaker, and battery monitoring
- **Communication**: ESP-NOW protocol for ultra-low latency (<10ms)

### System Capabilities
- Support for 1-9 wireless button units
- Millisecond-precision response time measurement
- Automatic button discovery (no manual MAC entry required)
- Persistent configuration via SD card
- Individual team mute control
- Configurable countdown timer (1-5 seconds)
- Web interface for easy configuration
- Physical buttons for game control
- Battery monitoring with percentage display
- Audio feedback for all game states

---

## BASE STATION COMPONENTS

### Required Hardware

| Component | Quantity | Specifications | Notes |
|-----------|----------|----------------|-------|
| ESP32-C6-DevKitC-1 | 1 | Espressif ESP32-C6 development board | Main microcontroller |
| 128x64 OLED Display | 1 | I2C SSD1306, 0x3C address | Status display |
| MicroSD Card Module | 1 | SPI interface | For logging and config |
| MicroSD Card | 1 | Any size, FAT32 formatted | Data storage |
| Momentary Push Button | 2 | SPST, normally open | Start/Stop and Reset controls |
| USB Cable (USB-C) | 1 | For power (5V 1A minimum) | Powers entire system |
| Jumper Wires | Various | 22-24 AWG | Connections |

### Optional Hardware

| Component | Quantity | Specifications | Notes |
|-----------|----------|----------------|-------|
| WS2812B LED Strip | 1 | Addressable RGB LED strip | Visual effects |
| Logic Level Converter | 1 | 3.3V to 5V bi-directional | For LED strip data line |
| 330Ω Resistor | 1 | 1/4W | LED data line protection |
| External 5V Power Supply | 1 | 2A or higher | For LED strip |

### Base Station Pin Assignments (ESP32-C6)

| Function | GPIO Pin | Wire Color Suggestion | Notes |
|----------|----------|----------------------|-------|
| **OLED SDA** | GPIO6 | Blue | I2C data line |
| **OLED SCL** | GPIO7 | Yellow | I2C clock line |
| **SD MOSI** | GPIO23 | Orange | SPI data out |
| **SD MISO** | GPIO19 | Purple | SPI data in |
| **SD SCK** | GPIO18 | Green | SPI clock |
| **SD CS** | GPIO5 | White | SPI chip select |
| **Start/Stop Button** | GPIO10 | Brown | Uses internal pull-up |
| **Reset Button** | GPIO1 | Red | Uses internal pull-up |
| **LED Strip (Optional)** | GPIO4 | - | Via level converter |
| **Power (5V)** | USB-C | Red | Powers entire base |
| **Ground** | GND | Black | Common ground |
| **3.3V** | 3V3 | Red | For OLED and SD module |

### Base Station Wiring Diagram

```
ESP32-C6 BASE STATION CONNECTIONS
==================================

OLED Display (128x64 I2C):
  ESP32 GPIO6 (SDA) ──────────── OLED SDA
  ESP32 GPIO7 (SCL) ──────────── OLED SCL
  ESP32 3.3V ─────────────────── OLED VCC (⚠️ NOT 5V!)
  ESP32 GND ──────────────────── OLED GND

SD Card Reader (SPI):
  ESP32 GPIO23 (MOSI) ─────────── SD Card MOSI/DI
  ESP32 GPIO19 (MISO) ─────────── SD Card MISO/DO
  ESP32 GPIO18 (SCK) ──────────── SD Card SCK/CLK
  ESP32 GPIO5 (CS) ────────────── SD Card CS
  ESP32 3.3V ──────────────────── SD Card VCC
  ESP32 GND ───────────────────── SD Card GND

Physical Control Buttons:
  Button 1 (Start/Stop):
    ESP32 GPIO10 ────────────── One side of button
    ESP32 GND ───────────────── Other side of button
    (Uses internal pull-up resistor)

  Button 2 (Reset):
    ESP32 GPIO1 ─────────────── One side of button
    ESP32 GND ───────────────── Other side of button
    (Uses internal pull-up resistor)

WS2812B LED Strip (Optional):
  External 5V Supply (+) ──────── LED Strip 5V
  External 5V Supply (-) ──────── LED Strip GND
  ESP32 GPIO4 ──────────────────── Logic Level Converter (Low Side)
  Logic Level Converter (High) ──[330Ω]── LED Strip Data In
  ESP32 GND ───────────────────── External 5V Supply GND (COMMON GROUND!)

Power:
  USB-C 5V ──────────────────── ESP32 USB-C Port
```

### Critical Notes for Base Station
- ⚠️ **OLED MUST use 3.3V** - 5V will damage most OLED displays
- ⚠️ **SD Card MUST be FAT32** - exFAT and NTFS are not supported
- ⚠️ **Buttons use internal pull-ups** - no external resistors needed
- ⚠️ **Common ground required** - all GND pins must connect together
- 💡 GPIO10 and GPIO1 chosen to avoid boot strapping pins (GPIO0, GPIO9)

---

## BUTTON UNIT COMPONENTS (Per Button)

### Required Hardware

| Component | Quantity | Specifications | Estimated Cost |
|-----------|----------|----------------|----------------|
| ESP32-C6-DevKitC-1 | 1 | Espressif ESP32-C6 development board | $15 |
| 0.91" OLED Display | 1 | 128x32 I2C SSD1306, 0x3C address | $8 |
| 3W Speaker | 1 | 3W 8Ω speaker for audio feedback | $6 |
| 60mm Arcade Button | 1 | Large dome button with microswitch | $12 |
| LiPo Battery | 1 | 503035 3.7V 500mAh (or 18650 for longer runtime) | $5-10 |
| TP4056 Module | 1 | Battery charging module with protection | $3 |
| MT3608 Boost Converter | 1 | DC-DC step-up (3.7V → 5V) for ESP32 | $2 |
| Toggle Switch | 1 | SPST, 3A rated, for main power | $2 |
| Power Indicator LED | 1 | 3mm or 5mm, any color | $0.50 |
| 10kΩ Resistor | 1 | 1/4W, button pull-down | $0.10 |
| 1kΩ Resistor | 1 | 1/4W, for power LED | $0.10 |
| 47kΩ Resistor | 2 | 1/4W, battery voltage divider | $0.20 |
| Jumper Wires | Various | 22-24 AWG stranded, various colors | $5 |
| Enclosure | 1 | Plastic project box or 3D printed | $10 |
| **Total (Required)** | | | **~$69** |

### Optional Hardware (Arcade Button LED Illumination)

| Component | Quantity | Specifications | Estimated Cost |
|-----------|----------|----------------|----------------|
| MT3608 Boost Converter | 1 | DC-DC step-up (3.7V → 12V) for LED | $2 |
| 2N2222 NPN Transistor | 1 | Or any similar NPN switching transistor | $0.50 |
| 1kΩ Resistor | 1 | 1/4W, transistor base | $0.10 |
| 330Ω Resistor | 1 | 1/4W, LED current limiting | $0.10 |
| **Total (Optional LED)** | | | **~$3** |

### Button Unit Pin Assignments (ESP32-C6)

| Function | GPIO Pin | Wire Color Suggestion | Notes |
|----------|----------|----------------------|-------|
| **OLED SDA** | GPIO6 | Blue | I2C data line |
| **OLED SCL** | GPIO7 | Yellow | I2C clock line |
| **Button Input** | GPIO15 | Green | With 10kΩ pull-down |
| **Status LED** | GPIO2 | - | Built-in LED (onboard) |
| **Speaker** | GPIO8 | Purple | PWM audio output |
| **Battery Monitor** | GPIO1 | White | ADC voltage sensing |
| **LED Control** | GPIO4 | Orange | Optional arcade LED control |
| **Power (5V)** | VIN | Red | From boost converter |
| **Ground** | GND | Black | Common ground |
| **3.3V** | 3V3 | Red | For OLED |

### Button Unit Wiring Diagram

```
ESP32-C6 BUTTON UNIT CONNECTIONS
=================================

OLED Display (128x32 I2C):
  ESP32 GPIO6 (SDA) ──────────── OLED SDA
  ESP32 GPIO7 (SCL) ──────────── OLED SCL
  ESP32 3.3V ─────────────────── OLED VCC (⚠️ NOT 5V!)
  ESP32 GND ──────────────────── OLED GND

Speaker (3W 8Ω):
  ESP32 GPIO8 ────────────────── Speaker + (Positive)
  ESP32 GND ──────────────────── Speaker - (Negative)
  Note: Direct connection OK, ESP32 PWM can drive 3W speaker

Arcade Button Input:
  Button NO (Normally Open) ──── ESP32 3.3V
  Button COM (Common) ────────── ESP32 GPIO15
  10kΩ Resistor ──────────────── Between GPIO15 and GND (pull-down)

Battery Voltage Monitoring:
  TP4056 OUT+ ─[47kΩ]─┬─[47kΩ]─ GND
                       │
                  ESP32 GPIO1 (ADC)
  Note: Voltage divider reads half battery voltage

Status LED:
  ESP32 GPIO2 ────────────────── Built-in LED (onboard, no wiring needed)

Power System:
  503035 LiPo Battery (+) ────── TP4056 BAT+
  503035 LiPo Battery (-) ────── TP4056 BAT-

  TP4056 OUT+ ──┬── Toggle Switch ──┬── MT3608 Boost IN+
                │                    │
                └──[1kΩ]── Power LED (+)

  TP4056 OUT- ─────────────────────── MT3608 Boost IN-
  Power LED (-) ───────────────────── GND

  MT3608 Boost OUT+ (5V) ────────── ESP32 VIN
  MT3608 Boost OUT- (GND) ───────── ESP32 GND

Charging:
  USB 5V ──────────────────────── TP4056 IN+
  USB GND ─────────────────────── TP4056 IN-

Common Ground Network:
  ████ ALL THESE MUST BE CONNECTED ████
  - LiPo Battery (-)
  - TP4056 OUT- and GND
  - Boost Converter GND
  - ESP32 GND
  - OLED GND
  - Speaker (-)
  - Button circuit GND
  - Power LED (-)
  ████████████████████████████████████

Optional - Arcade Button LED Illumination:
  Second MT3608 Boost:
    IN+ ────────────────────────── TP4056 OUT+ (after switch)
    IN- ────────────────────────── TP4056 OUT-
    OUT+ (12V) ─────────────────── 2N2222 Collector

  LED Control Circuit:
    ESP32 GPIO4 ──[1kΩ]── 2N2222 Base
    2N2222 Emitter ──[330Ω]─── Button LED (+)
    Button LED (-) ──────────── GND
```

### Critical Notes for Button Units
- ⚠️ **OLED MUST use 3.3V** - 5V will damage the display
- ⚠️ **Battery monitoring uses GPIO1** - NOT GPIO0 (boot strapping pin causes issues)
- ⚠️ **Boost converter MUST be adjusted to 5.0V** before connecting to ESP32
- ⚠️ **Common ground critical** - all GND points must connect together
- ⚠️ **Button uses GPIO15** - confirmed working with external 10kΩ pull-down
- 💡 Speaker connects directly to GPIO8 - no amplifier needed for 3W 8Ω speaker
- 💡 GPIO1-7 are ADC-capable on ESP32-C6; GPIO18-23 are NOT ADC-capable
- 💡 Arcade button LED illumination is optional - core functionality works without it

---

## BOOST CONVERTER SETUP

### For Button Units (REQUIRED - ESP32 Power)
1. Connect boost converter input to 3.7V source (battery or bench supply)
2. Connect multimeter to output terminals
3. Adjust potentiometer slowly until output reads **exactly 5.0V**
4. Label converter "ESP32 - 5V"
5. Disconnect and install in circuit

### For Arcade Button LED (OPTIONAL - LED Illumination Only)
1. Connect boost converter input to 3.7V source
2. Connect multimeter to output terminals
3. Adjust potentiometer until output reads **12V** (or 5V if LED is 5V rated)
4. Label converter "LED - 12V"
5. Disconnect and install in circuit

**⚠️ WARNING**: Always set boost converter voltage BEFORE connecting to ESP32 or LED! Overvoltage can damage components.

---

## SYSTEM FEATURES

### Base Station Features
✅ 128x64 OLED display showing team names and game status
✅ SD card logging with auto-save configuration
✅ WiFi access point (192.168.4.1) for web configuration
✅ Physical buttons for game control (Start/Stop, Reset)
✅ ESP-NOW wireless communication (ultra-low latency)
✅ Individual team mute control via web interface
✅ Supports 1-9 teams (configurable)
✅ Persistent configuration (survives power cycles)

### Button Unit Features
✅ 128x32 OLED display with team name and battery level
✅ Audio feedback with 3W speaker (buzzer, victory, lockout sounds)
✅ Battery voltage monitoring and percentage display
✅ Automatic team name synchronization via heartbeat
✅ Visual status LED patterns for each game state
✅ Individual mute capability (controlled from base station)
✅ LiPo battery powered with USB charging
✅ 2-4 hour runtime (typical with 500mAh battery)
✅ Hardware interrupt button detection for instant response
✅ Built-in debouncing (50ms)

---

## ARDUINO LIBRARIES REQUIRED

### For All Devices (Base Station + Buttons)
1. **ESP32 Board Support**
   - File → Preferences → Additional Board Manager URLs:
   - `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
   - Tools → Board → Board Manager → Search "esp32" → Install

2. **Adafruit GFX Library** (by Adafruit)
3. **Adafruit SSD1306** (by Adafruit)
4. **Adafruit BusIO** (dependency, usually auto-installs)

### Built-in Libraries (No Installation Needed)
- WiFi
- WebServer (base station only)
- esp_now
- Wire (I2C)
- FS (base station only)
- SD (base station only)
- SPI (base station only)

---

## BOARD SETTINGS (Arduino IDE)

**For Both Base Station and Button Units:**
- **Board**: "ESP32C6 Dev Module"
- **Upload Speed**: 921600
- **Flash Size**: "4MB (32Mb)"
- **Partition Scheme**: "Default 4MB with spiffs"
- **Port**: Select your USB port

---

## ASSEMBLY TIPS

### Base Station Assembly Order
1. Flash code to ESP32 first (note MAC address from Serial Monitor)
2. Test OLED display independently (I2C scanner if needed)
3. Test SD card module independently (format as FAT32)
4. Wire physical buttons to GPIO10 and GPIO1 (connect other side to GND)
5. Connect all components following wiring diagram
6. Verify all grounds are connected together
7. Power via USB-C cable or 5V wall adapter
8. Insert FAT32-formatted SD card
9. Access web interface at 192.168.4.1 to configure teams

### Button Unit Assembly Order
1. Flash code to ESP32 first (note MAC address from Serial Monitor)
2. **CRITICAL**: Adjust boost converter to 5.0V BEFORE connecting to ESP32
3. Test OLED display independently
4. Test speaker with startup sound
5. Wire battery voltage divider to GPIO1 (NOT GPIO0!)
6. Wire button to GPIO15 with 10kΩ pull-down to GND
7. Assemble power system (TP4056 → switch → boost → ESP32)
8. Connect all components following wiring diagram
9. Verify all grounds are connected together (critical!)
10. Test battery charging (TP4056 red LED = charging, blue/green = full)
11. Configure button MAC address in base station web interface
12. Test communication (OLED should show team name)

---

## TROUBLESHOOTING QUICK REFERENCE

### Base Station
- **OLED blank**: Check I2C wiring (GPIO6/7), verify 3.3V power
- **SD card fail**: Must be FAT32, check SPI wiring (GPIO5/18/19/23)
- **WiFi won't start**: Check Serial Monitor for errors, restart ESP32
- **Buttons not working**: Verify MAC addresses match in config exactly

### Button Units
- **OLED blank**: Check I2C wiring (GPIO6/7), verify 3.3V power (NOT 5V!)
- **No sound**: Check speaker wiring to GPIO8, verify team not muted
- **Wrong battery %**: Check voltage divider on GPIO1 (NOT GPIO0!)
- **Button not registering**: Check GPIO15 wiring, verify 10kΩ pull-down
- **No heartbeat (HB:X)**: Check base station MAC in button-secret.h
- **Won't power on**: Verify boost converter set to 5.0V, check battery voltage

---

## ESTIMATED TOTAL COST

### For Complete System (1 Base Station + 4 Button Units)

| Item | Cost |
|------|------|
| Base Station Hardware | ~$45 |
| Button Unit x4 (required components) | ~$276 ($69 each) |
| **Total System** | **~$321** |

**Per Additional Button**: ~$69

**Note**: Costs are estimates based on typical online prices. Bulk ordering can reduce costs significantly.

---

This wiring guide reflects the current implementation as of the latest code updates. All pin assignments, features, and wiring diagrams are accurate to the actual working system.
