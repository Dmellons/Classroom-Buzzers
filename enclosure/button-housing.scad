// =============================================================================
// Classroom Buzzer Button Housing
// Parametric OpenSCAD Design for ABS 3D Printing
// =============================================================================
//
// Usage:
//   1. Adjust parameters below to fit your components
//   2. Render (F6) then Export STL
//   3. Print top and bottom separately (flip bottom for printing)
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
box_width = 80;           // X dimension
box_depth = 100;          // Y dimension
box_height = 45;          // Z dimension (not including button protrusion)

// Wall thickness
wall = 2.5;               // Wall thickness for ABS durability

// Corner rounding
corner_radius = 5;        // Rounded corners

// Lid parameters
lid_height = 15;          // How tall the top portion is
lid_lip = 2;              // Overlap lip for top/bottom fit
lid_tolerance = 0.4;      // Gap for easy fit (increased from 0.3mm for easier assembly)

// -----------------------------------------------------------------------------
// COMPONENT DIMENSIONS (measure your actual parts!)
// -----------------------------------------------------------------------------

// Arcade Button (typically 28mm mounting hole)
button_hole_dia = 28;     // Mounting hole diameter
button_depth = 20;        // How deep button body extends below panel

// OLED Display (0.91" 128x32) - Verified: PCB 38x12mm, Active area 22.4x5.6mm
oled_window_w = 23;       // Visible window width (active 22.4mm + 0.6mm tolerance)
oled_window_h = 12;       // Window height (module height is 12mm)
oled_depth = 12;          // Display depth/thickness
oled_offset_z = 10;       // Distance from bottom of front face

// Speaker (40mm, 3W 8Ω) - Verified: 40mm diameter x 20mm height
speaker_dia = 40;         // Speaker diameter
speaker_depth = 20;       // Speaker depth (verified from datasheet)
speaker_grill_holes = 3;  // Grill hole diameter

// USB-C Port (for charging) - Widened for easier cable insertion
usbc_width = 10;          // USB-C opening width (increased from 9mm)
usbc_height = 4.5;        // USB-C opening height (increased from 3.5mm)
usbc_offset_z = 5;        // Height from bottom

// Power Toggle Switch (SPST, 6mm mounting hole)
// Using toggle switch instead of rocker for simpler mounting
switch_hole_dia = 6.2;    // Toggle switch mounting hole (6mm + 0.2mm tolerance)
switch_offset_z = 18;     // Height from bottom (centered on side wall)

// ESP32-C6-DevKitC-1 - Verified from official Espressif dimensions
esp32_length = 51.8;      // Length of devkit (corrected from 54mm)
esp32_width = 25.4;       // Width of devkit
esp32_height = 10;        // Height including components (corrected from 8mm)

// Battery (503035 LiPo)
battery_length = 35;      // Battery length
battery_width = 30;       // Battery width
battery_height = 5;       // Battery thickness

// TP4056 Charger Module - Verified dimensions
tp4056_length = 26;       // Module length (corrected from 25mm)
tp4056_width = 17;        // Module width (corrected from 19mm)
tp4056_height = 4;        // Module height

// MT3608 Boost Converter - Verified dimensions
mt3608_length = 37;       // Module length (corrected from 36mm)
mt3608_width = 17;        // Module width
mt3608_height = 14;       // Module height including potentiometer (corrected from 10mm)

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

// Speaker grill pattern
module speaker_grill(dia, hole_size) {
    spacing = hole_size * 2;
    count = floor(dia / spacing) - 1;
    offset_val = (dia - (count * spacing)) / 2;

    for (x = [0:count]) {
        for (y = [0:count]) {
            cx = -dia/2 + offset_val + x * spacing;
            cy = -dia/2 + offset_val + y * spacing;
            if (sqrt(cx*cx + cy*cy) < dia/2 - hole_size) {
                translate([cx, cy, 0])
                    cylinder(h=wall*2, d=hole_size, $fn=16, center=true);
            }
        }
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
        translate([outer_width/2 - oled_window_w/2, -1, wall + oled_offset_z])
            cube([oled_window_w, wall+2, oled_window_h]);

        // Speaker grill (right side, X=max)
        translate([outer_width - wall/2, outer_depth/2, bottom_height/2])
            rotate([0, 90, 0])
                speaker_grill(speaker_dia, speaker_grill_holes);

        // USB-C port (back face, Y=max)
        translate([wall + 15, outer_depth - wall - 1, wall + usbc_offset_z])
            cube([usbc_width, wall+2, usbc_height]);

        // Power toggle switch (left side, X=0) - circular hole
        translate([-1, outer_depth/2, wall + switch_offset_z])
            rotate([0, 90, 0])
                cylinder(h=wall+2, d=switch_hole_dia, $fn=32);

        // Ventilation slots for boost converter (right side, near back)
        // Small slots for heat dissipation
        for (i = [0:3]) {
            translate([outer_width - wall/2 - 10, outer_depth - 25 - (i * 5), wall + 8])
                cube([wall+1, 2, 10], center=true);
        }
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

    // OLED friction-fit pocket (display slides in from inside)
    // Creates a U-shaped channel that holds the display against the window
    oled_pocket_depth = 5;  // How deep the pocket is
    oled_tol = 0.5;         // Tolerance for snug fit (0.5mm clearance on each side)

    // Left rail (with tolerance)
    translate([outer_width/2 - oled_window_w/2 - 3 - oled_tol, wall, wall + oled_offset_z - 2])
        cube([3, oled_pocket_depth, oled_window_h + 4]);
    // Right rail (with tolerance)
    translate([outer_width/2 + oled_window_w/2 + oled_tol, wall, wall + oled_offset_z - 2])
        cube([3, oled_pocket_depth, oled_window_h + 4]);
    // Bottom ledge (with tolerance)
    translate([outer_width/2 - oled_window_w/2 - 3 - oled_tol, wall, wall + oled_offset_z - 2])
        cube([oled_window_w + 6 + oled_tol*2, oled_pocket_depth, 2]);

    // TP4056 USB-C charging module mount (26mm x 17mm) - near USB-C port at back
    // Rails to hold module so USB port aligns with cutout in back wall
    tp_x = wall + 12;
    tp_y = outer_depth - wall - 20;  // Near back wall (adjusted for new 17mm width)
    translate([tp_x, tp_y, wall])
        cube([3, tp4056_width + 2, 3]);
    translate([tp_x + tp4056_length - 3, tp_y, wall])
        cube([3, tp4056_width + 2, 3]);
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

        // Arcade button hole (centered on top)
        translate([outer_width/2, outer_depth/2, -1])
            cylinder(h=lid_height + 2, d=button_hole_dia, $fn=64);

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
bottom_half();

// Option 3: Top half only (for printing - flip in slicer)
// translate([0, 0, lid_height]) rotate([180, 0, 0]) top_half();

// Option 4: Both parts side by side (for single print plate)
// bottom_half();
// translate([outer_width + 10, 0, lid_height]) rotate([180, 0, 0]) top_half();
