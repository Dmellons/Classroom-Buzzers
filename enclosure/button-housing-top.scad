// =============================================================================
// Classroom Buzzer Button Housing - TOP HALF ONLY
// Parametric OpenSCAD Design for ABS 3D Printing
// =============================================================================
//
// This file renders ONLY the top/lid portion for easy STL export.
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
// COMPONENT DIMENSIONS
// -----------------------------------------------------------------------------

// Arcade Button (typically 28mm mounting hole)
button_hole_dia = 28;     // Mounting hole diameter

// Note: OLED and rocker switch are in the bottom half only
// This file just needs matching overall dimensions for proper fit

// -----------------------------------------------------------------------------
// MOUNTING POST PARAMETERS
// -----------------------------------------------------------------------------

post_dia = 6;             // Mounting post diameter
post_hole_dia = 2.5;      // Screw hole diameter (M3)

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
// RENDER - TOP HALF (flipped for printing)
// -----------------------------------------------------------------------------

// Flipped so flat surface is on print bed
translate([0, 0, lid_height]) rotate([180, 0, 0]) top_half();
