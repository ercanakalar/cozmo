// ===========================================================================
//  Lift towers: stand on the cover wings and carry the 4-bar lift pivots.
//  side = "right": holds the lift servo (MG90S); its spline drives the lower
//                  link directly through the outer plate.
//  side = "left":  two M3 pivots for the passive links.
//  Print lying on the outer plate (largest flat face), no supports.
// ===========================================================================
include <params.scad>

side = "right";

zb = tub_top + 2;                  // wing top (52)
px = lift_pivot_x;
zl = lift_pivot_lo_z;
zh = lift_pivot_hi_z;
ear_y = [51, 53];                  // outer plate (|y|)
inner_y = [43.5, 46.5];            // servo tab plate (|y|)
x_range = [px - 12, px + 23];      // [-37, -2]
top_z = zh + 6;

// Servo body (frame of the robot, right side = -y, spline pointing -y)
servo_body_x = [px - servo_shaft_off, px - servo_shaft_off + servo_l];
servo_tab_xc = (servo_body_x[0] + servo_body_x[1]) / 2;

module frame() {
  // Outer plate with pivot bosses
  translate([x_range[0], ear_y[0], zb]) cube([x_range[1] - x_range[0], ear_y[1] - ear_y[0], top_z - zb]);
  for (z = (side == "right") ? [zh] : [zl, zh])
    translate([px, ear_y[0], z]) rotate([-90, 0, 0]) cylinder(d = 9, h = lift_link_y - lift_link_t / 2 - ear_y[0] - 0.3);
  // Inner plate
  translate([x_range[0], inner_y[0], zb]) cube([x_range[1] - x_range[0], inner_y[1] - inner_y[0], top_z - zb]);
  // End walls + roof tie the plates together
  for (x = [x_range[0], x_range[1] - 2.5])
    translate([x, inner_y[0], zb]) cube([2.5, ear_y[1] - inner_y[0], top_z - zb]);
  translate([x_range[0], inner_y[0], top_z - 2.5]) cube([x_range[1] - x_range[0], ear_y[1] - inner_y[0], 2.5]);
  // Feet with M3 holes (match the cover wing holes)
  translate([x_range[0], 48.5, zb]) cube([x_range[1] - x_range[0], ear_y[1] - 48.5, 3]);
}

module tower_right_frame() {
  difference() {
    frame();
    // Servo body passes through the inner plate (tabs rest on its -inner face)
    translate([servo_body_x[0] - clr, inner_y[0] - 1, zl - servo_w / 2 - clr])
      cube([servo_l + 2 * clr, inner_y[1] - inner_y[0] + 2, servo_w + 2 * clr]);
    // Tab screws
    for (dx = [-13.9, 13.9])
      translate([servo_tab_xc + dx, inner_y[0] - 1, zl]) rotate([-90, 0, 0]) cylinder(d = m2_tap, h = 6, $fn = 16);
    // Spline + horn hub clearance through the outer plate
    translate([px, ear_y[0] - 1, zl]) rotate([-90, 0, 0]) cylinder(d = 9, h = 5);
    // The feet and end wall must not block the servo body
    translate([servo_body_x[0] - clr, inner_y[0] + eps, zb - 1]) cube([servo_l + 2 * clr, 2, zl - zb + 7]);
    common_holes();
  }
}

module tower_left_frame() {
  difference() {
    frame();
    for (z = [zl, zh]) translate([px, ear_y[0] - 1, z]) rotate([-90, 0, 0]) cylinder(d = m3_tap, h = 20);
    common_holes();
  }
}

module common_holes() {
  // Upper pivot
  translate([px, ear_y[0] - 1, zh]) rotate([-90, 0, 0]) cylinder(d = m3_tap, h = 20);
  // Feet screws (self-tap from below through the cover)
  for (x = [px - 8.5, px + 19.5]) translate([x, 51, zb - 1]) cylinder(d = m3_tap, h = 8);
  // Cable pass for the servo lead
  translate([x_range[1] - 7, inner_y[0] - 1, zb + 3.5]) cube([4, 12, 4]);
}

// Built for the +y (left) side; the right tower is mirrored to -y.
if (side == "left") {
  tower_left_frame();
} else {
  mirror([0, 1, 0]) tower_right_frame();
}
