// ===========================================================================
//  Track wheels. drive = true: rear sprocket pressed on the N20 D-shaft.
//  drive = false: front idler spinning on an M3 screw.
//  Print 2 of each, flat side down, 100 % infill for the drive hub.
// ===========================================================================
include <params.scad>

drive = true;

hub_r_flat = r_pin * cos(180 / teeth) - link_t / 2 - 0.3;   // plate inner face
hub_r = hub_r_flat / cos(180 / teeth);                       // 12-gon circumradius
tooth = [2.5, 4.6];                                          // tangential, axial
tooth_h = link_t + 0.4;
width = link_w;

module tooth_shape() {
  // Tapered tooth from the hub flat outwards (along +x).
  hull() {
    translate([hub_r_flat - 0.5, -tooth[0] / 2, -tooth[1] / 2]) cube([0.5, tooth[0], tooth[1]]);
    translate([hub_r_flat + tooth_h - 0.4, -tooth[0] / 2 + 0.5, -tooth[1] / 2 + 0.6])
      cube([0.4, tooth[0] - 1.0, tooth[1] - 1.2]);
  }
}

module d_shaft_hole() {
  intersection() {
    cylinder(d = n20_shaft_d + 0.1, h = 40, center = true, $fn = 32);
    translate([-(n20_shaft_d + 1) / 2, -(n20_shaft_d + 1) / 2, -20])
      cube([n20_shaft_flat + 0.05 + (n20_shaft_d + 1) / 2 - n20_shaft_d / 2, n20_shaft_d + 1, 40]);
  }
}

module sprocket() {
  difference() {
    union() {
      // 12-sided hub: link plates sit on the flats
      rotate([0, 0, 0]) cylinder(r = hub_r, h = width, $fn = teeth);
      // Teeth centred on the flats, at mid-width
      for (i = [0 : teeth - 1])
        rotate([0, 0, 180 / teeth + i * 360 / teeth])
          translate([0, 0, width / 2]) tooth_shape();
      // Longer hub on the motor side for more shaft engagement
      if (drive) translate([0, 0, -1]) cylinder(d = 9, h = 1 + eps);
    }
    if (drive) {
      translate([0, 0, -1]) d_shaft_hole();
      // M2 grub screw / self-tapping screw onto the shaft flat
      translate([0, 0, 4]) rotate([0, 90, 0]) cylinder(d = m2_tap, h = hub_r + 1, $fn = 16);
    } else {
      translate([0, 0, -2]) cylinder(d = m3_hole + 0.1, h = width + 4);
    }
    // Lightening holes
    for (i = [0 : 5]) rotate([0, 0, i * 60 + 30])
      translate([8, 0, -1]) cylinder(d = 4.2, h = width + 2, $fn = 24);
  }
}

sprocket();
