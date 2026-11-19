#!/usr/bin/env python3
"""Interactive console to drive the robot by hand (bring-up, calibration).

Acts as a minimal server: broadcasts the discovery beacon, accepts the
robot's TCP connection, prints telemetry/events and sends your commands.
Stop the real server first (they use the same ports).

    python tools/robot_console.py

Commands:
    move <mm> [speed]        turn <deg> [deg/s]       drive <l> <r> <ms>
    head <deg>               lift <mm>                stop
    face <expression>        anim <name>              look <x> <y>
    tone <hz> [end_hz] [ms]  volume <0..1>            leds <hue> [pattern]
    mode <idle|listening|thinking|speaking>          status
    raw {"cmd": ...}         quit
"""

from __future__ import annotations

import asyncio
import json
import shlex
import socket
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "server"))
from cozmo_server.protocol import beacon_message  # noqa: E402

CONTROL_PORT = 5008
DISCOVERY_PORT = 5006


class Console:
    def __init__(self) -> None:
        self.writer: asyncio.StreamWriter | None = None
        self.status: dict = {}

    async def on_robot(self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter) -> None:
        self.writer = writer
        print(f"\n[robot connected from {writer.get_extra_info('peername')}]")
        while line := await reader.readline():
            msg = json.loads(line)
            if msg.get("type") == "status":
                self.status = msg
            else:
                print(f"\n<- {msg}")
        print("\n[robot disconnected]")
        self.writer = None

    def send(self, msg: dict) -> None:
        if self.writer is None:
            print("robot not connected yet")
            return
        self.writer.write((json.dumps(msg) + "\n").encode())
        print(f"-> {msg}")


def parse(line: str) -> dict | None:
    parts = shlex.split(line)
    if not parts:
        return None
    c, a = parts[0], parts[1:]
    f = lambda i, d: float(a[i]) if len(a) > i else d  # noqa: E731
    match c:
        case "move": return {"cmd": "move", "distance_mm": f(0, 100), "speed": f(1, 100)}
        case "turn": return {"cmd": "turn", "angle_deg": f(0, 90), "speed_dps": f(1, 120)}
        case "drive": return {"cmd": "drive", "left": f(0, 100), "right": f(1, 100), "duration_ms": int(f(2, 1000))}
        case "head": return {"cmd": "head", "angle_deg": f(0, 10), "duration_ms": 400}
        case "lift": return {"cmd": "lift", "height_mm": f(0, 0), "duration_ms": 500}
        case "stop": return {"cmd": "stop"}
        case "face": return {"cmd": "face", "expression": a[0] if a else "neutral", "duration_ms": 4000}
        case "anim": return {"cmd": "anim", "name": a[0] if a else "happy"}
        case "look": return {"cmd": "look", "x": f(0, 0), "y": f(1, 0)}
        case "tone": return {"cmd": "tone", "start_hz": f(0, 800), "end_hz": f(1, f(0, 800)), "duration_ms": int(f(2, 200))}
        case "volume": return {"cmd": "volume", "value": f(0, 0.7)}
        case "leds": return {"cmd": "leds", "hue": f(0, 120), "pattern": a[1] if len(a) > 1 else "solid", "duration_ms": 3000}
        case "mode": return {"cmd": "mode", "value": a[0] if a else "idle"}
        case "raw": return json.loads(line[3:])
    raise ValueError(f"unknown command '{c}'")


async def beacon() -> None:
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    while True:
        try:
            sock.sendto(beacon_message(CONTROL_PORT), ("255.255.255.255", DISCOVERY_PORT))
        except OSError:
            pass
        await asyncio.sleep(1)


async def keepalive(console: Console) -> None:
    while True:
        await asyncio.sleep(1)
        if console.writer is not None:
            console.writer.write(b'{"cmd":"ping"}\n')


async def main() -> None:
    console = Console()
    await asyncio.start_server(console.on_robot, "0.0.0.0", CONTROL_PORT)
    asyncio.create_task(beacon())
    asyncio.create_task(keepalive(console))
    print(__doc__)
    print("Waiting for the robot... (type commands any time)")
    while True:
        line = (await asyncio.to_thread(input, "> ")).strip()
        if line in ("quit", "exit"):
            return
        if line == "status":
            print(json.dumps(console.status, indent=2))
            continue
        try:
            msg = parse(line)
        except (ValueError, IndexError, json.JSONDecodeError) as exc:
            print(exc)
            continue
        if msg:
            console.send(msg)


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except (KeyboardInterrupt, EOFError):
        pass
