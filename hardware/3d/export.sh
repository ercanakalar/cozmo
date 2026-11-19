#!/usr/bin/env bash
# Re-export every printable STL into hardware/3d/stl/.
# Usage: ./export.sh            (needs OpenSCAD on PATH)
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p stl

render() {  # render <output.stl> <file.scad> [-D var=value ...]
  local out="$1" src="$2"
  shift 2
  echo "-> stl/$out"
  openscad -q -o "stl/$out" "$@" "$src"
}

render chassis.stl            chassis.scad
render electronics_tray.stl   electronics_tray.scad
render top_cover.stl          top_cover.scad
render head_front.stl         head.scad        -D 'part="front"'
render head_back.stl          head.scad        -D 'part="back"'
render lift_tower_left.stl    lift_tower.scad  -D 'side="left"'
render lift_tower_right.stl   lift_tower.scad  -D 'side="right"'
render lift_link.stl          lift.scad        -D 'part="link"'
render lift_link_servo.stl    lift.scad        -D 'part="link_servo"'
render lift_fork.stl          lift.scad        -D 'part="fork"'
render sprocket_drive.stl     sprocket.scad    -D 'drive=true'
render sprocket_idler.stl     sprocket.scad    -D 'drive=false'
render track_link.stl         track_link.scad  -D 'part="single"'
render track_links_x13.stl    track_link.scad  -D 'part="plate"'
render cube_body.stl          cube.scad        -D 'part="body"'
render cube_lid.stl           cube.scad        -D 'part="lid"'
echo "Done."
