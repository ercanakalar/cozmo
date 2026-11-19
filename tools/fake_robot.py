#!/usr/bin/env python3
"""Pretend to be the robot - test the PC server without any hardware.

* finds the server through its UDP beacon (or use --server IP)
* connects to the control port, prints every command and acknowledges it
* sends telemetry
* optionally streams a 16 kHz mono WAV file as microphone audio (--wav)
* saves everything the server says to the speaker into speaker_out.wav

    python tools/fake_robot.py                          # just log commands
    python tools/fake_robot.py --wav hello_cozmo.wav    # test speech recognition
"""

from __future__ import annotations

import argparse
import asyncio
import json
import signal
import socket
import sys
import time
import wave
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "server"))
from cozmo_server.protocol import MIC_MAGIC, SPEAKER_MAGIC, pack_audio, unpack_audio  # noqa: E402

DURATIONS = {"move": 1.0, "turn": 0.8, "drive": 0.5, "head": 0.3, "lift": 0.4, "anim": 1.5}


def discover(port: int, timeout: float) -> tuple[str, int]:
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(("", port))
    sock.settimeout(timeout)
    data, addr = sock.recvfrom(256)
    text = data.decode(errors="replace").split()
    return addr[0], int(text[1])


class SpeakerSink(asyncio.DatagramProtocol):
    def __init__(self) -> None:
        self.chunks: list[np.ndarray] = []

    def datagram_received(self, data: bytes, addr) -> None:
        packet = unpack_audio(data)
        if packet and packet.magic == SPEAKER_MAGIC:
            self.chunks.append(packet.samples.copy())


async def stream_wav(path: str, server_ip: str, port: int) -> None:
    with wave.open(path, "rb") as wf:
        if wf.getframerate() != 16000 or wf.getnchannels() != 1 or wf.getsampwidth() != 2:
            raise SystemExit("WAV must be 16 kHz, mono, 16-bit")
        pcm = np.frombuffer(wf.readframes(wf.getnframes()), dtype="<i2")
    # 1 s of silence before and after so the server VAD can find the edges.
    pcm = np.concatenate([np.zeros(16000, np.int16), pcm, np.zeros(16000, np.int16)])
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    start = time.monotonic()
    for n, i in enumerate(range(0, len(pcm) - 319, 320)):
        sock.sendto(pack_audio(MIC_MAGIC, n, pcm[i : i + 320]), (server_ip, port))
        await asyncio.sleep(max(0.0, start + (n + 1) * 0.02 - time.monotonic()))
    print(f"[mic] streamed {len(pcm) / 16000:.1f} s of audio")


async def main() -> None:
    p = argparse.ArgumentParser()
    p.add_argument("--server", help="server IP (default: discover via beacon)")
    p.add_argument("--control-port", type=int, default=5008)
    p.add_argument("--audio-port", type=int, default=5005)
    p.add_argument("--speaker-port", type=int, default=5007)
    p.add_argument("--wav", help="16 kHz mono WAV to send as microphone audio")
    p.add_argument("--out", default="speaker_out.wav")
    args = p.parse_args()

    server, port = args.server, args.control_port
    if server is None:
        print("Waiting for server beacon on UDP 5006...")
        server, port = discover(5006, 10.0)
    print(f"Server at {server}:{port}")

    loop = asyncio.get_running_loop()
    main_task = asyncio.current_task()
    for sig in (signal.SIGINT, signal.SIGTERM):
        loop.add_signal_handler(sig, main_task.cancel)
    sink = SpeakerSink()
    await loop.create_datagram_endpoint(lambda: sink, local_addr=("0.0.0.0", args.speaker_port))

    reader, writer = await asyncio.open_connection(server, port)

    def send(msg: dict) -> None:
        writer.write((json.dumps(msg) + "\n").encode())

    send({"type": "hello", "name": "fake-cozmo", "fw": "sim"})

    async def telemetry() -> None:
        while True:
            send({"type": "status", "battery_pct": 87, "battery_v": 3.98, "picked_up": False})
            await asyncio.sleep(0.5)

    async def finish(cmd_id: int, delay: float) -> None:
        await asyncio.sleep(delay)
        send({"type": "done", "id": cmd_id})

    asyncio.create_task(telemetry())
    if args.wav:
        asyncio.create_task(stream_wav(args.wav, server, args.audio_port))

    try:
        while line := await reader.readline():
            msg = json.loads(line)
            if msg.get("cmd") == "ping":
                continue
            print(f"[cmd] {msg}")
            if "id" in msg:
                asyncio.create_task(finish(msg["id"], DURATIONS.get(msg["cmd"], 0.2)))
    except (KeyboardInterrupt, asyncio.CancelledError):
        pass
    finally:
        if sink.chunks:
            pcm = np.concatenate(sink.chunks)
            with wave.open(args.out, "wb") as wf:
                wf.setnchannels(1)
                wf.setsampwidth(2)
                wf.setframerate(16000)
                wf.writeframes(pcm.astype("<i2").tobytes())
            print(f"Saved {len(pcm) / 16000:.1f} s of robot speech to {args.out}")


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        pass
