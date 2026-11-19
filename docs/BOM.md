# Bill of materials (budget: 4000 TL)

Prices are **rough estimates for Turkey in late 2026**. Electronics prices
move fast, so check them before you buy at the usual Turkish maker shops
(Robotistan, Direnc.net, Robolink Market, Motorobit…) or AliExpress (cheaper,
slower). Parts you already own are marked **owned**.

## Electronics

| # | Part | Qty | Notes | Est. TL |
|---|------|-----|-------|--------:|
| 1 | ESP32 DevKit (ESP32-WROOM-32, 38 pin) | 1 | **owned**. Must be WROOM, not WROVER (WROVER uses GPIO16/17 for PSRAM). | 0 |
| 2 | INMP441 I2S MEMS microphone | 1 | **owned**. If yours is analog (MAX9814/KY-037) see the note below. | 0 |
| 3 | 1.3" I2C OLED 128x64 (SH1106), white or blue | 1 | The robot's face. A 0.96" SSD1306 also works (see `oled_096` in `params.scad`). | 250 |
| 4 | N20 gear motor **with encoder**, 6 V, 200 RPM (150:1), 3 mm D shaft | 2 | Drives the treads. Encoders are optional, the firmware falls back to open loop. | 800 |
| 5 | DRV8833 dual motor driver module | 1 | | 90 |
| 6 | MG90S metal-gear micro servo | 1 | Lift (carries load). | 200 |
| 7 | SG90 micro servo | 1 | Head tilt. | 90 |
| 8 | MAX98357A I2S class-D amplifier | 1 | | 180 |
| 9 | Speaker 28 mm, 8 Ω, 1–2 W | 1 | | 60 |
| 10 | MPU6050 (GY-521) IMU | 1 | Pick-up detection and accurate turns. | 120 |
| 11 | WS2812B LEDs (4 pcs, cut from a strip or single PCBs) | 4 | Backpack lights. | 60 |
| 12 | 1S LiPo 3.7 V ~2000 mAh, 103450 size (≤ 52×36×11 mm) | 1 | With protection board and JST lead. | 350 |
| 13 | TP4056 USB-C charger module **with protection** (DW01) | 1 | Charge through the rear wall. | 40 |
| 14 | 5 V boost converter, ≥ 2 A continuous (e.g. FP6277 / HW-668 type) | 1 | **Not** an MT3608, it browns out when servos move. | 100 |
| 15 | SS12D00 slide switch | 1 | | 10 |
| 16 | Copper tape (5–10 mm) | 1 roll | "Pet me" touch pad inside the head. | 30 |
| 17 | Passives: 10 kΩ ×2, 100 kΩ ×2, 330 Ω ×1, 470 µF 10 V ×2, 100 nF ×3 | | Encoder pull-ups, battery divider, LED data resistor, bulk caps. | 40 |
| 18 | Wire: silicone 26/28 AWG + female/male jumpers + JST-PH 2.0 connectors | | | 100 |
| | **Electronics subtotal** | | | **2520** |

## Mechanics

| # | Part | Qty | Notes | Est. TL |
|---|------|-----|-------|--------:|
| 19 | PLA or PETG filament 1 kg | 1 | The robot uses ~250 g; the rest is for re-prints and cubes. | 700 |
| 20 | M3 screw assortment (6–25 mm) + nuts + washers | 1 set | See the screw list in [ASSEMBLY.md](ASSEMBLY.md). | 120 |
| 21 | M2 self-tapping screws (6–8 mm) | ~15 | OLED, servo tabs, horns. | 30 |
| 22 | Double-sided foam tape, zip ties (2.5 mm), heat-shrink | | | 60 |
| | **Mechanics subtotal** | | | **910** |

## Total

| | TL |
|---|---:|
| Electronics | 2520 |
| Mechanics | 910 |
| **Total** | **≈ 3430** |
| Margin left from 4000 TL | ≈ 570 |

The margin covers shipping, price differences, or a spare servo.

### Ways to save money

* Use an SG90 for the lift as well (saves ~110 TL; cubes are light).
* Buy N20 motors **without** encoders (saves ~250–300 TL). The robot still
  drives, distances and turns are just less accurate (the IMU keeps turns OK).
* Use a 0.96" OLED (saves ~80–100 TL; the face is smaller).
* Buy from AliExpress (often 30–50 % cheaper, 2–4 weeks shipping).

### No 3D printer?

A printing service will cost roughly 800–1500 TL for all parts, which goes
over the budget. A friend's printer, a school/university maker lab or a
local maker space is the cheapest option. Total print time is about
18–22 hours (tracks are ~6 h of that).

### You also need (not in the budget)

* A PC/laptop on the same WiFi network (runs the speech brain; Windows,
  macOS or Linux). Any machine from the last ~8 years works; a GPU is not
  required.
* Soldering iron, wire stripper, small screwdrivers, a multimeter (helpful).

### Analog microphone instead of INMP441?

The firmware expects an I2S microphone. If you own an analog module
(MAX9814, KY-037/038), buy an INMP441 (~100 TL) — analog mics pick up a lot
of motor and WiFi noise on the ESP32 ADC and speech recognition suffers.
