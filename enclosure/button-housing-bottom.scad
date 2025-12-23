// Classroom Buzzer Button Housing - BOTTOM HALF ONLY
// Parametric OpenSCAD Design for ABS 3D Printing
// =============================================================================
//
// This file renders ONLY the bottom portion for easy STL export.
// Open in OpenSCAD, press F6 to render, then Export as STL.
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
lid_tolerance = 0.3;      // Gap for easy fit

// -----------------------------------------------------------------------------
// COMPONENT DIMENSIONS (measure your actual parts!)
// -----------------------------------------------------------------------------

// OLED Display (0.91" 128x32) - Actual size: 23.1mm x 12mm x 12mm
oled_window_w = 24;       // Visible window width (display is 23.1mm + tolerance)
oled_window_h = 12;       // Visible window height (display is 12mm)
oled_depth = 12;          // Display depth/thickness
oled_offset_z = 10;       // Distance from bottom of front face

// Speaker (40mm, 3W)
speaker_dia = 40;         // Speaker diameter
speaker_grill_holes = 3;  // Grill hole diameter

// USB-C Port (for charging)
usbc_width = 9;           // USB-C opening width
usbc_height = 3.5;        // USB-C opening height
usbc_offset_z = 5;        // Height from bottom

// Power Rocker Switch (0.83" x 0.59" x 0.87" = 21mm x 15mm x 22mm)
// Typical panel cutout for this size rocker is ~19mm x 13mm
rocker_cutout_w = 19;     // Rocker switch panel cutout width
rocker_cutout_h = 13;     // Rocker switch panel cutout height
rocker_offset_z = 18;     // Height from bottom (centered on side wall)

// ESP32-C6-DevKitC-1
esp32_length = 54;        // Length of devkit
esp32_width = 25.4;       // Width of devkit

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

        // Power rocker switch (left side, X=0) - rectangular cutout
        translate([-1, outer_depth/2 - rocker_cutout_w/2, wall + rocker_offset_z - rocker_cutout_h/2])
            cube([wall+2, rocker_cutout_w, rocker_cutout_h]);
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

    // Left rail
    translate([outer_width/2 - oled_window_w/2 - 3, wall, wall + oled_offset_z - 2])
        cube([3, oled_pocket_depth, oled_window_h + 4]);
    // Right rail
    translate([outer_width/2 + oled_window_w/2, wall, wall + oled_offset_z - 2])
        cube([3, oled_pocket_depth, oled_window_h + 4]);
    // Bottom ledge
    translate([outer_width/2 - oled_window_w/2 - 3, wall, wall + oled_offset_z - 2])
        cube([oled_window_w + 6, oled_pocket_depth, 2]);

    // TP4056 USB-C charging module mount (25mm x 19mm) - near USB-C port at back
    // Rails to hold module so USB port aligns with cutout in back wall
    tp_x = wall + 12;
    tp_y = outer_depth - wall - 24;  // Near back wall
    translate([tp_x, tp_y, wall])
        cube([3, 22, 3]);
    translate([tp_x + 22, tp_y, wall])
        cube([3, 22, 3]);
}

// -----------------------------------------------------------------------------
// RENDER - BOTTOM HALF
// -----------------------------------------------------------------------------

bottom_half();
