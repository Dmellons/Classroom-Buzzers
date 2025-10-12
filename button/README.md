# Button Unit Setup Guide

Each button unit is a self-contained wireless buzzer powered by an 18650 battery. It communicates with the base station using ESP-NOW and provides LED feedback through the arcade button.

## 📋 Hardware Components (Per Button)

| Component | Quantity | Specifications |
|-----------|----------|----------------|
| ESP32-C6-DevKitC-1 | 1 | Main microcontroller |
| 60mm LED Arcade Button | 1 | With built-in LED (5V or 12V) |
| 18650 Li-ion Battery | 1 | 3.7V, protected cell recommended |
| TP4056 Charging Module | 1 | With protection circuit |
| MT3608 Boost Converter | 2 | One for ESP32 (3.7V→5V), one for LED (3.7V→12V) |
| 2N2222 NPN Transistor | 1 | Or any similar NPN transistor |
| 10kΩ Resistor | 1 | For button pull-down |
| 1kΩ Resistor | 1 | For transistor base |
| Toggle Switch | 1 | SPST, for main power |
| 18650 Battery Holder | 1 | With solder tabs or wires |
| Jumper Wires | Various | 22-24 AWG recommended |

## 🔌 Wiring Diagram

### Power System
```
18650 Battery (+) → TP4056 BAT+
18650 Battery (-) → TP4056 BAT-

TP4056 OUT+ → Toggle Switch → Split to both boost converters IN+
TP4056 OUT- → Both boost converters IN-

Boost Converter #1 (5V for ESP32):
  OUT+ (adjusted to 5V) → ESP32 5V/VIN pin
  OUT- (GND) → ESP32 GND

Boost Converter #2 (12V for LED):
  OUT+ (adjusted to 12V) → Transistor Collector
  OUT- (GND) → Common Ground
```

⚠️ **CRITICAL**: All grounds must be connected together!

### Button Input Circuit
```
Arcade Button:
  NO (Normally Open) → ESP32 3.3V
  COM (Common) → ESP32 GPIO15
  
10kΩ Pull-down Resistor:
  One end → ESP32 GPIO15
  Other end → ESP32 GND
```

### LED Control Circuit
```
ESP32 GPIO4 → 1kΩ Resistor → Transistor Base (B)
Transistor Emitter (E) → Button LED (-)
Button LED (+) → Boost Converter #2 OUT+ (12V)
Transistor Collector (C) → Boost Converter #2 OUT- (GND)
```

### Charging Port
```
USB 5V+ → TP4056 IN+
USB GND → TP4056 IN-
```

💡 **Tip**: Mount the TP4056 module so the micro-USB port is accessible from outside your enclosure for easy charging.

## ⚙️ Boost Converter Setup

**BEFORE connecting to ESP32 or LED, adjust the boost converters!**

### Boost Converter #1 (ESP32 Power)
1. Connect input to a 3.7V source (or the battery)
2. Connect a multimeter to the output
3. Use a small screwdriver to adjust the potentiometer
4. Set output to exactly **5.0V**
5. Mark this converter "ESP32 - 5V"

### Boost Converter #2 (LED Power)
1. Connect input to a 3.7V source
2. Connect a multimeter to the output
3. Adjust potentiometer to **12V** (or 5V if your button LED is 5V)
4. Mark this converter "LED - 12V"

⚠️ **Check your button LED voltage!** Some arcade buttons use 5V LEDs, others use 12V. Adjust accordingly.

## 📚 Required Arduino Libraries

Install these via Library Manager:
- **ESP32 Board Support** (same as base station)

Built-in libraries (no installation needed):
- esp_now
- WiFi

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
4. Charging time: ~2-4 hours for typical 2000mAh battery

### Battery Life
Expected runtime (approximate):
- **Idle/waiting**: 8-12 hours
- **Active use** (frequent presses): 4-6 hours

💡 **Tip**: Implement deep sleep (see customization section) for much longer battery life.

### Safety
- ✅ Use protected 18650 cells (with built-in protection circuit)
- ✅ TP4056 provides overcharge/overdischarge protection
- ❌ Don't leave charging unattended for extended periods
- ❌ Don't use damaged or swollen batteries

## 🎮 Button Operation

### Power On
1. Flip toggle switch to ON
2. Button LED should blink twice (indicates ready)
3. Press button to test - should blink briefly

### States
- **Waiting**: LED off, game not started
- **Ready**: LED off, game started, waiting for press
- **Pressed**: LED flashes briefly when you press
- **Winner**: LED stays on solid
- **Locked Out**: LED stays off after someone else buzzed

### Power Off
Flip toggle switch to OFF when not in use to conserve battery.

## 🔧 Troubleshooting

### Button Not Registering

**Check MAC Address**
- Verify button MAC is correctly entered in base station config
- MAC addresses are case-insensitive but must match exactly

**Check Game State**
- Game must be started on base station
- Serial Monitor should show "Button pressed - sending buzz"

**Check ESP-NOW Connection**
- Serial Monitor shows "Send Status: Success" or "Fail"
- If "Fail", check base station MAC address in code

### LED Not Working

**Check LED Voltage**
- Measure boost converter output with multimeter
- Should be 12V (or 5V depending on your button)

**Check Transistor**
- With button pressed and winner, measure transistor emitter voltage
- Should be close to 0V when LED should be on
- If not, check GPIO4 output and transistor connections

**Check LED Polarity**
- Button LED has + and - terminals
- LED + should go to boost converter +
- LED - should go to transistor emitter

### Battery Issues

**Won't Charge**
- Check TP4056 red LED comes on when USB plugged in
- Verify USB power source provides at least 500mA
- Check battery isn't over-discharged (should be >2.5V)

**Drains Quickly**
- Check for short circuits
- Verify ESP32 isn't constantly transmitting
- Consider implementing deep sleep mode

**Won't Power On**
- Check toggle switch connections
- Verify boost converter is set to 5V
- Check battery voltage (should be 3.0-4.2V)

### Physical Button Issues

**Button Doesn't Click**
- Arcade buttons need firm pressure
- Check button isn't jammed or damaged

**False Triggers**
- Check 10kΩ pull-down resistor is connected
- Verify button NO/COM connections
- May need debouncing adjustment in code

## 📝 Customization

### button/secret.h
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
- `setup()` - Initializes ESP-NOW and button
- `loop()` - Checks for button press, handles debouncing
- `buttonISR()` - Hardware interrupt for instant button detection
- `onDataRecv()` - Receives status updates from base station
- `sendBuzzer()` - Transmits button press via ESP-NOW
- `setLED()` - Controls button LED state

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
- [ ] Boost converters set to correct voltages
- [ ] Button press registers in Serial Monitor
- [ ] LED lights up on button press
- [ ] ESP-NOW communication successful
- [ ] Battery charges properly (TP4056 LED indicators)
- [ ] Toggle switch cuts power completely
- [ ] No short circuits (check with multimeter)

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