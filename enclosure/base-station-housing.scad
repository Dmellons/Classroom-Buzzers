// =============================================================================
// Classroom Buzzer Base Station Housing
// Parametric OpenSCAD Design for ABS 3D Printing
// =============================================================================
//
// Usage:
//   1. Adjust parameters below to fit your components
//   2. Render (F6) then Export STL
//   3. Print top and bottom separately (flip top for printing)
//
// ABS Print Settings:
//   - Nozzle: 230-250°C
//   - Bed: 90-110°C
//   - Enclosed printer recommended
//   - Infill: 20-30%
//   - Layer height: 0.2mm
//
// =============================================================================

// -----------------------------------------------------------------------------
// MAIN ENCLOSURE PARAMETERS
// -----------------------------------------------------------------------------

// Overall box dimensions (internal)
box_width = 100;          // X dimension
box_depth = 80;           // Y dimension
box_height = 35;          // Z dimension

// Wall thickness
wall = 2.5;               // Wall thickness for ABS durability

// Corner rounding
corner_radius = 5;        // Rounded corners

// Lid parameters
lid_height = 12;          // How tall the top portion is
lid_lip = 2;              // Overlap lip for top/bottom fit
lid_tolerance = 0.4;      // Gap for easy fit (increased from 0.3mm for easier assembly)

// -----------------------------------------------------------------------------
// COMPONENT DIMENSIONS (measure your actual parts!)
// -----------------------------------------------------------------------------

// OLED Display (128x64, 0.96") - Verified: PCB 27x27mm, Active 22x12mm
oled_window_w = 23;       // Visible window width (22mm active + 1mm margin)
oled_window_h = 13;       // Visible window height (12mm active + 1mm margin)
oled_pcb_w = 27;          // PCB width (verified)
oled_pcb_h = 27;          // PCB height (verified)
oled_offset_x = 10;       // Distance from left edge
oled_offset_z = 8;        // Distance from bottom

// SD Card Module
sd_slot_w = 30;           // SD module width
sd_slot_h = 5;            // SD module height
sd_offset_z = 5;          // Height from bottom

// USB-C Port (ESP32 power) - Enlarged for easier cable access
usbc_width = 12;          // USB-C opening width (verified adequate)
usbc_height = 8;          // USB-C opening height (verified adequate)
usbc_offset_z = 3;        // Height from bottom

// Control Buttons (Start/Stop and Reset)
button_hole_dia = 12;     // Button mounting hole diameter
button_spacing = 25;      // Distance between buttons

// Ventilation slots
vent_width = 3;           // Individual vent slot width
vent_length = 20;         // Vent slot length
vent_count = 4;           // Number of vents
vent_spacing = 6;         // Space between vents

// ESP32-C6-DevKitC-1 - Verified from official Espressif dimensions
esp32_length = 51.8;      // Length of devkit (corrected from 54mm)
esp32_width = 25.4;       // Width of devkit
esp32_height = 10;        // Height including components

// -----------------------------------------------------------------------------
// MOUNTING POST PARAMETERS
// -----------------------------------------------------------------------------

post_dia = 6;             // Mounting post diameter
post_hole_dia = 2.5;      // Screw hole diameter (M3)
post_height = 5;          // Post height from floor

// -----------------------------------------------------------------------------
// CALCULATED VALUES (don't modify)
// -----------------------------------------------------------------------------

outer_width = box_width + wall * 2;
outer_depth = box_depth + wall * 2;
outer_height = box_height + wall * 2;
bottom_height = outer_height - lid_height;

// -----------------------------------------------------------------------------
// MODULES
// -----------------------------------------------------------------------------

// Rounded box helper
module rounded_box(w, d, h, r) {
    hull() {
        translate([r, r, 0]) cylinder(h=h, r=r, $fn=32);
        translate([w-r, r, 0]) cylinder(h=h, r=r, $fn=32);
        translate([r, d-r, 0]) cylinder(h=h, r=r, $fn=32);
        translate([w-r, d-r, 0]) cylinder(h=h, r=r, $fn=32);
    }
}

// Ventilation slots
module vent_slots() {
    for (i = [0:vent_count-1]) {
        translate([0, i * vent_spacing, 0])
            cube([vent_length, vent_width, wall * 2]);
    }
}

// Mounting post
module mounting_post(height) {
    difference() {
        cylinder(h=height, d=post_dia, $fn=24);
        translate([0, 0, -1])
            cylinder(h=height+2, d=post_hole_dia, $fn=16);
    }
}

// -----------------------------------------------------------------------------
// BOTTOM HALF
// -----------------------------------------------------------------------------

module bottom_half() {
    difference() {
        // Outer shell
        rounded_box(outer_width, outer_depth, bottom_height, corner_radius);

        // Hollow out inside
        translate([wall, wall, wall])
            rounded_box(box_width, box_depth, bottom_height, corner_radius - wall/2);

        // --- CUTOUTS ---

        // OLED window (front face, Y=0)
        translate([wall + oled_offset_x, -1, wall + oled_offset_z])
            cube([oled_window_w, wall+2, oled_window_h]);

        // USB-C port (back face, Y=max) - for ESP32 power
        translate([outer_width/2 - usbc_width/2, outer_depth - wall - 1, wall + usbc_offset_z])
            cube([usbc_width, wall+2, usbc_height]);

        // SD card slot (back face, offset to side)
        translate([outer_width - wall - sd_slot_w - 10, outer_depth - wall - 1, wall + sd_offset_z])
            cube([sd_slot_w, wall+2, sd_slot_h]);

        // Ventilation slots (right side)
        translate([outer_width - wall/2 - vent_length/2, outer_depth/2 - (vent_count * vent_spacing)/2, wall + 5])
            rotate([0, 0, 0])
                vent_slots();

        // Ventilation slots (left side)
        translate([-wall/2 + vent_length/2, outer_depth/2 - (vent_count * vent_spacing)/2, wall + 5])
            rotate([0, 0, 0])
                vent_slots();
    }

    // --- MOUNTING POSTS ---

    // Corner posts for lid screws
    post_inset = 8;
    translate([wall + post_inset, wall + post_inset, wall])
        mounting_post(post_height);
    translate([outer_width - wall - post_inset, wall + post_inset, wall])
        mounting_post(post_height);
    translate([wall + post_inset, outer_depth - wall - post_inset, wall])
        mounting_post(post_height);
    translate([outer_width - wall - post_inset, outer_depth - wall - post_inset, wall])
        mounting_post(post_height);

    // --- COMPONENT MOUNTING ---

    // OLED mounting ledge
    translate([wall + oled_offset_x - 2, wall, wall])
        cube([oled_pcb_w + 4, 4, oled_offset_z]);

    // ESP32 mounting rails (updated for correct 51.8mm length)
    esp_x = wall + 10;
    esp_y = wall + 25;
    translate([esp_x, esp_y, wall])
        cube([3, esp32_length + 2, 4]);
    translate([esp_x + esp32_width + 2, esp_y, wall])
        cube([3, esp32_length + 2, 4]);
}

// -----------------------------------------------------------------------------
// TOP HALF (LID)
// -----------------------------------------------------------------------------

module top_half() {
    difference() {
        union() {
            // Outer shell
            rounded_box(outer_width, outer_depth, lid_height, corner_radius);

            // Inner lip for fitting onto bottom
            translate([wall + lid_tolerance, wall + lid_tolerance, -lid_lip])
                rounded_box(box_width - lid_tolerance*2, box_depth - lid_tolerance*2,
                           lid_lip, corner_radius - wall/2);
        }

        // Hollow out inside
        translate([wall, wall, -1])
            rounded_box(box_width, box_depth, lid_height - wall + 1, corner_radius - wall/2);

        // --- CUTOUTS ---

        // Button holes (Start/Stop and Reset)
        button_y = outer_depth / 2;
        translate([outer_width/2 - button_spacing/2, button_y, -1])
            cylinder(h=lid_height + 2, d=button_hole_dia, $fn=32);
        translate([outer_width/2 + button_spacing/2, button_y, -1])
            cylinder(h=lid_height + 2, d=button_hole_dia, $fn=32);

        // Label recesses for buttons (optional decoration)
        translate([outer_width/2 - button_spacing/2 - 8, button_y - 15, lid_height - 0.5])
            cube([16, 8, 1]);
        translate([outer_width/2 + button_spacing/2 - 8, button_y - 15, lid_height - 0.5])
            cube([16, 8, 1]);

        // Screw holes for mounting posts
        post_inset = 8;
        translate([wall + post_inset, wall + post_inset, -lid_lip - 1])
            cylinder(h=lid_height + lid_lip + 2, d=post_hole_dia, $fn=16);
        translate([outer_width - wall - post_inset, wall + post_inset, -lid_lip - 1])
            cylinder(h=lid_height + lid_lip + 2, d=post_hole_dia, $fn=16);
        translate([wall + post_inset, outer_depth - wall - post_inset, -lid_lip - 1])
            cylinder(h=lid_height + lid_lip + 2, d=post_hole_dia, $fn=16);
        translate([outer_width - wall - post_inset, outer_depth - wall - post_inset, -lid_lip - 1])
            cylinder(h=lid_height + lid_lip + 2, d=post_hole_dia, $fn=16);
    }
}

// -----------------------------------------------------------------------------
// RENDER OPTIONS
// -----------------------------------------------------------------------------

// Uncomment ONE of these to render:

// Option 1: Show assembled (for visualization)
// color("DarkSlateGray") bottom_half();
// color("SlateGray") translate([0, 0, bottom_height + 5]) top_half();

// Option 2: Bottom half only (for printing)
// bottom_half();

// Option 3: Top half only (for printing - flip in slicer)
// translate([0, 0, lid_height]) rotate([180, 0, 0]) top_half();

// Option 4: Both parts side by side (for single print plate)
 bottom_half();
 translate([outer_width + 10, 0, lid_height]) rotate([180, 0, 0]) top_half();
