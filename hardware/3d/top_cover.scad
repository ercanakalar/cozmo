// ===========================================================================
//  Top cover: closes the tub, carries the head cheeks (pivot + servo horn),
//  the shoulder wings for the lift towers, backpack light windows and the
//  microphone port. Print upright (plate on the bed), no supports.
// ===========================================================================
include <params.scad>

cover_t = 2;
z0 = tub_top;                         // 50
boss_xy = [tub_len / 2 - 4.5, tub_w / 2 - 4.5];
wing_x = [lift_pivot_x - 12, lift_pivot_x + 23];   // [-37, -2]
wing_y = [tub_w / 2 - 4, 53];                    // reaches over the track
led_x0 = -36;
mic_pos = [34, 0];
hp = head_pivot;
cheek_left_y = [head_w / 2 + 0.5, head_w / 2 + 0.5 + cheek_t];           // pivot screw side
cheek_right_y = [-(head_w / 2 + 3 + cheek_t), -(head_w / 2 + 3)];        // servo horn side (3 mm horn gap)

module plate() {
  translate([-tub_len / 2, -tub_w / 2, z0]) rounded_box([tub_len, tub_w, cover_t], tub_r);
  // Shoulder wings
  for (s = [-1, 1])
    translate([wing_x[0], s > 0 ? wing_y[0] : -wing_y[1], z0])
      rounded_box([wing_x[1] - wing_x[0], wing_y[1] - wing_y[0], cover_t + 0.5], 3);
  // Locating ribs that drop just inside the tub walls
  for (s = [-1, 1])
    translate([-20, s > 0 ? tub_w / 2 - wall - 1.6 : -tub_w / 2 + wall + 0.4, z0 - 2]) cube([40, 1.2, 2 + eps]);
}

module cheek(y_range, servo_side) {
  difference() {
    hull() {
      translate([hp[0] - 10, y_range[0], z0 + cover_t - eps]) cube([20, y_range[1] - y_range[0], 1]);
      translate([hp[0], y_range[0], hp[2]]) rotate([-90, 0, 0]) cylinder(r = 7.5, h = y_range[1] - y_range[0]);
    }
    if (servo_side) {
      // Screwdriver access to the horn screw + 4 holes to screw/tie the horn on
      translate([hp[0], y_range[0] - 1, hp[2]]) rotate([-90, 0, 0]) cylinder(d = 5.5, h = cheek_t + 2);
      for (a = [0 : 90 : 270])
        translate([hp[0] + 7 * cos(a), y_range[0] - 1, hp[2] + 7 * sin(a)]) rotate([-90, 0, 0]) cylinder(d = 1.6, h = cheek_t + 2, $fn = 12);
    } else {
      translate([hp[0], y_range[0] - 1, hp[2]]) rotate([-90, 0, 0]) cylinder(d = m3_hole, h = cheek_t + 2);
    }
  }
  // Gussets
  for (dx = [-10, 7])
    translate([hp[0] + dx, y_range[0] + (servo_side ? 0 : 0), z0 + cover_t - eps])
      cube([3, y_range[1] - y_range[0], 8]);
}

module cover() {
  difference() {
    union() {
      plate();
      cheek(cheek_left_y, false);
      cheek(cheek_right_y, true);
    }
    // Screw holes (countersunk) into the tub bosses
    for (sx = [-1, 1], sy = [-1, 1])
      translate([sx * boss_xy[0], sy * boss_xy[1], z0 - 1]) {
        cylinder(d = m3_hole, h = cover_t + 2);
        translate([0, 0, cover_t + 1 - 1.6]) cylinder(d1 = m3_hole, d2 = 6.2, h = 1.6 + eps);
      }
    // Backpack light windows (glue a strip of white paper / diffuser below)
    for (i = [0 : led_count - 1])
      translate([led_x0 + i * led_pitch - 2, -3, z0 - 1]) cube([4, 6, cover_t + 2]);
    // Head cable slot (OLED, head servo, touch pad)
    translate([hp[0] - 12, 2, z0 - 1]) rounded_box([10, 12, cover_t + 2], 2);
    // Lift servo + left tower cable slot
    for (s = [-1, 1]) translate([lift_pivot_x - 4, s * 30 - 4, z0 - 1]) rounded_box([12, 8, cover_t + 2], 2);
    // Lift tower mounting holes (M3 into the tower feet)
    for (s = [-1, 1], x = [lift_pivot_x - 8.5, lift_pivot_x + 19.5])
      translate([x, s * 51, z0 - 1]) cylinder(d = m3_hole, h = cover_t + 3);
    // Microphone port (INMP441 glued underneath, sound hole centred)
    translate([mic_pos[0], mic_pos[1], z0 - 1]) cylinder(d = 2.5, h = cover_t + 2);
    for (a = [0 : 60 : 300])
      translate([mic_pos[0] + 3.2 * cos(a), mic_pos[1] + 3.2 * sin(a), z0 + cover_t - 0.6]) cylinder(d = 1.2, h = 1, $fn = 12);
  }
  // Mic locating ring underneath
  translate([mic_pos[0], mic_pos[1], z0 - 1.5]) difference() {
    cylinder(d = 17, h = 1.5 + eps);
    translate([0, 0, -1]) cube([14.4, 14.4, 4], center = true);
  }
}

cover();
