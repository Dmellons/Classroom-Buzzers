// =============================================================================
// Base Station Handheld - BACK SHELL ONLY
// Ready to export as STL - prints back DOWN
// =============================================================================

// -----------------------------------------------------------------------------
// PARAMETERS (must match front shell!)
// -----------------------------------------------------------------------------

body_width = 70;
body_length = 130;
body_depth = 25;
wall = 2.5;
corner_radius = 8;
edge_radius = 3;
shell_tolerance = 0.3;
lip_depth = 2;

// Components
usbc_width = 12;
usbc_height = 7;

sd_slot_w = 15;
sd_slot_h = 3;
sd_pos_y = 60;

esp32_length = 54;
esp32_width = 25.4;

post_dia = 5;
post_hole_dia = 2.2;

// Calculated
inner_width = body_width - wall*2;
inner_length = body_length - wall*2;
half_depth = body_depth / 2;

// -----------------------------------------------------------------------------
// MODULES
// -----------------------------------------------------------------------------

module rounded_rect_2d(w, l, r) {
    hull() {
        translate([r, r]) circle(r=r, $fn=32);
        translate([w-r, r]) circle(r=r, $fn=32);
        translate([r, l-r]) circle(r=r, $fn=32);
        translate([w-r, l-r]) circle(r=r, $fn=32);
    }
}

module rounded_body(w, l, d, corner_r, edge_r) {
    minkowski() {
        linear_extrude(height=d - edge_r*2)
            offset(r=-edge_r)
                rounded_rect_2d(w, l, corner_r);
        sphere(r=edge_r, $fn=24);
    }
}

module mounting_post(height, outer_d, inner_d) {
    difference() {
        cylinder(h=height, d=outer_d, $fn=24);
        translate([0, 0, -1])
            cylinder(h=height+2, d=inner_d, $fn=16);
    }
}

// -----------------------------------------------------------------------------
// BACK SHELL
// -----------------------------------------------------------------------------

module back_shell() {
    difference() {
        union() {
            intersection() {
                rounded_body(body_width, body_length, body_depth, corner_radius, edge_radius);
                translate([-1, -1, half_depth - 1])
                    cube([body_width+2, body_length+2, half_depth + 2]);
            }

            // Alignment lip
            translate([wall + shell_tolerance, wall + shell_tolerance, half_depth - lip_depth])
                cube([inner_width - shell_tolerance*2, inner_length - shell_tolerance*2, lip_depth]);
        }

        // Hollow inside (stops at back wall - keeps back enclosed)
        translate([wall, wall, half_depth])
            cube([inner_width, inner_length, half_depth - wall*2]);

        // Lip interior
        translate([wall*2, wall*2, half_depth - lip_depth - 1])
            cube([inner_width - wall*2, inner_length - wall*2, lip_depth + 2]);

        // USB-C port (bottom)
        translate([body_width/2 - usbc_width/2, -1, half_depth + (half_depth-wall)/2 - usbc_height/2])
            cube([usbc_width, wall + 2, usbc_height]);

        // SD card slot (right side)
        translate([body_width - wall - 1, sd_pos_y - sd_slot_w/2, half_depth + 3])
            cube([wall + 2, sd_slot_w, sd_slot_h]);

        // Ventilation
        vent_count = 5;
        vent_spacing = 8;
        vent_length = 25;
        for (i = [0:vent_count-1]) {
            translate([body_width/2 - vent_length/2,
                       body_length/2 - (vent_count * vent_spacing)/2 + i * vent_spacing,
                       body_depth - wall - 1])
                cube([vent_length, 3, wall + 2]);
        }

        // Screw holes
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

    // Mounting posts
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

    // ESP32 rails
    esp_x = body_width/2 - esp32_width/2;
    esp_y = body_length/2 - esp32_length/2 + 10;

    translate([esp_x - 2, esp_y, half_depth])
        cube([3, esp32_length, 4]);
    translate([esp_x + esp32_width - 1, esp_y, half_depth])
        cube([3, esp32_length, 4]);
}

// -----------------------------------------------------------------------------
// RENDER - Positioned for printing (back down on bed)
// -----------------------------------------------------------------------------

translate([0, 0, -half_depth + 1]) back_shell();
