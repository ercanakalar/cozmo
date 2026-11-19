// ===========================================================================
//  Chassis tub: holds both N20 motors (rear), the LiPo (front, on the
//  floor), the electronics tray (ledges at z = 24.5), speaker + USB-C +
//  power switch on the rear wall, idler axle slots at the front.
//  Print upright, open side up, no supports needed (bridges only).
// ===========================================================================
include <params.scad>

x0 = -tub_len / 2;
y0 = -tub_w / 2;
ix = tub_len / 2 - wall;   // inner half length (41)
iy = tub_w / 2 - wall;     // inner half width (33)
tray_z = 24.5;             // underside of the electronics tray
boss_xy = [tub_len / 2 - 4.5, tub_w / 2 - 4.5];
speaker_pos = [14, 34];    // (y, z) on the rear wall
usbc_pos = [-16, 29.7];
switch_pos = [-27, 41];

module shell() {
  difference() {
    translate([x0, y0, tub_z0]) rounded_box([tub_len, tub_w, tub_h], tub_r);
    translate([x0 + wall, y0 + wall, tub_z0 + floor_t]) rounded_box([tub_len - 2 * wall, tub_w - 2 * wall, tub_h], tub_r - wall);
  }
}

module motor_cradle(side) {
  // U-shaped cradle under each motor + zip-tie slot.
  y_face = side * (tub_w / 2);                  // gearbox face flush with outer wall
  y_in = y_face - side * n20_len;
  cz = axle_z;
  difference() {
    translate([axle_x_rear - n20_w / 2 - 2.5, min(y_face, y_in) + (side > 0 ? 2 : 0), tub_z0 + floor_t - eps])
      cube([n20_w + 5, n20_len - 2, cz - n20_h / 2 + 3 - tub_z0 - floor_t]);
    // motor body
    translate([axle_x_rear - n20_w / 2 - clr, min(y_face, y_in) - 1, cz - n20_h / 2 - clr])
      cube([n20_w + 2 * clr, n20_len + 2, n20_h + 2]);
    // zip-tie slots
    for (yy = [y_face - side * 9, y_face - side * 24])
      translate([axle_x_rear - n20_w / 2 - 2.6, yy - 2, tub_z0 + floor_t + 1.2]) cube([n20_w + 5.2, 4, 1.8]);
  }
}

module wall_cutouts() {
  for (side = [-1, 1]) {
    // Motor gearbox window through the side wall (gearbox face sits flush)
    translate([axle_x_rear - n20_w / 2 - clr, side > 0 ? iy - 1 : -tub_w / 2 - 1, axle_z - n20_h / 2 - clr])
      cube([n20_w + 2 * clr, wall + 2, n20_h + 2 * clr]);
    // Idler axle slot (move the axle forward to tension the track)
    hull() for (dx = [-2, 2])
      translate([axle_x_front + dx, side * (tub_w / 2 + 1), axle_z]) rotate([90, 0, 0]) cylinder(d = m3_hole, h = wall + 2, center = true);
  }
  // Rear wall: speaker grille, USB-C charge port, power switch
  for (dy = [-9 : 4.5 : 9], dz = [-9 : 4.5 : 9])
    if (dy * dy + dz * dz < 110)
      translate([x0 - 1, speaker_pos[0] + dy, speaker_pos[1] + dz]) rotate([0, 90, 0]) cylinder(d = 3, h = wall + 2, $fn = 16);
  translate([x0 - 1, usbc_pos[0] - usbc_slot[0] / 2, usbc_pos[1] - usbc_slot[1] / 2]) cube([wall + 2, usbc_slot[0], usbc_slot[1]]);
  translate([x0 - 1, switch_pos[0] - switch_slot[0] / 2, switch_pos[1] - switch_slot[1] / 2]) cube([wall + 2, switch_slot[0], switch_slot[1]]);
  // Cable pass-through in the floor for a charge LED / debugging (optional)
  translate([0, 0, tub_z0 - 1]) cylinder(d = 6, h = floor_t + 2);
}

module speaker_ring() {
  // Ring the 28 mm speaker presses into (glue lightly).
  translate([x0 + wall - eps, speaker_pos[0], speaker_pos[1]]) rotate([0, 90, 0])
    difference() {
      cylinder(d = speaker_d + 3, h = speaker_t);
      translate([0, 0, -1]) cylinder(d = speaker_d, h = speaker_t + 2);
      // keep the bottom open for the tray / wires
      translate([0, -speaker_d, -1]) cube([speaker_d, 2 * speaker_d, speaker_t + 2]);
    }
}

module tray_ledges() {
  for (side = [-1, 1])
    translate([-18, side > 0 ? iy - 2 : -iy, tray_z - 2]) cube([52, 2 + eps, 2]);
  // front ledge
  translate([ix - 2, -iy + 4, tray_z - 2]) cube([2 + eps, 2 * iy - 8, 2]);
}

module corner_bosses() {
  for (sx = [-1, 1], sy = [-1, 1])
    translate([sx * boss_xy[0], sy * boss_xy[1], tub_z0 + floor_t - eps])
      difference() {
        cylinder(d = 7, h = tub_h - floor_t);
        translate([0, 0, 4]) cylinder(d = m3_tap, h = tub_h);
      }
}

module battery_stops() {
  // Low walls keep the LiPo from sliding into the motors.
  translate([axle_x_rear + n20_w / 2 + 2, -battery[1] / 2, tub_z0 + floor_t - eps]) cube([2, battery[1], 5]);
  for (s = [-1, 1])
    translate([0, s * (battery[1] / 2 + 1.5) - 1, tub_z0 + floor_t - eps]) cube([20, 2, 4]);
}

module chassis() {
  difference() {
    union() {
      shell();
      for (side = [-1, 1]) motor_cradle(side);
      speaker_ring();
      tray_ledges();
      corner_bosses();
      battery_stops();
    }
    wall_cutouts();
  }
}

chassis();
