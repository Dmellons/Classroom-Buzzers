# 3D Printable Enclosure Guide

This directory contains parametric OpenSCAD designs for the button unit housing.

## Files

- `button-housing.scad` - Parametric OpenSCAD source file

## Generating STL Files

### Option 1: Using OpenSCAD (Recommended)

1. **Download and install OpenSCAD**: https://openscad.org/downloads.html

2. **Open the file**: `button-housing.scad`

3. **Render and export**:
   - Press **F6** to render (this may take a minute)
   - File → Export → Export as STL
   - Save as `button-housing-bottom.stl`

4. **Export top half**:
   - Edit the file, uncomment the top half render option:
     ```
     // Option 3: Top half only (for printing - flip in slicer)
     translate([0, 0, lid_height]) rotate([180, 0, 0]) top_half();
     ```
   - Comment out the bottom half line
   - Press **F6** to render
   - Export as `button-housing-top.stl`

### Option 2: Online Converter

If you don't want to install OpenSCAD:
1. Go to https://www.viewstl.com/ or similar
2. Some online services can convert SCAD to STL
3. Note: Complex parametric designs may not work with all converters

### Option 3: Request Pre-Generated STL

The OpenSCAD file is parametric, meaning dimensions can be adjusted. If you want pre-generated STL files with default dimensions, you can:
1. Ask in the GitHub Issues for STL exports
2. Use a 3D printing service that accepts SCAD files

## ABS Print Settings

**Recommended settings for ABS:**

| Setting | Value |
|---------|-------|
| Nozzle Temperature | 230-250°C |
| Bed Temperature | 90-110°C |
| Layer Height | 0.2mm |
| Infill | 20-30% |
| Walls/Perimeters | 3-4 |
| Top/Bottom Layers | 4-5 |
| Supports | Not needed for bottom, may need for top |

**Important ABS Tips:**
- Use an enclosed printer if possible (prevents warping)
- Use a heated bed with adhesion (glue stick, ABS slurry, or PEI sheet)
- Allow parts to cool slowly (don't open enclosure immediately)
- Consider printing with a brim for better bed adhesion

## Design Details

### Button Unit Dimensions
- **Internal**: 80mm x 100mm x 45mm
- **Wall Thickness**: 2.5mm
- **Corner Radius**: 5mm
- **Lid Overlap**: 2mm
- **Lid Tolerance**: 0.4mm (increased for easier assembly)

### Button Unit Cutouts (Verified Dimensions)
- **Top**: 28mm hole for 60mm arcade button
- **Front**: OLED window (23mm x 12mm) for 0.91" 128x32 display
- **Right Side**: Speaker grill (40mm diameter, 3mm holes)
- **Back**: USB-C port (10mm x 4.5mm, widened for easier cable insertion)
- **Left Side**: Toggle switch hole (6.2mm diameter)

### Base Station Dimensions
- **Internal**: 100mm x 80mm x 35mm
- **Wall Thickness**: 2.5mm
- **Corner Radius**: 5mm
- **Lid Tolerance**: 0.4mm (increased for easier assembly)

### Base Station Cutouts (Verified Dimensions)
- **Front**: OLED window (23mm x 13mm) for 0.96" 128x64 display
- **Top**: Two 12mm holes for control buttons
- **Back**: USB-C port (12mm x 8mm), SD card slot (30mm x 5mm)
- **Sides**: Ventilation slots for cooling

### Internal Features

**Button Unit:**
- Corner screw posts (M3 screw holes with chamfered entrances)
- OLED friction-fit pocket with 0.5mm tolerance
- ESP32 mounting rails (updated for 51.8mm length)
- TP4056 mounting rails near USB-C cutout
- Ventilation slots for boost converter heat dissipation
- Improved lid tolerance (0.4mm) for easier assembly

**Base Station:**
- Corner screw posts (M3 screw holes)
- OLED mounting ledge
- ESP32 mounting rails (updated for 51.8mm length)
- Ventilation slots on sides
- Improved OLED window sizing (23mm x 13mm)

## Verified Component Dimensions

All dimensions have been verified from manufacturer datasheets (December 2024):

### Button Unit Components
| Component | Actual Dimensions | Notes |
|-----------|-------------------|-------|
| ESP32-C6-DevKitC-1 | 25.4 × 51.8 × 10mm | Official Espressif specs |
| 0.91" OLED (128x32) | PCB: 38 × 12mm, Active: 22.4 × 5.6mm | SSD1306 I2C |
| 60mm Arcade Button | 28mm mounting hole | Standard size |
| 3W 8Ω Speaker | 40mm dia × 20mm height | For audio feedback |
| TP4056 Charger | 26 × 17mm | With protection circuit |
| MT3608 Boost | 37 × 17 × 14mm | Including potentiometer |
| 503035 LiPo Battery | 35 × 30 × 5mm | 500mAh capacity |
| Toggle Switch | 6mm mounting hole | SPST power switch |

### Base Station Components
| Component | Actual Dimensions | Notes |
|-----------|-------------------|-------|
| ESP32-C6-DevKitC-1 | 25.4 × 51.8 × 10mm | Official Espressif specs |
| 0.96" OLED (128x64) | PCB: 27 × 27mm, Active: 22 × 12mm | SSD1306 I2C |
| MicroSD Module | 20-28mm × 20-25mm | Varies by manufacturer |
| Momentary Buttons | 12mm mounting hole | Start/Stop and Reset |

## Customization

The OpenSCAD files are fully parametric. All component dimensions are defined at the top of each file and can be adjusted if you have different parts.

**Button Housing** (`button-housing.scad`):
```scad
// Verified dimensions (DO NOT change unless using different components)
esp32_length = 51.8;     // ESP32-C6 length
oled_window_w = 23;      // OLED window width
tp4056_length = 26;      // Charger module length
tp4056_width = 17;       // Charger module width
mt3608_length = 37;      // Boost converter length
mt3608_height = 14;      // Boost converter height
switch_hole_dia = 6.2;   // Toggle switch hole
usbc_width = 10;         // USB-C opening (widened)
usbc_height = 4.5;       // USB-C opening (widened)
```

**Base Station Housing** (`base-station-housing.scad`):
```scad
// Verified dimensions
esp32_length = 51.8;     // ESP32-C6 length
oled_window_w = 23;      // OLED window width
oled_window_h = 13;      // OLED window height
usbc_width = 12;         // USB-C opening
usbc_height = 8;         // USB-C opening
```

## Assembly

1. Print both halves (bottom upright, top flipped)
2. Install components in bottom half:
   - Mount ESP32 on rails
   - Secure TP4056 near USB port opening
   - Mount OLED behind window
   - Position speaker at grill
   - Wire battery and boost converter
3. Install toggle switch in side hole
4. Mount arcade button in top
5. Connect wires to button
6. Secure with M3 screws in corner posts

## Recent Updates (December 2024)

### Dimension Corrections
All component dimensions have been updated with verified measurements from manufacturer datasheets:
- **ESP32-C6**: Length corrected from 54mm to 51.8mm
- **Button OLED window**: Reduced from 24mm to 23mm width
- **Base OLED window**: Reduced from 30mm × 16mm to 23mm × 13mm
- **TP4056**: Corrected from 25mm × 19mm to 26mm × 17mm
- **MT3608**: Corrected from 36mm × 17mm × 10mm to 37mm × 17mm × 14mm
- **Toggle switch**: Changed from rectangular rocker (19mm × 13mm) to circular hole (6.2mm)

### Assembly Improvements
- **USB-C access**: Widened from 9mm × 3.5mm to 10mm × 4.5mm for easier cable insertion
- **Lid tolerance**: Increased from 0.3mm to 0.4mm for easier assembly
- **Speaker depth**: Added proper 20mm clearance specification
- **Ventilation**: Added heat dissipation slots for boost converter
- **OLED pocket**: Improved tolerances for easier installation

## Alternative: Simple Box Enclosure

If you prefer not to use this custom design, any project box around 100x80x50mm will work. You'll need to:
- Drill a 28mm hole for the arcade button
- Cut a window for the OLED (optional, can mount externally)
- Drill a 6mm hole for toggle switch (not rectangular rocker)
- Drill holes for USB charging and speaker sound
