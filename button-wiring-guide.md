# Complete Button Unit Wiring Guide

## Overview
This guide covers the complete wiring for a wireless quiz buzzer button unit including ESP32-C6, OLED display, arcade button, 3W speaker for audio feedback, battery management, and power control.

**Current Implementation Status:** This guide reflects the actively developed battery-powered version with audio feedback, voltage monitoring, and OLED display.

## Components Required (Per Button Unit)

### Main Components:
- 1x ESP32-C6-DevKitC-1 (or ESP32-C6-DevKitM-1)
- 1x 0.91" OLED Display (128x32 I2C SSD1306)
- 1x Large Arcade Button (60mm LED illuminated)
- 1x 3W 8Ω Speaker (for audio feedback - buzzer sounds, victory tunes)
- 1x 503035 LiPo Battery (3.7V 500mAh) - or larger capacity like 18650
- 1x TP4056 Battery Charging Module (with protection)
- 1x MT3608 DC-DC Boost Converter (3.7V → 5V for ESP32)
- 1x MT3608 DC-DC Boost Converter (3.7V → 12V for button LED) - Optional
- 1x Toggle Switch (SPST, for main power)
- 1x 2N2222 NPN Transistor (for LED control) - Optional
- 1x Power Indicator LED (3mm or 5mm, any color)
- 1x USB-C charging port (optional - can use TP4056's micro USB)

### Resistors:
- 1x 10kΩ resistor (button pull-down)
- 1x 1kΩ resistor (transistor base)
- 1x 1kΩ resistor (power indicator LED)
- 1x 330Ω resistor (LED current limiting)
- 2x 47kΩ resistor (battery voltage divider)

### Wiring Materials:
- 22-24 AWG stranded wire (various colors)
- Heat shrink tubing
- Solder and flux
- Perfboard or PCB (optional)

---

## Pin Assignments (ESP32-C6)

| Function | ESP32-C6 Pin | Wire Color | Notes |
|----------|--------------|------------|--------|
| **I2C SDA** | GPIO6 | Blue | OLED Display Data |
| **I2C SCL** | GPIO7 | Yellow | OLED Display Clock |
| **Button Input** | GPIO15 | Green | Arcade button signal |
| **Status LED** | GPIO2 | Orange | Built-in LED status indicator |
| **Speaker Output** | GPIO8 | Purple | PWM audio feedback (3W 8Ω speaker) |
| **Battery Monitor** | GPIO1/A0 | White | Battery voltage sensing via ADC |
| **LED Control** | GPIO4 | Red/White | (Optional) Controls button LED via transistor |
| **Power (5V)** | VIN | Red | From boost converter |
| **Ground** | GND | Black | Common ground |
| **3.3V** | 3V3 | Red | For pull-up if needed |

### Important Pin Selection Notes

**Why GPIO1 for Battery Monitoring?**
- GPIO0 is a **strapping pin** on ESP32-C6 that affects boot mode
- Connecting voltage dividers to GPIO0 can cause boot failures
- GPIO1 is ADC-capable and has no boot-time conflicts
- The voltage divider (two 47kΩ resistors) safely divides battery voltage by 2

**ADC-Capable Pins on ESP32-C6:**
- **ADC1:** GPIO0-GPIO7 (use GPIO1-5 for analog sensing to avoid strapping pins)
- GPIO18-GPIO23 are **NOT** ADC-capable and cannot be used for battery voltage sensing

**Audio Output Requirements:**
- Speaker connects directly to GPIO8 (PWM capable)
- No amplifier needed for 3W 8Ω speaker
- ESP32 can drive the speaker directly with sufficient volume

---

## Complete Wiring Diagram

```
ESP32-C6 BUTTON UNIT WIRING
============================

OLED Display (0.91" 128x32 I2C):
  ESP32 GPIO6 (SDA) ──────────────── OLED SDA
  ESP32 GPIO7 (SCL) ──────────────── OLED SCL
  ESP32 3.3V ─────────────────────── OLED VCC
  ESP32 GND ──────────────────────── OLED GND

Arcade Button Connection:
  Button NO (Normally Open) ────────── ESP32 3.3V
  Button COM (Common) ───────────────── ESP32 GPIO15
  10kΩ Resistor ─────────────────────── Between GPIO15 and GND

Speaker Connection (3W 8Ω):
  ESP32 GPIO8 ────────────────────── Speaker + (Positive)
  ESP32 GND ──────────────────────── Speaker - (Negative)
  Note: Direct connection OK for 3W speaker, ESP32 PWM can drive it

Status LED (Built-in):
  ESP32 GPIO2 ────────────────────── Built-in LED (onboard)

Button LED Control Circuit:
  ESP32 GPIO4 ──[1kΩ]── 2N2222 Base
  2N2222 Collector ─────────────────── 12V Boost OUT+
  2N2222 Emitter ───[330Ω]─────────── Button LED + (Anode)
  Button LED - (Cathode) ───────────── Common GND

Power Management System:
  503035 LiPo Battery + ────────────── TP4056 BAT+
  503035 LiPo Battery - ────────────── TP4056 BAT-

  TP4056 OUT+ ──── Power Switch ───── 5V Boost IN+ & 12V Boost IN+
  TP4056 OUT- ──────────────────────── 5V Boost IN- & 12V Boost IN-

  5V Boost OUT+ (adjusted to 5.0V) ──── ESP32 VIN
  5V Boost GND ─────────────────────── Common GND

  12V Boost OUT+ (adjusted to 12V) ──── LED Control Circuit
  12V Boost GND ────────────────────── Common GND

Power Indicator LED:
  TP4056 OUT+ (after switch) ──[1kΩ]── Power LED Anode (+)
  Power LED Cathode (-) ────────────── Common GND

Battery Voltage Monitoring:
  TP4056 OUT+ ──[47kΩ]─┬─[47kΩ]── GND
                        │
                   ESP32 GPIO1 (A0)

Charging Circuit:
  USB-C/Micro USB 5V ─────────────── TP4056 IN+
  USB-C/Micro USB GND ────────────── TP4056 IN-

Common Ground Network:
  ████ ALL THESE MUST BE CONNECTED ████
  - 503035 LiPo Battery -
  - TP4056 OUT- and GND
  - Both Boost Converter GND
  - ESP32 GND
  - OLED GND
  - Button circuit GND
  - LED circuit GND
  - Power Indicator LED -
  ████████████████████████████████████
```

---

## Step-by-Step Assembly Instructions

### 1. Prepare the Boost Converters

**5V Boost Converter (for ESP32):**
1. Connect multimeter to output terminals
2. Power the input with 3.7V (or use a bench supply)
3. Slowly adjust the potentiometer until output reads exactly 5.0V
4. Mark with label "ESP32 - 5.0V"

**12V Boost Converter (for LED):**
1. Connect multimeter to output terminals
2. Power the input with 3.7V
3. Adjust potentiometer until output reads 12.0V (or voltage required by your LED)
4. Mark with label "LED - 12V"

### 2. Wire the Power System

1. **Battery Connection:**
   ```
   503035 LiPo Battery + (Red) ──── TP4056 BAT+
   503035 LiPo Battery - (Black) ── TP4056 BAT-
   ```

2. **Power Switch and Indicator LED:**
   ```
   TP4056 OUT+ ──── Switch Terminal 1
   Switch Terminal 2 ──── Both Boost Converters IN+ & Power LED (via 1kΩ)
   Power LED Anode (+) ──[1kΩ]── TP4056 OUT+ (after switch)
   Power LED Cathode (-) ──────── Common GND
   ```

3. **Boost Converters:**
   ```
   TP4056 OUT- ──── Both Boost Converters IN-
   5V Boost OUT+ ──── ESP32 VIN
   12V Boost OUT+ ──── LED Control Circuit
   All GND ──── Common Ground Bus
   ```

### 3. Wire the ESP32 Connections

1. **OLED Display:**
   ```
   ESP32 GPIO6 ──── OLED SDA (Blue wire)
   ESP32 GPIO7 ──── OLED SCL (Yellow wire)
   ESP32 3.3V ──── OLED VCC (Red wire)
   ESP32 GND ──── OLED GND (Black wire)
   ```

2. **Button Input:**
   ```
   Button NO ──── ESP32 3.3V (Red wire)
   Button COM ──── ESP32 GPIO15 (Green wire)
   10kΩ Resistor ──── Between GPIO15 and GND
   ```

3. **Speaker (3W 8Ω Audio Feedback):**
   ```
   ESP32 GPIO8 ──── Speaker + (Purple wire)
   ESP32 GND ──── Speaker - (Black wire)
   ```

4. **Status LED (Built-in):**
   ```
   ESP32 GPIO2 ──── Built-in LED (onboard, no external wiring needed)
   ```

5. **Battery Monitoring:**
   ```
   TP4056 OUT+ ──[47kΩ]─┬─[47kΩ]── GND
                         │
                    ESP32 GPIO1 (White wire)
   ```

6. **LED Control (Optional - for arcade button illumination):**
   ```
   ESP32 GPIO4 ──[1kΩ]── 2N2222 Base (Orange wire)
   ```

### 7. Build the LED Control Circuit (Optional)

```
LED Control Circuit Assembly:
           12V+
            │
            │
    ┌───────┴──────┐
    │   2N2222     │
    │   Collector  │
    │       │      │
    │   ┌───┴───┐  │
    │   │ 330Ω  │  │  
    │   └───┬───┘  │
    │       │      │
    │   Button LED+│
    │       │      │
    │   Button LED-│
    │       │      │
    │   ┌───┴───┐  │
    │   │  GND  │  │
    │   └───────┘  │
    │              │
    │     Base     │
    │   ┌───┴───┐  │
    │   │  1kΩ  │  │
    │   └───┬───┘  │
    │       │      │
    └───────┴──────┘
         GPIO4
```

---

## Enclosure and Mounting

### Recommended Enclosure Layout:
```
     ┌─────────────────────────────────┐
     │  ┌─────────┐    ┌─────────────┐ │
     │  │ ESP32-C6│    │   TP4056    │ │
     │  │         │    │  Charging   │ │
     │  └─────────┘    └─────────────┘ │
     │                                 │
     │  ┌─────────┐    ┌─────────────┐ │
     │  │  OLED   │    │ 5V Boost    │ │
     │  │ Display │    │ Converter   │ │
     │  └─────────┘    └─────────────┘ │
     │                                 │
     │     ┌───────────────────┐       │
     │     │    18650 Battery  │       │
     │     │     Holder        │       │
     │     └───────────────────┘       │
     │                                 │
     │  ┌─────────────┐  ┌───────────┐ │
     │  │ 12V Boost   │  │   Power   │ │
     │  │ Converter   │  │  Switch   │ │
     │  └─────────────┘  └───────────┘ │
     └─────────────────────────────────┘
                    │
              ┌─────────────┐
              │   Arcade    │
              │   Button    │
              │  (60mm LED) │
              └─────────────┘
```

### Mounting Points:
1. **Arcade Button:** Mount through enclosure top with retaining nut
2. **OLED Display:** Mount behind small window or cutout
3. **Power Switch:** Side-mounted toggle switch
4. **Charging Port:** Use TP4056's micro USB or add external USB-C
5. **ESP32:** Secure with standoffs or mounting posts

---

## Testing and Calibration

### Pre-Power Testing:
1. **Continuity Check:** Verify all ground connections with multimeter
2. **Voltage Check:** Test boost converter outputs before connecting ESP32
3. **Isolation Check:** Ensure no shorts between power rails

### Power-On Testing:
1. **Battery Voltage:** Check TP4056 output voltage (should be 3.7-4.2V)
2. **Boost Converters:** Verify 5V and 12V outputs under load
3. **ESP32 Power:** Confirm 5V at VIN pin and 3.3V at 3V3 pin

### Software Testing:
1. **Upload Code:** Flash the button.ino sketch
2. **Serial Monitor:** Check for MAC address and battery voltage readings (GPIO1 ADC)
3. **Button Test:** Press button and verify GPIO15 reading
4. **Speaker Test:** Verify startup sound plays on GPIO8
5. **Status LED Test:** Check GPIO2 built-in LED patterns
6. **OLED Test:** Check display shows team name, battery %, and game status
7. **Battery Monitor:** Verify battery voltage and percentage display correctly

### ESP-NOW Pairing:
1. **Get MAC Address:** Note the button's MAC from Serial Monitor
2. **Base Station Config:** Add button MAC to base station team configuration
3. **Connection Test:** Verify heartbeat communication and button presses

---

## Troubleshooting Guide

### Power Issues:
| Problem | Possible Cause | Solution |
|---------|----------------|----------|
| ESP32 won't start | No 5V power | Check boost converter output and connections |
| Battery drains quickly | Short circuit | Check all connections with multimeter |
| Won't charge | TP4056 wiring | Verify charging circuit connections |

### Communication Issues:
| Problem | Possible Cause | Solution |
|---------|----------------|----------|
| No ESP-NOW connection | Wrong MAC address | Re-check MAC addresses in configuration |
| OLED blank | I2C wiring | Verify SDA/SCL connections and address |
| Button not responding | GPIO15 wiring | Check button and pull-down resistor |

### LED Issues:
| Problem | Possible Cause | Solution |
|---------|----------------|----------|
| LED won't light | Transistor circuit | Check transistor orientation and connections |
| LED too dim | Wrong voltage | Verify 12V boost output and current limiting |
| LED always on | Transistor fault | Check GPIO4 control signal and transistor |

---

## Safety Notes

⚠️ **Important Safety Guidelines:**

1. **Battery Safety:**
   - Use only quality 18650 batteries with protection circuits
   - Never short circuit battery terminals
   - Monitor charging temperature

2. **Electrical Safety:**
   - Double-check all connections before powering on
   - Use appropriate wire gauges for current loads
   - Insulate all connections properly

3. **Component Protection:**
   - Ensure correct voltage levels before connecting ESP32
   - Use ESD precautions when handling electronics
   - Verify polarity on all power connections

---

## Bill of Materials (BOM)

| Component | Quantity | Part Number/Specs | Estimated Cost |
|-----------|----------|-------------------|----------------|
| ESP32-C6-DevKitC-1 | 1 | Espressif ESP32-C6-DevKitC-1 | $15 |
| OLED Display | 1 | 0.91" 128x32 I2C SSD1306 | $8 |
| Arcade Button | 1 | 60mm LED illuminated | $12 |
| 3W Speaker | 1 | 3W 8Ω speaker for audio feedback | $6 |
| 503035 LiPo Battery | 1 | 3.7V 500mAh (or 18650 for longer runtime) | $5-10 |
| TP4056 Module | 1 | With protection circuit | $3 |
| MT3608 Boost (5V) | 1 | DC-DC Step-up converter for ESP32 | $2 |
| MT3608 Boost (12V) | 1 | DC-DC Step-up converter for LED (optional) | $2 |
| Toggle Switch | 1 | SPST 3A rated | $2 |
| Power Indicator LED | 1 | 3mm or 5mm, any color | $0.50 |
| 2N2222 Transistor | 1 | NPN switching transistor (optional) | $0.50 |
| Resistors | 6 | 10kΩ, 1kΩ(2x), 330Ω, 47kΩ(2x) | $1 |
| Wire & Connectors | 1 | 22AWG stranded, various colors | $5 |
| Enclosure | 1 | Plastic project box | $10 |
| **Total per unit** | | | **~$72.00** |

**Note:** Items marked "optional" are for arcade button LED illumination. The core functionality (button press detection, audio feedback, OLED display, battery monitoring) works without them.

---

This complete wiring guide provides everything needed to build a professional wireless quiz buzzer button with display, battery management, and robust construction.