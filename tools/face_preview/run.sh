#!/usr/bin/env bash
# Preview every eye expression without hardware. Needs g++ and Pillow.
set -euo pipefail
cd "$(dirname "$0")"
SRC=../../firmware/src
g++ -std=gnu++17 -O2 -I stub -I "$SRC" face_preview.cpp "$SRC/face/Face.cpp" "$SRC/drivers/Ssd1306.cpp" -o face_preview
./face_preview
python3 render.py
