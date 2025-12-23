// =============================================================================
// Base Station Handheld - FRONT SHELL ONLY
// Ready to export as STL - prints face DOWN
// =============================================================================

// -----------------------------------------------------------------------------
// PARAMETERS (must match back shell!)
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
oled_window_w = 30;
oled_window_h = 16;
oled_pcb_w = 27;
oled_pcb_h = 27;
oled_pos_y = 85;

button_hole_dia = 12;
button1_x = body_width/2 - 15;
button2_x = body_width/2 + 15;
button_y = 45;

led_hole_dia = 5;
led_pos_y = 70;

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

module screw_hole(depth, hole_d, head_d) {
    union() {
        translate([0, 0, -1])
            cylinder(h=depth+2, d=hole_d, $fn=16);
        translate([0, 0, depth - 2])
            cylinder(h=3, d1=hole_d, d2=head_d, $fn=16);
    }
}

module button_label(w, h, depth) {
    translate([-w/2, -h/2, 0])
        cube([w, h, depth]);
}

// -----------------------------------------------------------------------------
// FRONT SHELL
// -----------------------------------------------------------------------------

module front_shell() {
    difference() {
        union() {
            intersection() {
                rounded_body(body_width, body_length, body_depth, corner_radius, edge_radius);
                translate([-1, -1, -1])
                    cube([body_width+2, body_length+2, half_depth + lip_depth + 1]);
            }
        }

        translate([wall, wall, wall])
            cube([inner_width, inner_length, half_depth + lip_depth]);

        // OLED window
        translate([body_width/2 - oled_window_w/2, oled_pos_y - oled_window_h/2, -1])
            cube([oled_window_w, oled_window_h, wall + 2]);

        // OLED recess
        translate([body_width/2 - oled_window_w/2 - 2, oled_pos_y - oled_window_h/2 - 2, wall - 1])
            cube([oled_window_w + 4, oled_window_h + 4, 2]);

        // LED hole
        translate([body_width/2, led_pos_y, -1])
            cylinder(h=wall+2, d=led_hole_dia, $fn=16);

        // Button holes
        translate([button1_x, button_y, -1])
            cylinder(h=wall+2, d=button_hole_dia, $fn=32);
        translate([button2_x, button_y, -1])
            cylinder(h=wall+2, d=button_hole_dia, $fn=32);

        // Button labels
        translate([button1_x, button_y - 12, wall - 0.8])
            button_label(14, 6, 1);
        translate([button2_x, button_y - 12, wall - 0.8])
            button_label(14, 6, 1);

        // Screw holes
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

    // OLED mounts
    oled_mount_w = oled_pcb_w + 4;
    oled_mount_h = oled_pcb_h + 4;
    translate([body_width/2 - oled_mount_w/2 + 2, oled_pos_y - oled_mount_h/2 + 2, wall])
        cube([2, oled_mount_h - 4, 3]);
    translate([body_width/2 + oled_mount_w/2 - 4, oled_pos_y - oled_mount_h/2 + 2, wall])
        cube([2, oled_mount_h - 4, 3]);
}

// -----------------------------------------------------------------------------
// RENDER - Flipped for printing (face down on bed)
// -----------------------------------------------------------------------------

rotate([180, 0, 0]) translate([0, -body_length, -half_depth - lip_depth]) front_shell();
