// ===========================================================================
//  Head: shell with the OLED face window, head-tilt servo inside.
//    part = "front": face + sides, holds OLED and servo (print face down)
//    part = "back":  rear cap, slides into the front part (print flat side down)
//
//  Local frame: origin on the tilt axis (y axis). The head servo's spline
//  points to -y and its horn is screwed to the right cheek of the cover.
//  The +y side turns on an M3 screw through the left cheek.
//  NOTE: trim the lower mounting tab of the head servo flush with its body,
//  it does not fit inside the head. One screw + a drop of glue holds it.
// ===========================================================================
include <params.scad>

part = "front";

hb = head_below;
hc = [0, 0, head_h / 2 - hb];      // head box centre
split_x = -9;                       // front / back split plane
w = head_wall;
face_x = head_d / 2;               // 18
win_z = hc[2] + 1.5;               // face window centre height
pcb_z = win_z - oled_view_dz;
servo_y_bottom = -(head_w / 2 + 3) + 0.2 + servo_shaft_h;   // servo body bottom (-2.5)
servo_tab_y = servo_y_bottom - servo_tab_z;                  // tab underside
rib_y = [servo_tab_y - servo_tab_t - 2.6, servo_tab_y - servo_tab_t];

module outer() { translate(hc) rounded_cube([head_d, head_w, head_h], head_r); }
module inner() { translate(hc) rounded_cube([head_d - 2 * w, head_w - 2 * w, head_h - 2 * w], head_r - w); }

module shell() { difference() { outer(); inner(); } }

module oled_mount() {
  // Four standoffs behind the face plate; the glass sits between them.
  for (sy = [-1, 1], sz = [-1, 1]) {
    py = sy * (oled_pcb[0] / 2 - oled_hole_in);
    pz = pcb_z + sz * (oled_pcb[1] / 2 - oled_hole_in);
    translate([face_x - w - oled_glass_t, py, pz]) rotate([0, 90, 0])
      difference() {
        cylinder(d = 4.6, h = oled_glass_t + eps);
        translate([0, 0, -2]) cylinder(d = m2_tap, h = 8, $fn = 16);
      }
  }
}

module servo_rib() {
  difference() {
    intersection() {
      translate([-servo_w / 2 - 2.5, rib_y[0], -hb]) cube([servo_w + 5, rib_y[1] - rib_y[0], head_h]);
      inner();
    }
    // Body cut-out: body runs from 5.9 mm below the axis upwards
    translate([-servo_w / 2 - clr, rib_y[0] - 1, -servo_shaft_off - clr]) cube([servo_w + 2 * clr, 10, servo_l + 2 * clr]);
    // Upper tab screw
    translate([0, rib_y[0] - 1, -servo_shaft_off + servo_l / 2 + 13.9]) rotate([-90, 0, 0]) cylinder(d = m2_tap, h = 10, $fn = 16);
  }
}

module pivot_boss() {
  translate([0, head_w / 2 - w - 6 + eps, 0]) rotate([-90, 0, 0])
    difference() {
      cylinder(d = 9, h = 6);
      translate([0, 0, -1]) cylinder(d = m3_tap, h = 8);
    }
}

module front() {
  difference() {
    union() {
      intersection() {
        shell();
        translate([split_x, -50, -50]) cube([100, 100, 100]);
      }
      oled_mount();
      servo_rib();
      pivot_boss();
    }
    // Face window
    translate([face_x - w - 1, -oled_view[0] / 2, win_z - oled_view[1] / 2]) cube([w + 2, oled_view[0], oled_view[1]]);
    // Pivot screw through the left wall
    translate([0, head_w / 2 - w - 7, 0]) rotate([-90, 0, 0]) cylinder(d = m3_tap, h = 20);
    // Servo spline + horn hub through the right wall
    translate([0, -head_w / 2 - 1, 0]) rotate([-90, 0, 0]) cylinder(d = 8.5, h = w + 2);
    // Cable exit in the floor, just behind the axis
    translate([split_x + 1, 3, -hb - 1]) cube([7, 12, w + 2]);
  }
}

module back() {
  difference() {
    union() {
      intersection() {
        shell();
        translate([-100 + split_x, -50, -50]) cube([100, 100, 100]);
      }
      // Insertion lip
      intersection() {
        translate(hc) rounded_cube([head_d - 2 * w - 0.4, head_w - 2 * w - 0.4, head_h - 2 * w - 0.4], head_r - w);
        translate([split_x - eps, -50, -50]) cube([3, 100, 100]);
      }
    }
    intersection() {
      translate(hc) rounded_cube([head_d - 4 * w, head_w - 4 * w, head_h - 4 * w], head_r - 2 * w);
      translate([split_x - 20, -50, -50]) cube([24, 100, 100]);
    }
    // Cable notch matching the front part
    translate([split_x - 2, 3, -hb - 1]) cube([6, 12, 2 * w + 2]);
  }
}

if (part == "front") rotate([0, 90, 0]) translate([-face_x, 0, 0]) front();   // face on the bed
else rotate([0, -90, 0]) translate([head_d / 2, 0, 0]) back();                 // rear face on the bed
