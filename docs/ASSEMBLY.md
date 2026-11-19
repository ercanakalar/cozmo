# Printing and assembly

## 1. Print the parts

Ready-made STLs are in [`hardware/3d/stl/`](../hardware/3d/stl). They are
generated from the parametric OpenSCAD sources next to them; edit
`params.scad` and run `hardware/3d/export.sh` to regenerate.

General settings: PLA or PETG, 0.2 mm layers, 3 walls, 20 % infill
(100 % for sprockets), 0.4 mm nozzle.

| STL | Qty | Place on the bed | Supports |
|-----|----:|------------------|----------|
| `chassis.stl` | 1 | upright, open side up | no |
| `electronics_tray.stl` | 1 | flat | no |
| `top_cover.stl` | 1 | upright (cheeks pointing up) | no |
| `head_front.stl` | 1 | face down (as exported) | no |
| `head_back.stl` | 1 | rear face down (as exported) | no |
| `lift_tower_left.stl` | 1 | rotate to lie on the large outer plate | no |
| `lift_tower_right.stl` | 1 | rotate to lie on the large outer plate | no |
| `lift_link.stl` | 3 | flat | no |
| `lift_link_servo.stl` | 1 | flat, horn recess up | no |
| `lift_fork.stl` | 1 | back face down (as exported) | no |
| `sprocket_drive.stl` | 2 | flat, 100 % infill | no |
| `sprocket_idler.stl` | 2 | flat | no |
| `track_links_x13.stl` | 4 | flat (as exported) — 52 links | no |
| `cube_body.stl` / `cube_lid.stl` | 1 each | open side up / flat | no |

Print one `track_link.stl` first and test it with two links and a piece of
filament: the hinge must swing freely. If it is tight, increase `pin_hole`
or `gap` in the SCAD files; if your printer over-extrudes, also `clr`.

## 2. Screws

| Where | Screw | Qty |
|-------|-------|----:|
| Cover → tub | M3×8 countersunk (self-tapping into the bosses) | 4 |
| Lift towers → cover | M3×8 from below (self-tapping into the feet) | 4 |
| Lift link pivots on the towers | M3×12 + washer | 3 |
| Lift links → fork carriers | M3×10 + washer | 4 |
| Head pivot (left cheek → head) | M3×10 + washer | 1 |
| Idler axles | M3×25 + nut + 2 washers | 2 |
| Servo tabs (head 1, lift 2) | M2×6 self-tapping | 3 |
| OLED | M2×5 self-tapping | 4 |
| Sprocket set screw (optional) | M2×4 self-tapping | 2 |

## 3. Electronics first (on the bench)

1. Flash the firmware (see the main README) and wire everything on a
   breadboard following [WIRING.md](WIRING.md).
2. Run the PC server or `tools/robot_console.py`, check the face appears,
   `tone 800` beeps, `mode listening` changes the lights, and the server
   log shows audio arriving (`status` → `mic_rms` rises when you talk).
3. Only then solder/crimp the final harness.

## 4. Mechanical assembly

1. **Motors**: push each N20 gearbox through the side-wall window so the
   gearbox face is flush with the outer wall. Strap it to its cradle with a
   zip tie through the two slots. Solder the 100 nF capacitor and wires
   before mounting.
2. **Speaker, switch, charger**: press the speaker into the ring on the rear
   wall (a drop of hot glue), mount the slide switch and the TP4056 so its
   USB-C port lines up with the rear slot (foam tape on the tray).
3. **Battery** goes flat on the floor in front of the motors.
4. **Tray**: stick the modules on the tray (see the layout comment in
   `electronics_tray.scad`), mount the MPU6050 flat with its X axis
   pointing forward, then lay the tray on the ledges.
5. **Sprockets**: press a drive sprocket onto each motor shaft (D-flat
   aligned, hub towards the wall). Mount the idlers on M3×25 screws through
   the front slots with washers on both sides of the wheel and a nut inside.
6. **Tracks**: chain 26 links per side with 1.75 mm filament pins, close the
   loop around both wheels, then slide the idler screw forward until the
   track is snug (not tight) and tighten the nut.
7. **Head**: screw the OLED behind the face window. Trim the lower mounting
   tab of the SG90 flush with its body, put the servo into the rib (spline
   pointing out of the right side), fix it with one M2 screw. Stick a 3×3 cm
   piece of copper tape inside the roof and solder a wire to it. Route the
   wires out of the bottom slot, then press the back cap on.
8. **Cover**: glue the INMP441 under the mic hole (sound port facing up) and
   the 4 WS2812 LEDs under the light windows (a strip of paper or thin white
   plastic makes a nice diffuser). Pass the head and lift wires through the
   slots.
9. **Head on the cheeks**: power the robot, connect `tools/robot_console.py`
   and send `head 10` so the servo sits at 10°. Screw a servo horn onto the right cheek's
   inside (2 small screws through the 4-hole pattern), slide the head
   between the cheeks so the spline enters the horn, then put the M3 pivot
   screw through the left cheek into the head.
10. **Lift**: mount the towers on the cover wings (MG90S in the right
    tower, spline pointing outwards). With the robot powered, send `lift 0`,
    then attach the single-arm horn to `lift_link_servo` and push it onto the
    spline with the link pointing forward and slightly down. Add the other
    three links and the fork assembly; the forks should just touch the floor.
11. **Close up**: screw the cover onto the tub.

## 5. Calibration

Use `python tools/robot_console.py` (stop the server first).

* **Servos**: `head -25` should look fully down, `head 44` fully up.
  Adjust `HEAD_SERVO_US_AT_MIN/MAX` in `config.h` until they do. Same for
  the lift: `lift 0` (forks on the floor) and `lift 60` with
  `LIFT_SERVO_US_AT_MIN/MAX`. If a servo moves the wrong way, swap the two
  values. Never let a servo buzz against an end stop.
* **Treads**: `move 200` must drive 20 cm forward. If it goes backwards,
  flip `MOTOR_L_INVERT`/`MOTOR_R_INVERT`. If the distance is off, measure
  and adjust `ENC_TICKS_PER_WHEEL_REV` (your gearbox ratio × 7).
* **Turns**: `turn 90` must turn left by 90°. If it spins forever or turns
  right, flip `IMU_YAW_SIGN`. Without an IMU, tune `TURN_SLIP_FACTOR`.
* **Speed**: `drive 220 220 1000` should travel ~22 cm. Set
  `MAX_SPEED_MMPS` to what your motors really reach at full power.
* **Microphone**: `status` shows `mic_rms`. Speaking from 50 cm should give
  a few hundred to a few thousand. Change `MIC_GAIN_SHIFT` (smaller = louder).

## 6. Troubleshooting

| Symptom | Fix |
|---------|-----|
| Robot resets when the lift or treads move | Boost converter too weak or missing bulk capacitors |
| Face shows garbage / shifted by 2 px | Wrong controller: toggle `OLED_IS_SH1106` |
| `mic_rms` always 0 | Check wiring, then flip `MIC_USE_RIGHT_SLOT` |
| Server never sees the robot | Same WiFi? Firewall allows Python on UDP 5005/5006 and TCP 5008? Set `COZMO_SERVER_IP` in `secrets.h` |
| Speech is choppy | Weak WiFi (`rssi` below -75): move the router/robot closer |
| Robot reacts to the TV | Keep `require_wake_word = true`, raise `[vad] start_margin_db` |
| Tracks jump teeth | Tension the idler, or print the links with less elephant's foot |
