# Button Unit Setup Guide

Each button unit is a self-contained wireless buzzer powered by a LiPo battery. It communicates with the base station using ESP-NOW and provides:
- **OLED Display** showing team name, battery level, and game status
- **Audio Feedback** with buzzer sounds, victory tunes, and lockout tones
- **Visual Feedback** through built-in status LED and optional arcade button LED
- **Battery Monitoring** with real-time voltage and percentage display
- **Team Name Sync** via automatic heartbeat system

## 📋 Hardware Components (Per Button)

### Required Components

| Component | Quantity | Specifications |
|-----------|----------|----------------|
| ESP32-C6-DevKitC-1 | 1 | Main microcontroller |
| 0.91" OLED Display | 1 | 128x32 I2C SSD1306 |
| 3W Speaker | 1 | 3W 8Ω speaker for audio feedback |
| 60mm LED Arcade Button | 1 | Large arcade button with microswitch |
| 503035 LiPo Battery | 1 | 3.7V 500mAh (or 18650 for longer runtime) |
| TP4056 Charging Module | 1 | With protection circuit |
| MT3608 Boost Converter | 1 | For ESP32 (3.7V→5V) |
| 10kΩ Resistor | 1 | For button pull-down |
| 1kΩ Resistor | 1 | For power LED |
| 47kΩ Resistor | 2 | For battery voltage divider |
| Power Indicator LED | 1 | 3mm or 5mm, any color |
| Toggle Switch | 1 | SPST, for main power |
| Jumper Wires | Various | 22-24 AWG recommended |

### Optional Components (for Arcade Button LED Illumination)

| Component | Quantity | Specifications |
|-----------|----------|----------------|
| MT3608 Boost Converter | 1 | For LED (3.7V→12V) |
| 2N2222 NPN Transistor | 1 | Or any similar NPN transistor |
| 1kΩ Resistor | 1 | For transistor base |
| 330Ω Resistor | 1 | LED current limiting |

## 🔌 Wiring Diagram

### Complete Pin Assignments

| Function | ESP32-C6 Pin | Notes |
|----------|--------------|-------|
| OLED SDA | GPIO6 | I2C data |
| OLED SCL | GPIO7 | I2C clock |
| Button Input | GPIO15 | With 10kΩ pull-down |
| Status LED | GPIO2 | Built-in LED |
| Speaker | GPIO8 | PWM audio output |
| Battery Monitor | GPIO1 | ADC voltage sensing |
| LED Control | GPIO4 | Optional arcade button LED |
| Power (5V) | VIN | From boost converter |
| Ground | GND | Common ground |

### Power System
```
503035 LiPo Battery (+) → TP4056 BAT+
503035 LiPo Battery (-) → TP4056 BAT-

TP4056 OUT+ → Toggle Switch → Boost Converter IN+
TP4056 OUT- → Boost Converter IN-

Boost Converter (5V for ESP32):
  OUT+ (adjusted to 5V) → ESP32 VIN
  OUT- (GND) → ESP32 GND

Power Indicator LED:
  TP4056 OUT+ (after switch) → 1kΩ Resistor → Power LED (+)
  Power LED (-) → Common Ground

Battery Voltage Monitor:
  TP4056 OUT+ → 47kΩ → ESP32 GPIO1 → 47kΩ → GND
  (Voltage divider for battery level sensing)
```

⚠️ **CRITICAL**: All grounds must be connected together!
⚠️ **NOTE**: Use GPIO1 for battery monitoring, NOT GPIO0 (boot strapping pin)

### OLED Display (I2C)
```
ESP32 GPIO6 (SDA) → OLED SDA
ESP32 GPIO7 (SCL) → OLED SCL
ESP32 3.3V → OLED VCC
ESP32 GND → OLED GND
```

⚠️ **CRITICAL**: Use 3.3V for OLED, NOT 5V!

### Speaker (3W 8Ω Audio)
```
ESP32 GPIO8 → Speaker + (Positive)
ESP32 GND → Speaker - (Negative)
```

No amplifier needed - ESP32 can drive 3W speaker directly via PWM.

### Button Input Circuit
```
Arcade Button:
  NO (Normally Open) → ESP32 3.3V
  COM (Common) → ESP32 GPIO15

10kΩ Pull-down Resistor:
  One end → ESP32 GPIO15
  Other end → ESP32 GND
```

The button uses internal pull-down configuration with external 10kΩ resistor for reliability.

### LED Control Circuit (Optional - for Arcade Button Illumination)
```
Boost Converter #2 (12V for LED):
  IN+ → TP4056 OUT+ (after switch)
  IN- → TP4056 OUT-
  OUT+ (adjusted to 12V) → 2N2222 Collector

ESP32 GPIO4 → 1kΩ Resistor → 2N2222 Base
2N2222 Emitter → 330Ω Resistor → Button LED (+)
Button LED (-) → GND
```

### Charging Port
```
USB 5V+ → TP4056 IN+
USB GND → TP4056 IN-
```

💡 **Tip**: Mount the TP4056 module so the micro-USB port is accessible from outside your enclosure for easy charging.

## ⚙️ Boost Converter Setup

**BEFORE connecting to ESP32 or LED, adjust the boost converter(s)!**

### Boost Converter (ESP32 Power) - REQUIRED
1. Connect input to a 3.7V source (or the battery)
2. Connect a multimeter to the output
3. Use a small screwdriver to adjust the potentiometer
4. Set output to exactly **5.0V**
5. Mark this converter "ESP32 - 5V"

### Boost Converter #2 (LED Power) - OPTIONAL
Only needed if you want to illuminate the arcade button LED:
1. Connect input to a 3.7V source
2. Connect a multimeter to the output
3. Adjust potentiometer to **12V** (or 5V if your button LED is 5V)
4. Mark this converter "LED - 12V"

⚠️ **Check your button LED voltage!** Some arcade buttons use 5V LEDs, others use 12V. Adjust accordingly.

## 📚 Required Arduino Libraries

Install these libraries via Arduino IDE Library Manager:

1. **ESP32 Board Support**
   - Go to File → Preferences
   - Add to Additional Board Manager URLs:
     ```
     https://espressif.github.io/arduino-esp32/package_esp32_index.json
     ```
   - Tools → Board → Board Manager → Search "esp32" → Install

2. **Adafruit GFX Library** (by Adafruit) - for OLED display
3. **Adafruit SSD1306** (by Adafruit) - for OLED display
4. **Adafruit BusIO** (dependency, should auto-install)

Built-in libraries (no installation needed):
- esp_now
- WiFi
- Wire (I2C for OLED)

## 🚀 Installation Steps

### 1. Get Base Station MAC Address

You need the base station's MAC address before programming buttons.

From base station Serial Monitor, copy the MAC address, which looks like:
```
Base Station MAC Address: AA:BB:CC:DD:EE:FF
```

### 2. Program the Button

1. Open `button.ino` in Arduino IDE
2. **CRITICAL STEP**: Update line 16 with your base station's MAC address:
   ```cpp
   uint8_t baseStationMAC[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
   ```
   
   Example conversion:
   - MAC: `A4:B2:31:F0:12:34`
   - Code: `{0xA4, 0xB2, 0x31, 0xF0, 0x12, 0x34}`

3. Select board settings:
   - **Board**: "ESP32C6 Dev Module"
   - **Upload Speed**: 921600
   
4. Click **Upload**

5. Open Serial Monitor (115200 baud)

6. **IMPORTANT**: Copy and save this button's MAC address:
   ```
   Button MAC Address: BB:CC:DD:EE:FF:11
   ```

7. Repeat for each button, saving each unique MAC address

### 3. Register Buttons with Base Station

1. Connect to base station WiFi: `QuizBuzzer-Setup`
2. Navigate to `http://192.168.4.1`
3. Enter each button's MAC address in the corresponding team slot
4. Click "Save Configuration"

## 🔋 Battery Management

### Charging
1. Plug micro-USB cable into TP4056 module
2. Red LED = Charging
3. Blue/Green LED = Fully charged
4. Charging time: ~1 hour for 503035 500mAh battery

### Battery Life
Expected runtime (approximate with 503035 500mAh battery):
- **Idle/waiting**: 4-6 hours
- **Active use** (frequent presses): 2-3 hours

💡 **Tip**: Implement deep sleep (see customization section) for much longer battery life. The compact 503035 battery is ideal for portable use while keeping the unit small and lightweight.

### Safety
- ✅ Use quality LiPo batteries from reputable sources
- ✅ TP4056 provides overcharge/overdischarge protection
- ❌ Don't leave charging unattended for extended periods
- ❌ Don't use damaged or swollen batteries
- ❌ LiPo batteries are sensitive - handle with care

## 🎮 Button Operation

### Power On
1. Flip toggle switch to ON
2. Built-in LED blinks twice (indicates ready)
3. Startup melody plays on speaker
4. OLED display shows connection status

### Display States

#### Connected (Team Name Received)
```
┌──────────────────┐
│ Team Alpha   85% │  ← Battery indicator
│                  │
│  Ready to play!  │
└──────────────────┘
```

#### Disconnected (Not Configured)
```
┌──────────────────┐
│ A4:B2:31:F0  72% │  ← MAC address + battery
│ HB:OK M:5        │  ← Heartbeat status
│ Btn:3 Sent:8     │  ← Debug counters
└──────────────────┘
```

#### Game Ready
```
┌──────────────────┐
│ Team Alpha   85% │
│                  │
│     READY!       │  ← Large text
│   Press now!     │
└──────────────────┘
```

#### Winner
```
┌──────────────────┐
│ Team Alpha   85% │
│                  │
│    WINNER!       │  ← Large text
│  You got it!     │
└──────────────────┘
```

### Audio Feedback

The button plays different sounds for each state:
- **Startup**: Ascending melody (C-E-G-C)
- **Game Ready**: Single beep (600Hz, 200ms)
- **Button Press**: Descending buzz (800-600-400Hz)
- **Winner**: Victory fanfare (800Hz → 1000Hz)
- **Locked Out**: Sad descending tone (400-300-200Hz)
- **Invalid Press**: Low error tone (200Hz)

Audio can be individually muted per team from the base station web interface.

### LED Status Patterns

Built-in status LED (GPIO2):
- **Waiting**: Slow pulse every 3 seconds
- **Ready**: Fast blink (500ms on/off)
- **Winner**: Solid on
- **Locked Out**: Off

### Battery Monitoring

- Battery percentage shown in top-right of display
- Updates every 30 seconds
- Voltage range: 3.0V (0%) to 4.2V (100%)
- LiPo discharge curve calculated automatically

### Heartbeat System

- Button sends heartbeat every 10 seconds
- Base station responds with team name and game status
- Automatic team name synchronization
- Connection status shown on OLED

### Power Off
Flip toggle switch to OFF when not in use to conserve battery.

## 🔧 Troubleshooting

### Button Not Registering

**Check MAC Address**
- Verify button MAC is correctly entered in base station config
- MAC addresses are case-insensitive but must match exactly
- Check OLED display - should show team name when configured

**Check Game State**
- Game must be started on base station
- OLED should show "READY!" when game is active
- Serial Monitor should show "Button pressed - sending buzz"

**Check ESP-NOW Connection**
- OLED shows heartbeat status (HB:OK or HB:X)
- Serial Monitor shows "Send Status: Success" or "Fail"
- If "Fail", check base station MAC address in button-secret.h

### OLED Display Issues

**Blank Display**
- Check I2C wiring: SDA→GPIO6, SCL→GPIO7
- Verify 3.3V power (NOT 5V - will damage display!)
- Try I2C scanner sketch to detect address
- Default address is 0x3C, some use 0x3D

**Garbled Display**
- Check loose connections
- Verify stable 3.3V power supply
- Ensure common ground with ESP32

**Shows MAC but No Team Name**
- Button not configured in base station yet
- Check base station web interface configuration
- Verify heartbeat communication (HB:OK on display)

### Speaker/Audio Issues

**No Sound**
- Check speaker wiring to GPIO8
- Verify speaker polarity (shouldn't matter for PWM)
- Test with different tone frequencies
- Check if team is muted in base station config

**Distorted Sound**
- Speaker may be 4Ω instead of 8Ω (still works)
- Check for loose connections
- PWM frequency may need adjustment

### Battery Issues

**Battery Percentage Shows 0% or Wrong Value**
- Check voltage divider: 47kΩ → GPIO1 → 47kΩ → GND
- Verify using GPIO1, NOT GPIO0 (boot strapping pin)
- Measure actual battery voltage with multimeter
- Should read ~1.85V at GPIO1 for 3.7V battery

**Won't Charge**
- Check TP4056 red LED comes on when USB plugged in
- Verify USB power source provides at least 500mA
- Check battery isn't over-discharged (should be >2.5V)

**Drains Quickly**
- Check for short circuits with multimeter
- Verify ESP32 isn't constantly transmitting (check Serial Monitor)
- OLED and speaker use power - normal runtime is 2-4 hours
- Consider implementing deep sleep mode for longer life

**Won't Power On**
- Check toggle switch connections
- Verify boost converter is set to 5V
- Check battery voltage (should be 3.0-4.2V)
- Measure 5V at ESP32 VIN pin

### LED Issues (Optional Arcade Button LED)

**LED Not Working**
- Measure boost converter #2 output with multimeter (should be 12V)
- Check transistor connections and orientation
- Verify GPIO4 output with multimeter
- Check LED polarity: + to boost, - to transistor

**LED Too Dim/Bright**
- Adjust boost converter voltage
- Change 330Ω current limiting resistor

### Physical Button Issues

**Button Doesn't Click or Register**
- Arcade buttons need firm pressure
- Check button isn't jammed or damaged
- Verify GPIO15 connection
- Check 10kΩ pull-down resistor is connected
- Serial Monitor should show BtnPress counter incrementing

**False Triggers**
- Check 10kΩ pull-down resistor is connected properly
- Verify button NO/COM connections (not NC!)
- May need debouncing adjustment in code (currently 50ms)
- Check for electrical noise from other components

## 📝 Customization

### button/button-secret.h
```cpp
#define BASE_STATION_MAC {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
```

### Implement Deep Sleep for Battery Life

Add to end of `loop()`:
```cpp
// After 5 minutes of no button press, go to sleep
if (millis() - lastPressTime > 300000) {
  enterDeepSleep();
}
```

This will wake on button press and dramatically extend battery life.

### Adjust Debounce Delay

Change line 31:
```cpp
const unsigned long debounceDelay = 50; // Milliseconds
```
Increase if getting double-presses, decrease for faster response.

### Change Button Pin

To use a different GPIO pin for the button, update:
- Line 12: `#define BUTTON_PIN 15` → your pin number
- Line 90: `esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_PIN, HIGH);`

### Use Different LED Pin

To use a different GPIO for LED control:
- Line 13: `#define LED_PIN 2` → your pin number
- Line 14: `#define BOOST_EN_PIN 4` → your pin number

## 🔐 Security Considerations

ESP-NOW uses encryption by default in Arduino implementation. For additional security in public events, you can:
1. Enable ESP-NOW encryption (requires PMK/LMK keys)
2. Add a "pairing" button that only allows registration when pressed
3. Implement MAC address filtering

## 💾 Code Structure

Key functions:
- `setup()` - Initializes ESP-NOW, I2C, OLED, speaker, and button
- `loop()` - Checks for button press, sends heartbeats, updates display and battery
- `buttonISR()` - Hardware interrupt for instant button detection
- `onDataRecv()` - Receives status updates and team name from base station
- `onDataSent()` - Callback for ESP-NOW transmission status
- `sendBuzzer()` - Transmits button press via ESP-NOW
- `sendHeartbeat()` - Sends periodic heartbeat to base station
- `updateDisplay()` - Updates OLED with current state, team name, and battery
- `setStatusLED()` - Controls built-in status LED (GPIO2)
- `showStatePattern()` - Displays LED patterns based on game state
- `playTone()` - Generates PWM audio tones on speaker
- `playBuzzSound()` - Button press audio feedback
- `playWinnerSound()` - Victory fanfare
- `playLockoutSound()` - Lockout tone
- `playReadySound()` - Game ready beep
- `playStartupSound()` - Boot melody
- `readBatteryVoltage()` - Reads ADC from voltage divider (GPIO1)
- `calculateBatteryPercent()` - Converts voltage to percentage
- `updateBatteryLevel()` - Updates battery display
- `drawBatteryIcon()` - Draws battery indicator on OLED

## 📦 Enclosure Recommendations

Consider housing each button unit in:
- **3D printed case** - Custom design for perfect fit
- **Project box** - Hammond or similar, drill hole for button
- **Arcade button housing** - Purpose-built for arcade buttons

Mount considerations:
- Make TP4056 USB port accessible for charging
- Expose toggle switch for easy on/off
- Ensure button is securely mounted
- Consider strain relief for any external wiring

## 🧪 Testing Checklist

Before final assembly:
- [ ] Boost converter set to 5.0V (measure with multimeter)
- [ ] OLED display initializes and shows MAC address
- [ ] Speaker plays startup melody on power-on
- [ ] Button press registers in Serial Monitor (BtnPress counter increments)
- [ ] Built-in status LED blinks on power-up
- [ ] Battery percentage displays correctly on OLED
- [ ] ESP-NOW communication successful (HB:OK on display)
- [ ] Team name appears on OLED after base station configuration
- [ ] All game state sounds play correctly (ready, buzz, winner, lockout)
- [ ] Battery charges properly (TP4056 LED indicators: red=charging, blue/green=full)
- [ ] Toggle switch cuts power completely
- [ ] No short circuits (check all power rails with multimeter)
- [ ] Optional: Arcade button LED lights up (if using LED control circuit)

## 🆘 Getting Help

If you encounter issues:
1. Check Serial Monitor for error messages
2. Test each component individually
3. Verify all connections match wiring diagram
4. Measure voltages with multimeter
5. Open a GitHub issue with:
   - Serial Monitor output
   - Photos of your wiring
   - Description of the problem

---

**Next Step**: Test your complete system! See the [main README](../README.md) for game operation.