# Cozmo PC server

The robot is a thin client: it streams its microphone to this program and
waits for commands. This server does the heavy lifting:

```
robot mic ─UDP 5005─▶ VAD ─▶ speech-to-text ─▶ wake word ─▶ intent parser ─┬─▶ commands ─TCP 5008─▶ robot
                                                 (faster-whisper)            └─▶ text-to-speech ─UDP 5007─▶ robot speaker
                                                               (optional Claude chat for anything unknown)
```

## Install (Windows, macOS or Linux, Python 3.11+)

```bash
cd server
python -m venv .venv
```

```bash
source .venv/bin/activate        # Windows: .venv\Scripts\activate
```

```bash
pip install -e ".[whisper,dev]"
```

Text to speech uses `espeak-ng` by default (small and supports Turkish):

* Windows: install from https://github.com/espeak-ng/espeak-ng/releases and make sure `espeak-ng` is on PATH
* Ubuntu/Debian: `sudo apt install espeak-ng`
* macOS: `brew install espeak-ng`

For a much more natural voice, install [Piper](https://github.com/rhasspy/piper),
download a Turkish voice (e.g. `tr_TR-dfki-medium.onnx` + its `.json`) into
`server/models/` and set `[tts] backend = "piper"` in the config.

The first run of faster-whisper downloads the speech model (`small` ≈ 500 MB).
On a laptop CPU, `small` needs ~1–2 s per sentence; use `base` if that is too
slow, or `device = "cuda"` with an NVIDIA GPU.

## Run

```bash
python -m cozmo_server
```

Then power on the robot. It finds the PC automatically through the UDP
broadcast beacon (both must be on the same WiFi network; allow Python through
the firewall on ports 5005–5008).

Say **"Cozmo, ileri git"**, **"Kozmo, sola dön"**, **"Cozmo dans et"**,
**"Cozmo, adın ne?"**… After the robot answers, follow-ups within 12 s do not
need the name. See `cozmo_server/intents.py` for every command (Turkish and
English). Switch language with `[persona] language = "en"`.

### Testing without a robot

```bash
python -m cozmo_server --keyboard
```

```bash
python ../tools/fake_robot.py --server 127.0.0.1
```

Type commands in the first terminal; the second one prints what the robot
would receive and saves the robot's speech to `speaker_out.wav`. Use
`fake_robot.py --wav my_voice.wav` (16 kHz mono) to test speech recognition.

### Free-form conversation with Claude (optional)

```bash
pip install -e ".[llm]"
```

```bash
python -m cozmo_server --llm
```

Requires an Anthropic API key (`ANTHROPIC_API_KEY`). Rule-based commands
still run instantly and offline; only sentences the parser does not
understand go to Claude, which answers in character and can trigger
animations and movements. Set `[llm] model` in the config to change the model.

## Tests

```bash
python -m pytest
```
