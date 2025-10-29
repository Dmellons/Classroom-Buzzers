# Complete Button Unit Wiring Guide

## Overview
This guide covers the complete wiring for a wireless quiz buzzer button unit including ESP32-C6, OLED display, arcade button, battery management, and power control.

## Components Required (Per Button Unit)

### Main Components:
- 1x ESP32-C6-DevKitC-1 (or ESP32-C6-DevKitM-1)
- 1x 0.91" OLED Display (128x32 I2C SSD1306)
- 1x Large Arcade Button (60mm LED illuminated)
- 1x 18650 Li-ion Battery (3.7V, 3000-3500mAh recommended)
- 1x TP4056 Battery Charging Module (with protection)
- 1x MT3608 DC-DC Boost Converter (3.7V → 5V)
- 1x MT3608 DC-DC Boost Converter (3.7V → 12V for LED)
- 1x Toggle Switch (SPST, for main power)
- 1x 2N2222 NPN Transistor (for LED control)
- 1x 18650 Battery Holder
- 1x USB-C charging port (optional - can use TP4056's micro USB)

### Resistors:
- 1x 10kΩ resistor (button pull-down)
- 1x 1kΩ resistor (transistor base)
- 1x 330Ω resistor (LED current limiting)

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
| **LED Control** | GPIO4 | Orange | Controls button LED via transistor |
| **Boost Enable** | GPIO2 | Purple | Optional boost converter enable |
| **Battery Monitor** | GPIO0/A0 | White | Battery voltage sensing |
| **Power (5V)** | VIN | Red | From boost converter |
| **Ground** | GND | Black | Common ground |
| **3.3V** | 3V3 | Red | For pull-up if needed |

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

Button LED Control Circuit:
  ESP32 GPIO4 ──[1kΩ]── 2N2222 Base
  2N2222 Collector ─────────────────── 12V Boost OUT+
  2N2222 Emitter ───[330Ω]─────────── Button LED + (Anode)
  Button LED - (Cathode) ───────────── Common GND

Power Management System:
  18650 Battery + ──────────────────── TP4056 BAT+
  18650 Battery - ──────────────────── TP4056 BAT-
  
  TP4056 OUT+ ──── Power Switch ───── 5V Boost IN+ & 12V Boost IN+
  TP4056 OUT- ──────────────────────── 5V Boost IN- & 12V Boost IN-
  
  5V Boost OUT+ (adjusted to 5.0V) ──── ESP32 VIN
  5V Boost GND ─────────────────────── Common GND
  
  12V Boost OUT+ (adjusted to 12V) ──── LED Control Circuit
  12V Boost GND ────────────────────── Common GND

Battery Voltage Monitoring:
  TP4056 OUT+ ──[47kΩ]─┬─[47kΩ]── GND
                        │
                   ESP32 GPIO0 (A0)

Charging Circuit:
  USB-C/Micro USB 5V ─────────────── TP4056 IN+
  USB-C/Micro USB GND ────────────── TP4056 IN-

Common Ground Network:
  ████ ALL THESE MUST BE CONNECTED ████
  - 18650 Battery -
  - TP4056 OUT- and GND
  - Both Boost Converter GND
  - ESP32 GND
  - OLED GND
  - Button circuit GND
  - LED circuit GND
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
   18650 Battery + (Red) ──── TP4056 BAT+
   18650 Battery - (Black) ── TP4056 BAT-
   ```

2. **Power Switch:**
   ```
   TP4056 OUT+ ──── Switch Terminal 1
   Switch Terminal 2 ──── Both Boost Converters IN+
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

3. **LED Control:**
   ```
   ESP32 GPIO4 ──[1kΩ]── 2N2222 Base (Orange wire)
   ```

### 4. Build the LED Control Circuit

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

### 5. Battery Monitoring Circuit

```
Battery Voltage Divider:
TP4056 OUT+ ──[47kΩ]─┬─[47kΩ]── GND
                      │
                 ESP32 GPIO0
```

This divides the battery voltage by 2, so:
- 4.2V battery → 2.1V at GPIO0
- 3.7V battery → 1.85V at GPIO0  
- 3.0V battery → 1.5V at GPIO0

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
2. **Serial Monitor:** Check for MAC address and battery voltage readings
3. **Button Test:** Press button and verify GPIO15 reading
4. **LED Test:** Verify LED control via GPIO4
5. **OLED Test:** Check display initialization and text output

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
| 18650 Battery | 1 | 3.7V 3000mAh with protection | $8 |
| TP4056 Module | 1 | With protection circuit | $3 |
| MT3608 Boost (5V) | 1 | DC-DC Step-up converter | $2 |
| MT3608 Boost (12V) | 1 | DC-DC Step-up converter | $2 |
| Toggle Switch | 1 | SPST 3A rated | $2 |
| 2N2222 Transistor | 1 | NPN switching transistor | $0.50 |
| Resistors | 3 | 10kΩ, 1kΩ, 330Ω, 47kΩ(2x) | $1 |
| Wire & Connectors | 1 | 22AWG stranded, various colors | $5 |
| Enclosure | 1 | Plastic project box | $10 |
| **Total per unit** | | | **~$68.50** |

---

This complete wiring guide provides everything needed to build a professional wireless quiz buzzer button with display, battery management, and robust construction.