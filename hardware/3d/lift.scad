// ===========================================================================
//  Lift mechanism (parallel 4-bar like the original toy's forklift):
//    part = "link"       plain link, print 3
//    part = "link_servo" lower right link, screws onto a single-arm servo horn
//    part = "fork"       both carriers + crossbar + forks in one piece
//  Links: print flat. Fork: print standing on its back face (x = -2 side).
// ===========================================================================
include <params.scad>

part = "fork";

L = lift_link_len;
pin_dz = lift_pivot_hi_z - lift_pivot_lo_z;  // 12

module link_body() {
  hull() for (x = [0, L]) translate([x, 0, 0]) cylinder(d = lift_link_h + 2, h = lift_link_t);
}

module link() {
  difference() {
    link_body();
    for (x = [0, L]) translate([x, 0, -1]) cylinder(d = m3_hole, h = lift_link_t + 2);
    // lightening slot
    hull() for (x = [14, L - 14]) translate([x, 0, -1]) cylinder(d = 3, h = lift_link_t + 2);
  }
}

module link_servo() {
  difference() {
    union() {
      link_body();
      translate([0, 0, 0]) cylinder(d = 14, h = lift_link_t);
    }
    // Horn screw / hub
    translate([0, 0, -1]) cylinder(d = 2.6, h = lift_link_t + 2);
    translate([0, 0, lift_link_t - 1.6]) cylinder(d = 7.6, h = 2);  // horn hub recess
    // Single-arm horn holes (SG90/MG90S arm: 3.5 mm pitch from 10 mm)
    for (x = [10.5, 14, 17.5]) translate([x, 0, -1]) cylinder(d = 1.5, h = lift_link_t + 2, $fn = 12);
    // Horn arm recess (glue + screws)
    translate([0, -3.2, lift_link_t - 1.6]) cube([20, 6.4, 2]);
    translate([L, 0, -1]) cylinder(d = m3_hole, h = lift_link_t + 2);
    hull() for (x = [24, L - 14]) translate([x, 0, -1]) cylinder(d = 3, h = lift_link_t + 2);
  }
}

// Fork assembly, frame: origin on the lower carrier pin, x forward.
carrier_y = [lift_link_y - lift_link_t / 2 - 3.3, lift_link_y - lift_link_t / 2 - 0.3];  // just inside the links
bar_z = [-carrier_drop, -carrier_drop + 8];

module carrier(s) {
  y0 = s > 0 ? carrier_y[0] : -carrier_y[1];
  difference() {
    hull() {
      translate([-2, y0, bar_z[0]]) cube([5, carrier_y[1] - carrier_y[0], 1]);
      for (z = [0, pin_dz]) translate([0, y0, z]) rotate([-90, 0, 0]) cylinder(d = 9, h = carrier_y[1] - carrier_y[0]);
    }
    for (z = [0, pin_dz]) translate([0, y0 - 1, z]) rotate([-90, 0, 0]) cylinder(d = m3_tap, h = 10);
  }
}

module fork() {
  for (s = [-1, 1]) carrier(s);
  // Crossbar
  translate([-2, -carrier_y[1], bar_z[0]]) cube([5, 2 * carrier_y[1], bar_z[1] - bar_z[0]]);
  // Forks with lifting hooks
  for (s = [-1, 1]) translate([3 - eps, s * fork_y - 2.5, bar_z[0]]) {
    cube([fork_len, 5, 3.5]);
    translate([fork_len - 3, 0, 0]) cube([3, 5, 7]);
  }
  // Stiffening rib on the crossbar top
  translate([-2, -carrier_y[0], bar_z[1] - eps]) cube([2, 2 * carrier_y[0], 4]);
}

if (part == "link") link();
else if (part == "link_servo") link_servo();
else rotate([0, -90, 0]) translate([2, 0, carrier_drop]) fork();  // print orientation: back face down
