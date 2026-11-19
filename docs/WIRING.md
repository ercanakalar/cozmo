# Wiring

All pin numbers live in [`firmware/include/config.h`](../firmware/include/config.h).
If you change a pin there, change this table too.

## Power

```
                   SS12D00
 1S LiPo ──► TP4056 OUT+ ──switch──┬──► 5 V boost (≥2 A) ──┬──► ESP32 5V/VIN
 (B+/B-)    (USB-C charge)         │                       ├──► DRV8833 VM
                                   │                       ├──► servo +  (head, lift)
                                   │                       ├──► MAX98357A VIN
                                   │                       └──► 470 µF cap to GND (×2: at the boost and at the servos)
                                   ├──► WS2812 VDD  (LiPo voltage: 3.3 V data is then valid)
                                   └──► 100k ─┬─ 100k ─► GND
                                              └─► GPIO36 (battery sense, + 100 nF to GND)

 ESP32 3V3 ──► OLED VCC, MPU6050 VCC, INMP441 VDD, encoder VCC, 10k pull-ups
 All GNDs are common.
```

* Turn the robot **off** before plugging the ESP32 into USB to flash it,
  unless your DevKit has a diode between USB 5 V and VIN (many do, not all).
* Charging works with the switch on or off.

## Signal connections

| ESP32 pin | Goes to | Notes |
|-----------|---------|-------|
| GPIO21 | OLED SDA, MPU6050 SDA | I2C, 400 kHz |
| GPIO22 | OLED SCL, MPU6050 SCL | |
| GPIO26 | INMP441 SCK | I2S0 bit clock |
| GPIO25 | INMP441 WS | I2S0 word select |
| GPIO34 | INMP441 SD | input-only pin is fine |
| — | INMP441 L/R → GND | left slot (see `MIC_USE_RIGHT_SLOT`) |
| GPIO27 | MAX98357A BCLK | I2S1 |
| GPIO14 | MAX98357A LRC | |
| GPIO13 | MAX98357A DIN | |
| — | MAX98357A GAIN | leave open (9 dB) |
| GPIO16 | DRV8833 AIN1 | left tread motor |
| GPIO17 | DRV8833 AIN2 | |
| GPIO18 | DRV8833 BIN1 | right tread motor |
| GPIO19 | DRV8833 BIN2 | |
| — | DRV8833 EEP/nSLEEP → 3V3 | many modules already pull it up |
| GPIO35 | Left encoder channel A | + 10 kΩ pull-up to 3V3 (no internal pull-up on 34–39) |
| GPIO5  | Right encoder channel A | + 10 kΩ pull-up to 3V3 |
| GPIO4  | Head servo signal (SG90) | |
| GPIO23 | Lift servo signal (MG90S) | |
| GPIO32 | WS2812 DIN (through 330 Ω) | 4 LEDs chained |
| GPIO33 | Copper tape touch pad | single wire, no resistor |
| GPIO36 | Battery divider mid-point | ADC1, works with WiFi on |

DRV8833 outputs: `AOUT1/AOUT2` → left motor, `BOUT1/BOUT2` → right motor.
If a tread spins the wrong way, swap that motor's two wires or flip
`MOTOR_L_INVERT` / `MOTOR_R_INVERT` in `config.h`.

Encoders: power them from **3V3** so their outputs stay at a safe level for
the ESP32. Only channel A is used.

### Pins to avoid

* GPIO0, 2, 12, 15: boot strapping pins (GPIO12 high at boot breaks flash).
* GPIO6–11: connected to the flash chip.
* GPIO36/39 as interrupt inputs: ESP32 errata 3.11 (false edges while the
  ADC is on). GPIO36 is used here only as an analog input.
* GPIO16/17 on WROVER modules (PSRAM).

## Where each part goes

| Location | Parts |
|----------|-------|
| Head | OLED (face window), head servo (inside), copper tape (inside the roof) |
| Cover, front | INMP441 (glued under the mic hole, port facing up) |
| Cover, rear | WS2812 ×4 under the light windows |
| Lift tower (right) | Lift servo |
| Tray | ESP32, DRV8833, MPU6050 (flat, X axis forward), MAX98357A, boost, TP4056 |
| Tub floor | LiPo (front), N20 motors (rear) |
| Rear wall | Speaker, USB-C port of the TP4056, power switch |

Keep the microphone wires short and away from the motor wires. Twist each
motor's two wires together and solder a 100 nF ceramic capacitor across
each motor's terminals to reduce electrical noise.

## Head cable

The head moves, so leave a ~6 cm service loop for the OLED (4 wires), head
servo (3 wires) and touch pad (1 wire) going through the slot behind the
head into the body.
