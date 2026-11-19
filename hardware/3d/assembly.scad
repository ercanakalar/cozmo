// ===========================================================================
//  Full robot assembly preview (not for printing). Open in OpenSCAD and
//  change `head_tilt` / `lift_angle` to check clearances.
// ===========================================================================
include <params.scad>
use <chassis.scad>
use <electronics_tray.scad>
use <top_cover.scad>
use <sprocket.scad>
use <lift_tower.scad>
use <lift.scad>
use <head.scad>
use <cube.scad>

head_tilt = 10;       // -25 (down) .. 44 (up)
lift_angle = -19.7;   // -19.7 = forks on the floor, ~24 = raised 60 mm
show_cube = true;

module track_band(s) {
  ro = r_pin + link_t / 2 + grouser_h;
  ri = r_pin - link_t / 2;
  color("dimgray") translate([0, s * track_y + link_w / 2, 0]) rotate([90, 0, 0])
    linear_extrude(link_w) difference() {
      hull() for (x = [axle_x_rear, axle_x_front]) translate([x, axle_z]) circle(r = ro);
      hull() for (x = [axle_x_rear, axle_x_front]) translate([x, axle_z]) circle(r = ri);
    }
}

color("gold") chassis();
color("green") translate([0, 0, 24.5]) tray();
color("gold") cover();

for (s = [-1, 1]) {
  track_band(s);
  for (x = [axle_x_rear, axle_x_front])
    color("white") translate([x, s * (track_y - link_w / 2), axle_z])
      rotate([s > 0 ? -90 : 90, 0, 0]) sprocket();
}

color("gold") tower_left_frame();
color("gold") mirror([0, 1, 0]) tower_right_frame();

// Lift 4-bar
for (s = [-1, 1], z = [lift_pivot_lo_z, lift_pivot_hi_z])
  color("orange") translate([lift_pivot_x, s * lift_link_y + lift_link_t / 2, z])
    rotate([90, 0, 0]) rotate([0, 0, lift_angle]) link();
lower_end = [lift_pivot_x + lift_link_len * cos(lift_angle), 0, lift_pivot_lo_z + lift_link_len * sin(lift_angle)];
color("orange") translate(lower_end) fork();

// Head
translate(head_pivot) rotate([0, -head_tilt, 0]) {
  color("gold") front();
  color("goldenrod") back();
}

if (show_cube) color("lightblue") translate([lower_end[0] + 4, -20, 0]) body();
