// ===========================================================================
//  Cozmo replica - shared dimensions for every printed part
//  Units: mm.  Robot frame: x = forward, y = left, z = up, origin = ground
//  centre.  Change values here, then re-export with hardware/3d/export.sh.
// ===========================================================================

$fn = 48;
eps = 0.01;
clr = 0.25;            // general printing clearance

// ---------------------------------------------------------------- fasteners
m3_hole      = 3.3;    // clearance hole
m3_tap       = 2.6;    // self-tapping into plastic
m3_nut_af    = 5.7;    // nut across flats (+clearance)
m3_nut_h     = 2.6;
m2_tap       = 1.8;
pin_hole     = 1.95;   // 1.75 mm filament used as track pins

// ---------------------------------------------------------------- tracks
pitch        = 8;      // track link pitch
teeth        = 12;     // sprocket teeth = links wrapped per revolution
link_t       = 3.6;    // link thickness (= hinge barrel diameter)
link_w       = 16;     // link width
grouser_h    = 1.2;    // tread ridge height
r_pin        = (pitch / 2) / sin(180 / teeth);    // pin circle radius (15.45)
axle_z       = r_pin + link_t / 2 + grouser_h;    // axle height (18.45)
wheelbase    = 56;     // with 26 links per side: 2*56 + 12*8 = 208 = 26*8
links_per_side = 26;
axle_x_rear  = -wheelbase / 2;   // drive sprocket (motor)
axle_x_front =  wheelbase / 2;   // idler

// ---------------------------------------------------------------- chassis tub
wall         = 2;
tub_len      = 86;
tub_w        = 70;
tub_z0       = 8;      // ground clearance
tub_h        = 42;     // tub top at z = 50
tub_r        = 6;      // vertical corner radius
floor_t      = 2;
tub_top      = tub_z0 + tub_h;
track_y      = tub_w / 2 + 1 + link_w / 2;        // 44: track centre line

// ---------------------------------------------------------------- N20 motor
n20_w        = 12.2;   // gearbox width (z when mounted)
n20_h        = 10.2;   // gearbox height
n20_len      = 33;     // gearbox + motor + encoder
n20_shaft_d  = 3;
n20_shaft_flat = 2.5;  // D-shaft flat
n20_shaft_len = 10;

// ---------------------------------------------------------------- servos (SG90 / MG90S)
servo_l      = 23.0;   // body length
servo_w      = 12.6;   // body width
servo_h      = 22.8;   // body height (to tab underside ~15.9)
servo_tab_l  = 32.6;   // length over mounting tabs
servo_tab_t  = 2.6;
servo_tab_z  = 15.9;   // tab underside from body bottom
servo_shaft_off = 5.9; // output shaft centre from body end
servo_shaft_h = 28.5;  // body bottom to top of spline

// ---------------------------------------------------------------- OLED
// Default: 1.3" I2C module (SH1106). For a 0.96" module set oled_096 = true
// (and OLED_IS_SH1106 = false in firmware/include/config.h).
oled_096     = false;
oled_pcb     = oled_096 ? [27.4, 27.8] : [35.6, 33.8];  // w x h
oled_view    = oled_096 ? [23.0, 12.0] : [30.0, 15.4];  // visible window
oled_view_dz = oled_096 ? 3.6 : 2.6;   // window centre above PCB centre
oled_pcb_t   = 1.7;
oled_glass_t = 1.6;
oled_hole_d  = 2.2;
oled_hole_in = oled_096 ? 2.0 : 2.3;   // hole centre from PCB edges

// ---------------------------------------------------------------- head
head_w       = 56;     // y
head_d       = 36;     // x
head_h       = 44;     // z
head_r       = 7;
head_wall    = 1.8;
head_pivot   = [6, 0, 72];     // pivot axis position in robot frame
head_below   = 12;     // head bottom is this far below the pivot
cheek_t      = 3;

// ---------------------------------------------------------------- lift (parallel 4-bar)
lift_pivot_x = -25;
lift_pivot_lo_z = 59;
lift_pivot_hi_z = 71;
lift_link_len = 80;
lift_link_y  = 57.5;   // link centre plane (outside the tracks + servo horn)
lift_link_t  = 4;
lift_link_h  = 7;
carrier_drop = 28;     // carrier extends this far below the lower pin
fork_y       = 15;     // forks at +-fork_y
fork_len     = 16;

// ---------------------------------------------------------------- misc parts
battery      = [52, 36, 11];   // 1S LiPo 103450 bay (l, w, h)
speaker_d    = 28.5;           // 28 mm 8 ohm speaker
speaker_t    = 5;
switch_slot  = [8.8, 4.2];     // SS12D00 slide switch
usbc_slot    = [9.6, 3.8];     // TP4056 USB-C charging port
led_count    = 4;
led_pitch    = 7;              // WS2812 spacing on the backpack bar

// ---------------------------------------------------------------- helpers
module rounded_box(size, r, center = false) {
  // Box with rounded vertical edges.
  translate(center ? [-size[0] / 2, -size[1] / 2, -size[2] / 2] : [0, 0, 0])
    hull() for (x = [r, size[0] - r], y = [r, size[1] - r])
      translate([x, y, 0]) cylinder(r = r, h = size[2]);
}

module rounded_cube(size, r) {
  // Box with every edge rounded (centred).
  hull() for (x = [-1, 1], y = [-1, 1], z = [-1, 1])
    translate([x * (size[0] / 2 - r), y * (size[1] / 2 - r), z * (size[2] / 2 - r)])
      sphere(r = r, $fn = 24);
}

module hex_nut_pocket(h = m3_nut_h) {
  cylinder(d = m3_nut_af / cos(30), h = h, $fn = 6);
}
