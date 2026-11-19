// ===========================================================================
//  Play cube the robot can pick up with its forks. Hollow, with a lid, so
//  you can add weight (coins) or a battery + LED inside.
//    part = "body" / "lid"
// ===========================================================================
include <params.scad>

part = "body";
size = 40;
cw = 1.6;
slot = [7.5, 9.5];            // fork slot (width, height)
slot_z = 2.5;                 // slot bottom above the floor
slot_depth = 20;

module body() {
  difference() {
    rounded_box([size, size, size], 4);
    translate([cw, cw, cw]) rounded_box([size - 2 * cw, size - 2 * cw, size], 3);
    // Fork slots on the front face (x = 0), matching fork spacing
    for (s = [-1, 1])
      translate([-1, size / 2 + s * fork_y - slot[0] / 2, slot_z]) cube([slot_depth + 1, slot[0], slot[1]]);
    // Face marker so the robot (and you) know which side has the slots
    translate([size / 2, size / 2, size - 0.6]) cylinder(d = 10, h = 1);
  }
  // Slot liners (closed tubes inside the hollow cube)
  for (s = [-1, 1])
    translate([0, size / 2 + s * fork_y - slot[0] / 2 - cw, slot_z - cw])
      difference() {
        cube([slot_depth + cw, slot[0] + 2 * cw, slot[1] + 2 * cw]);
        translate([-1, cw, cw]) cube([slot_depth + 1, slot[0], slot[1]]);
      }
}

module lid() {
  difference() {
    union() {
      rounded_box([size, size, cw], 4);
      translate([cw + 0.3, cw + 0.3, cw - eps]) rounded_box([size - 2 * cw - 0.6, size - 2 * cw - 0.6, 3], 3);
    }
    translate([cw + 1.8, cw + 1.8, cw]) rounded_box([size - 2 * cw - 3.6, size - 2 * cw - 3.6, 4], 2);
  }
}

if (part == "lid") lid(); else body();
