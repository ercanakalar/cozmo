// ===========================================================================
//  Electronics tray: sits on the chassis ledges above the battery and the
//  motors. Modules are fixed with double-sided foam tape / zip ties.
//
//  Suggested layout (top view, front = +x):
//    ESP32 DevKit .......... centre-left, USB port towards the rear
//    DRV8833 ............... rear-right, next to the motors
//    MPU6050 ............... centre of the robot (flat, X axis forward)
//    TP4056 (USB-C) ........ rear-right corner, port through the rear wall
//    5 V boost converter ... front-right
//    MAX98357A ............. rear-left, next to the speaker
// ===========================================================================
include <params.scad>

tray_len = 74;
tray_w = tub_w - 2 * wall - 0.8;
tray_t = 2;
tray_x0 = -35;  // stays clear of the speaker on the rear wall

module tray() {
  difference() {
    translate([tray_x0, -tray_w / 2, 0]) rounded_box([tray_len, tray_w, tray_t], 3);
    // wiring holes down to battery / motors
    for (p = [[-28, 0], [-28, 22], [-28, -22], [30, 0]])
      translate([p[0], p[1], -1]) rounded_box([8, 12, tray_t + 2], 2, center = false);
    // zip-tie slot pairs
    for (x = [-20 : 14 : 30], y = [-27, 27])
      for (dx = [-3, 3]) translate([x + dx - 0.9, y - 2, -1]) cube([1.8, 4, tray_t + 2]);
    // ventilation / weight saving
    for (x = [-14 : 9 : 26], y = [-12 : 8 : 12])
      translate([x, y, -1]) cylinder(d = 4, h = tray_t + 2, $fn = 20);
    // corner notches for the cover screw bosses
    for (sx = [-1, 1], sy = [-1, 1])
      translate([sx > 0 ? tub_len / 2 - 4.5 : -tub_len / 2 + 4.5, sy * (tub_w / 2 - 4.5), -1])
        cylinder(d = 8, h = tray_t + 2);
  }
}

tray();
