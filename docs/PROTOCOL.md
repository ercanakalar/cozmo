# Robot ↔ PC protocol

| Port | Transport | Direction | Content |
|------|-----------|-----------|---------|
| 5006 | UDP broadcast | PC → all | Discovery beacon `COZMO_SERVER <tcp_port>\n`, once per second |
| 5008 | TCP | both | Control channel, one JSON object per line |
| 5005 | UDP | robot → PC | Microphone audio |
| 5007 | UDP | PC → robot | Speaker audio |

The robot learns the server IP from the beacon's sender address (or from
`COZMO_SERVER_IP` in `secrets.h`) and connects to the control port. The PC
learns the robot IP from that TCP connection.

## Audio packets (UDP)

Little-endian, 12 byte header followed by PCM samples:

| Offset | Type | Field |
|-------:|------|-------|
| 0 | char[4] | magic: `CZMU` (mic) or `CZSP` (speaker) |
| 4 | uint32 | sequence number |
| 8 | uint16 | number of samples |
| 10 | uint16 | flags |
| 12 | int16[n] | 16 kHz mono PCM |

Mic packets carry 320 samples (20 ms). Flags: bit 0 = robot speaker was
playing (echo likely), bit 1 = motors/servos moving.

Speaker packets carry up to 700 samples; the PC sends 320-sample packets
paced at real time with 200 ms sent up front. Flag bit 0 marks the last
packet of a clip. The robot queues up to ~1 s of audio.

Shared code: `firmware/src/net/Protocol.h` and `server/cozmo_server/protocol.py`.

## Control channel (TCP, JSON lines)

### PC → robot commands

Add `"id": <int>` to `move`, `turn`, `drive`, `head`, `lift` or `anim` to
get a `done` message back when the action finishes.

| `cmd` | Fields | Effect |
|-------|--------|--------|
| `ping` | | Keep-alive (sent every second). Treads stop if nothing arrives for 3 s while driving. |
| `move` | `distance_mm`, `speed` (mm/s) | Drive straight, negative = backwards |
| `turn` | `angle_deg` (+ = left), `speed_dps` | Turn in place (gyro-assisted) |
| `drive` | `left`, `right` (mm/s), `duration_ms` (0 = until stop) | Raw tread speeds |
| `stop` | | Stop treads and the current animation |
| `head` | `angle_deg` (-25…44.5), `duration_ms` | Tilt the head |
| `lift` | `height_mm` (0…60), `duration_ms` | Move the forks |
| `face` | `expression`, `duration_ms` (0 = keep) | `neutral happy sad angry surprised sleepy suspicious scared focused bored excited squint` |
| `look` | `x`, `y` (-1…1) | Move the eyes |
| `anim` | `name` | `happy sad angry surprised celebrate curious sleep wake nod_yes shake_no dance greet scared bored thinking purr lift_up lift_down` |
| `mode` | `value`: `idle` `listening` `thinking` `speaking` | Conversation state (face, lights, head pose) |
| `leds` | `hue` (0…360), `brightness` (0…1), `pattern` (`solid breathe chase blink off`), `duration_ms` | Backpack lights override |
| `tone` | `start_hz`, `end_hz`, `duration_ms`, `volume` | Robot chirp generated on the ESP32 |
| `volume` | `value` (0…1) | Speaker volume |
| `text` | `line1`, `line2`, `duration_ms` | Show text on the face |
| `audio_flush` | | Drop queued speaker audio |
| `sleep` | | Go to sleep (wakes on touch, sound or any command) |

### Robot → PC messages

| `type` | Fields |
|--------|--------|
| `hello` | `name`, `fw`, `mac` — sent once after connecting |
| `status` | every 200 ms: `battery_v`, `battery_pct`, `picked_up`, `touched`, `sleeping`, `head_deg`, `lift_mm`, `yaw_deg`, `speed_l`, `speed_r`, `encoders_ok`, `imu_ok`, `busy`, `anim`, `expression`, `mic_rms`, `rssi`, `audio_tx`, `audio_rx` |
| `done` | `what`, `id` |
| `event` | `name`: `picked_up put_down touched bumped loud_sound battery_low sleeping woke_up` |
| `error` | `message`, `cmd` |
