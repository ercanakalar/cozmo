// ===========================================================================
//  Track link - 26 per side (52 total). Joined with 1.75 mm filament pins.
//
//  Local frame: hinge pins at x = 0 (pin A) and x = pitch (pin B),
//  z = 0 is the ground side (grousers below), z = link_t faces the wheels.
//  Pin A carries the centre + outer knuckles, pin B the inner side knuckles,
//  so consecutive links interleave. The rectangular window in the middle
//  takes the sprocket tooth.
//
//  Print: flat as exported, 0.2 mm layers, 3 perimeters, PLA/PETG.
//  After printing push a filament piece through, cut flush, and melt the
//  ends slightly with a soldering iron to lock them.
// ===========================================================================
include <params.scad>

part = "plate";          // "single" or "plate"
plate_count = 13;        // links per print plate (4 plates = both tracks)

gap = 0.4;               // knuckle side gap
half = link_w / 2;       // 8
centre_half = 3;         // centre knuckle |y| <= 3
side_split = 5.5;        // side zone split between inner (B) and outer (A)
r_clear = link_t / 2 + 0.45;
window = [3.2, 5.4];     // sprocket tooth window (x, y)

// Knuckle y ranges [y0, y1]
a_ranges = [[-centre_half, centre_half], [side_split + gap, half], [-half, -side_split - gap]];
b_ranges = [[centre_half + gap, side_split], [-side_split, -centre_half - gap]];

module barrel(x, y0, y1) {
  translate([x, y0, link_t / 2]) rotate([-90, 0, 0]) cylinder(d = link_t, h = y1 - y0, $fn = 32);
}

module clearance(x, y0, y1) {
  translate([x, y0 - gap / 2, link_t / 2]) rotate([-90, 0, 0])
    cylinder(r = r_clear, h = y1 - y0 + gap, $fn = 32);
}

module track_link() {
  difference() {
    union() {
      // Knuckles + their plate segments
      for (r = a_ranges) {
        barrel(0, r[0], r[1]);
        translate([0, r[0], 0]) cube([pitch - r_clear, r[1] - r[0], link_t]);
      }
      for (r = b_ranges) {
        barrel(pitch, r[0], r[1]);
        translate([r_clear, r[0], 0]) cube([pitch - r_clear, r[1] - r[0], link_t]);
      }
      // Continuous middle web ties every knuckle together
      translate([r_clear, -half, 0]) cube([pitch - 2 * r_clear, link_w, link_t]);
      // Grousers (tread ridges) on the ground side, left and right of the window
      for (s = [-1, 1])
        translate([pitch / 2 - 1.1, s > 0 ? window[1] / 2 + 0.6 : -half, -grouser_h])
          cube([2.2, half - window[1] / 2 - 0.6, grouser_h + eps]);
    }
    // Neighbouring links' knuckles swing through these spaces
    for (r = b_ranges) clearance(0, r[0], r[1]);
    for (r = a_ranges) clearance(pitch, r[0], r[1]);
    // Pin holes
    for (x = [0, pitch])
      translate([x, -half - 1, link_t / 2]) rotate([-90, 0, 0]) cylinder(d = pin_hole, h = link_w + 2, $fn = 20);
    // Sprocket tooth window
    translate([pitch / 2 - window[0] / 2, -window[1] / 2, -grouser_h - 1]) cube([window[0], window[1], link_t + grouser_h + 2]);
  }
}

// Printed with the grousers up so the wheel side is flat on the bed.
module printable_link() {
  translate([0, 0, link_t]) mirror([0, 0, 1]) track_link();
}

if (part == "single") {
  printable_link();
} else {
  for (i = [0 : plate_count - 1])
    translate([0, i * (link_w + 4), 0]) printable_link();
}
