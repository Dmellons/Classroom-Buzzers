// =============================================================================
// Classroom Buzzer Base Station - HANDHELD DESIGN
// Parametric OpenSCAD Design for ABS 3D Printing
// =============================================================================
//
// A compact handheld controller with:
//   - OLED display on the face (visible while holding)
//   - Start/Stop and Reset buttons on top (thumb accessible)
//   - Ergonomic grip shape
//   - USB-C port on bottom for charging/power
//   - SD card slot on side
//
// Usage:
//   1. Adjust parameters below to fit your components
//   2. Render (F6) then Export STL
//   3. Print front and back separately
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

// Overall dimensions (like a TV remote / game controller)
body_width = 70;          // X dimension (width in hand)
body_length = 130;        // Y dimension (length, top to bottom)
body_depth = 25;          // Z dimension (thickness)

// Wall thickness
wall = 2.5;               // Wall thickness for ABS durability

// Corner rounding
corner_radius = 8;        // Rounded corners for comfort
edge_radius = 3;          // Edge rounding for grip

// Shell split
shell_tolerance = 0.3;    // Gap between front and back shells
lip_depth = 2;            // How deep the alignment lip goes

// -----------------------------------------------------------------------------
// COMPONENT DIMENSIONS (measure your actual parts!)
// -----------------------------------------------------------------------------

// OLED Display (128x64, 0.96") - on the FACE
oled_window_w = 30;       // Visible window width
oled_window_h = 16;       // Visible window height
oled_pcb_w = 27;          // PCB width for mounting
oled_pcb_h = 27;          // PCB height
oled_pos_y = 85;          // Position from bottom (upper area)

// Control Buttons (on TOP face, thumb accessible)
button_hole_dia = 12;     // Button mounting hole diameter
button1_x = body_width/2 - 15;   // Start/Stop button X
button2_x = body_width/2 + 15;   // Reset button X
button_y = 45;            // Distance from bottom

// SD Card slot (side access)
sd_slot_w = 15;           // SD card opening width
sd_slot_h = 3;            // SD card opening height
sd_pos_y = 60;            // Position from bottom

// USB-C Port (bottom for power)
usbc_width = 12;          // USB-C opening width
usbc_height = 7;          // USB-C opening height

// ESP32-C6-DevKitC-1
esp32_length = 54;        // Length of devkit
esp32_width = 25.4;       // Width of devkit
esp32_height = 10;        // Height including components

// Status LED hole
led_hole_dia = 5;         // Status LED viewing hole
led_pos_y = 70;           // Below OLED

// -----------------------------------------------------------------------------
// MOUNTING PARAMETERS
// -----------------------------------------------------------------------------

post_dia = 5;             // Mounting post diameter
post_hole_dia = 2.2;      // Screw hole diameter (M2.5)
post_height = body_depth - wall*2 - 2;  // Post height

// -----------------------------------------------------------------------------
// ERGONOMIC PARAMETERS
// -----------------------------------------------------------------------------

// Grip contour (makes it comfortable to hold)
grip_indent = 3;          // How deep the grip curves inward
grip_start_y = 10;        // Where grip area starts
grip_end_y = 50;          // Where grip area ends

// Palm swell (back bulges slightly for comfort)
palm_swell = 4;           // Extra thickness at palm area
palm_center_y = 30;       // Center of palm swell

// -----------------------------------------------------------------------------
// CALCULATED VALUES
// -----------------------------------------------------------------------------

inner_width = body_width - wall*2;
inner_length = body_length - wall*2;
inner_depth = body_depth - wall*2;
half_depth = body_depth / 2;

// -----------------------------------------------------------------------------
// MODULES
// -----------------------------------------------------------------------------

// 2D rounded rectangle
module rounded_rect_2d(w, l, r) {
    hull() {
        translate([r, r]) circle(r=r, $fn=32);
        translate([w-r, r]) circle(r=r, $fn=32);
        translate([r, l-r]) circle(r=r, $fn=32);
        translate([w-r, l-r]) circle(r=r, $fn=32);
    }
}

// 3D rounded box with edge rounding
module rounded_body(w, l, d, corner_r, edge_r) {
    minkowski() {
        linear_extrude(height=d - edge_r*2)
            offset(r=-edge_r)
                rounded_rect_2d(w, l, corner_r);
        sphere(r=edge_r, $fn=24);
    }
}

// Mounting post
module mounting_post(height, outer_d, inner_d) {
    difference() {
        cylinder(h=height, d=outer_d, $fn=24);
        translate([0, 0, -1])
            cylinder(h=height+2, d=inner_d, $fn=16);
    }
}

// Screw hole (countersunk)
module screw_hole(depth, hole_d, head_d) {
    union() {
        // Shaft
        translate([0, 0, -1])
            cylinder(h=depth+2, d=hole_d, $fn=16);
        // Countersink
        translate([0, 0, depth - 2])
            cylinder(h=3, d1=hole_d, d2=head_d, $fn=16);
    }
}

// Button label recess
module button_label(w, h, depth) {
    translate([-w/2, -h/2, 0])
        cube([w, h, depth]);
}

// -----------------------------------------------------------------------------
// FRONT SHELL (with display)
// -----------------------------------------------------------------------------

module front_shell() {
    difference() {
        union() {
            // Main body - front half
            intersection() {
                rounded_body(body_width, body_length, body_depth, corner_radius, edge_radius);
                translate([-1, -1, -1])
                    cube([body_width+2, body_length+2, half_depth + lip_depth + 1]);
            }
        }

        // Hollow out inside
        translate([wall, wall, wall])
            cube([inner_width, inner_length, half_depth + lip_depth]);

        // --- CUTOUTS ---

        // OLED window (on face)
        translate([body_width/2 - oled_window_w/2, oled_pos_y - oled_window_h/2, -1])
            cube([oled_window_w, oled_window_h, wall + 2]);

        // OLED mounting recess (slightly larger than window)
        translate([body_width/2 - oled_window_w/2 - 2, oled_pos_y - oled_window_h/2 - 2, wall - 1])
            cube([oled_window_w + 4, oled_window_h + 4, 2]);

        // Status LED hole
        translate([body_width/2, led_pos_y, -1])
            cylinder(h=wall+2, d=led_hole_dia, $fn=16);

        // Button holes (on top surface - angled for ergonomics)
        translate([button1_x, button_y, -1])
            cylinder(h=wall+2, d=button_hole_dia, $fn=32);
        translate([button2_x, button_y, -1])
            cylinder(h=wall+2, d=button_hole_dia, $fn=32);

        // Button label recesses
        translate([button1_x, button_y - 12, wall - 0.8])
            button_label(14, 6, 1);
        translate([button2_x, button_y - 12, wall - 0.8])
            button_label(14, 6, 1);

        // Screw holes for assembly (4 corners)
        screw_inset = 8;
        translate([screw_inset, screw_inset, 0])
            screw_hole(half_depth, post_hole_dia, post_hole_dia + 3);
        translate([body_width - screw_inset, screw_inset, 0])
            screw_hole(half_depth, post_hole_dia, post_hole_dia + 3);
        translate([screw_inset, body_length - screw_inset, 0])
            screw_hole(half_depth, post_hole_dia, post_hole_dia + 3);
        translate([body_width - screw_inset, body_length - screw_inset, 0])
            screw_hole(half_depth, post_hole_dia, post_hole_dia + 3);
    }

    // OLED mounting posts (inside)
    oled_mount_w = oled_pcb_w + 4;
    oled_mount_h = oled_pcb_h + 4;
    translate([body_width/2 - oled_mount_w/2 + 2, oled_pos_y - oled_mount_h/2 + 2, wall])
        cube([2, oled_mount_h - 4, 3]);
    translate([body_width/2 + oled_mount_w/2 - 4, oled_pos_y - oled_mount_h/2 + 2, wall])
        cube([2, oled_mount_h - 4, 3]);
}

// -----------------------------------------------------------------------------
// BACK SHELL
// -----------------------------------------------------------------------------

module back_shell() {
    difference() {
        union() {
            // Main body - back half
            intersection() {
                rounded_body(body_width, body_length, body_depth, corner_radius, edge_radius);
                translate([-1, -1, half_depth - 1])
                    cube([body_width+2, body_length+2, half_depth + 2]);
            }

            // Alignment lip (fits into front shell)
            translate([wall + shell_tolerance, wall + shell_tolerance, half_depth - lip_depth])
                cube([inner_width - shell_tolerance*2, inner_length - shell_tolerance*2, lip_depth]);
        }

        // Hollow out inside (stops at back wall - keeps back enclosed)
        translate([wall, wall, half_depth])
            cube([inner_width, inner_length, half_depth - wall*2]);

        // Remove lip interior
        translate([wall*2, wall*2, half_depth - lip_depth - 1])
            cube([inner_width - wall*2, inner_length - wall*2, lip_depth + 2]);

        // --- CUTOUTS ---

        // USB-C port (bottom edge)
        translate([body_width/2 - usbc_width/2, -1, half_depth + (half_depth-wall)/2 - usbc_height/2])
            cube([usbc_width, wall + 2, usbc_height]);

        // SD card slot (right side)
        translate([body_width - wall - 1, sd_pos_y - sd_slot_w/2, half_depth + 3])
            cube([wall + 2, sd_slot_w, sd_slot_h]);

        // Ventilation slots (back)
        vent_count = 5;
        vent_spacing = 8;
        vent_length = 25;
        for (i = [0:vent_count-1]) {
            translate([body_width/2 - vent_length/2,
                       body_length/2 - (vent_count * vent_spacing)/2 + i * vent_spacing,
                       body_depth - wall - 1])
                cube([vent_length, 3, wall + 2]);
        }

        // Screw holes (matching front)
        screw_inset = 8;
        translate([screw_inset, screw_inset, half_depth - 1])
            cylinder(h=half_depth + 2, d=post_hole_dia, $fn=16);
        translate([body_width - screw_inset, screw_inset, half_depth - 1])
            cylinder(h=half_depth + 2, d=post_hole_dia, $fn=16);
        translate([screw_inset, body_length - screw_inset, half_depth - 1])
            cylinder(h=half_depth + 2, d=post_hole_dia, $fn=16);
        translate([body_width - screw_inset, body_length - screw_inset, half_depth - 1])
            cylinder(h=half_depth + 2, d=post_hole_dia, $fn=16);
    }

    // --- MOUNTING POSTS ---

    screw_inset = 8;
    post_h = half_depth - wall - 1;

    translate([screw_inset, screw_inset, half_depth])
        mounting_post(post_h, post_dia, post_hole_dia);
    translate([body_width - screw_inset, screw_inset, half_depth])
        mounting_post(post_h, post_dia, post_hole_dia);
    translate([screw_inset, body_length - screw_inset, half_depth])
        mounting_post(post_h, post_dia, post_hole_dia);
    translate([body_width - screw_inset, body_length - screw_inset, half_depth])
        mounting_post(post_h, post_dia, post_hole_dia);

    // ESP32 mounting rails
    esp_x = body_width/2 - esp32_width/2;
    esp_y = body_length/2 - esp32_length/2 + 10;

    translate([esp_x - 2, esp_y, half_depth])
        cube([3, esp32_length, 4]);
    translate([esp_x + esp32_width - 1, esp_y, half_depth])
        cube([3, esp32_length, 4]);
}

// -----------------------------------------------------------------------------
// RENDER OPTIONS
// -----------------------------------------------------------------------------

// Uncomment ONE of these to render:

// Option 1: Show assembled (for visualization)
// color("DimGray") front_shell();
// color("Gray") back_shell();

// Option 2: Front shell only (for printing - prints face DOWN)
// rotate([180, 0, 0]) translate([0, -body_length, -half_depth - lip_depth]) front_shell();

// Option 3: Back shell only (for printing - prints back DOWN)
// translate([0, 0, -half_depth + 1]) back_shell();

// Option 4: Both parts side by side (for single print plate)
rotate([180, 0, 0]) translate([0, -body_length, -half_depth - lip_depth]) front_shell();
translate([body_width + 10, 0, -half_depth + 1]) back_shell();

// Option 5: Exploded view
// color("DimGray") front_shell();
// color("Gray") translate([0, 0, 20]) back_shell();
