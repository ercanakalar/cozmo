"""Network endpoints that talk to the robot.

* RobotLink:     TCP server for the JSON-lines control channel.
* Discovery:     UDP broadcast beacon so the robot can find this PC.
* AudioReceiver: UDP mic stream from the robot.
* AudioSender:   real-time paced UDP audio to the robot speaker.
"""

from __future__ import annotations

import asyncio
import itertools
import json
import logging
import socket
import time
from collections.abc import Callable
from typing import Any

import numpy as np

from .config import NetworkConfig
from .protocol import (
    FLAG_END_OF_UTTERANCE,
    MIC_MAGIC,
    SAMPLE_RATE,
    SPEAKER_MAGIC,
    beacon_message,
    pack_audio,
    unpack_audio,
)

log = logging.getLogger(__name__)


class RobotLink:
    def __init__(self, cfg: NetworkConfig) -> None:
        self.cfg = cfg
        self.telemetry: dict[str, Any] = {}
        self.robot_info: dict[str, Any] = {}
        self.on_event: Callable[[str], None] | None = None
        self.on_connect: Callable[[], None] | None = None
        self._writer: asyncio.StreamWriter | None = None
        self._ids = itertools.count(1)
        self._pending: dict[int, asyncio.Future[bool]] = {}
        self._server: asyncio.base_events.Server | None = None

    @property
    def connected(self) -> bool:
        return self._writer is not None and not self._writer.is_closing()

    @property
    def robot_ip(self) -> str | None:
        if not self.connected:
            return None
        peer = self._writer.get_extra_info("peername")  # type: ignore[union-attr]
        return peer[0] if peer else None

    async def start(self) -> None:
        self._server = await asyncio.start_server(self._on_client, self.cfg.host, self.cfg.control_port)
        asyncio.create_task(self._keepalive())
        log.info("Control server listening on %s:%d", self.cfg.host, self.cfg.control_port)

    async def _on_client(self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter) -> None:
        peer = writer.get_extra_info("peername")
        if self._writer is not None:
            log.info("New robot connection replaces the old one")
            self._writer.close()
        self._writer = writer
        log.info("Robot connected from %s", peer)
        try:
            while True:
                line = await reader.readline()
                if not line:
                    break
                try:
                    msg = json.loads(line)
                except json.JSONDecodeError:
                    log.warning("Bad JSON from robot: %r", line[:120])
                    continue
                self._handle(msg)
        except (ConnectionError, asyncio.IncompleteReadError):
            pass
        finally:
            if self._writer is writer:
                self._writer = None
                for fut in self._pending.values():
                    if not fut.done():
                        fut.set_result(False)
                self._pending.clear()
                log.warning("Robot disconnected")

    def _handle(self, msg: dict[str, Any]) -> None:
        kind = msg.get("type")
        if kind == "status":
            self.telemetry = msg
        elif kind == "hello":
            self.robot_info = msg
            log.info("Robot says hello: %s (fw %s)", msg.get("name"), msg.get("fw"))
            if self.on_connect:
                self.on_connect()
        elif kind == "done":
            fut = self._pending.pop(int(msg.get("id", -1)), None)
            if fut is not None and not fut.done():
                fut.set_result(True)
        elif kind == "event":
            log.info("Robot event: %s", msg.get("name"))
            if self.on_event:
                self.on_event(str(msg.get("name")))
        elif kind == "error":
            log.warning("Robot error: %s", msg)

    async def send(self, msg: dict[str, Any]) -> bool:
        if not self.connected:
            return False
        assert self._writer is not None
        try:
            self._writer.write((json.dumps(msg, separators=(",", ":")) + "\n").encode())
            await self._writer.drain()
            return True
        except ConnectionError:
            return False

    async def command(self, msg: dict[str, Any], wait: bool = True, timeout: float | None = None) -> bool:
        """Send a command; optionally wait until the robot reports it done."""
        waitable = msg.get("cmd") in ("move", "turn", "drive", "head", "lift", "anim")
        if not (wait and waitable):
            return await self.send(msg)
        msg = dict(msg, id=next(self._ids))
        fut: asyncio.Future[bool] = asyncio.get_running_loop().create_future()
        self._pending[msg["id"]] = fut
        if not await self.send(msg):
            self._pending.pop(msg["id"], None)
            return False
        try:
            return await asyncio.wait_for(fut, timeout or self.cfg.command_timeout_s)
        except asyncio.TimeoutError:
            self._pending.pop(msg["id"], None)
            log.warning("Timed out waiting for %s", msg.get("cmd"))
            return False

    async def _keepalive(self) -> None:
        while True:
            await asyncio.sleep(1.0)
            await self.send({"cmd": "ping"})


class Discovery:
    """Broadcasts 'COZMO_SERVER <port>' once a second."""

    def __init__(self, cfg: NetworkConfig) -> None:
        self.cfg = cfg

    async def run(self) -> None:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        sock.setblocking(False)
        message = beacon_message(self.cfg.control_port)
        log.info("Broadcasting discovery beacon to %s:%d", self.cfg.broadcast_address, self.cfg.discovery_port)
        while True:
            try:
                sock.sendto(message, (self.cfg.broadcast_address, self.cfg.discovery_port))
            except OSError as exc:
                log.debug("Beacon send failed: %s", exc)
            await asyncio.sleep(1.0)


class AudioReceiver(asyncio.DatagramProtocol):
    """Receives mic frames: callback(samples_int16, flags)."""

    def __init__(self, callback: Callable[[np.ndarray, int], None]) -> None:
        self._callback = callback
        self._last_seq: int | None = None
        self.lost = 0
        self.received = 0

    def datagram_received(self, data: bytes, addr: tuple[str, int]) -> None:
        packet = unpack_audio(data)
        if packet is None or packet.magic != MIC_MAGIC:
            return
        if self._last_seq is not None:
            gap = (packet.seq - self._last_seq - 1) & 0xFFFFFFFF
            if 0 < gap < 1000:
                self.lost += gap
        self._last_seq = packet.seq
        self.received += 1
        self._callback(packet.samples, packet.flags)


class AudioSender:
    """Streams int16 PCM to the robot speaker at real-time pace."""

    FRAME = 320  # 20 ms
    PREBUFFER_FRAMES = 10  # 200 ms sent up front to absorb WiFi jitter

    def __init__(self, cfg: NetworkConfig) -> None:
        self.cfg = cfg
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self._seq = 0
        self._cancel = False

    def cancel(self) -> None:
        self._cancel = True

    async def play(self, pcm: np.ndarray, robot_ip: str) -> float:
        """Send audio; returns when the last packet is sent. Returns duration (s)."""
        self._cancel = False
        frames = [pcm[i : i + self.FRAME] for i in range(0, len(pcm), self.FRAME)]
        start = time.monotonic()
        for n, frame in enumerate(frames):
            if self._cancel:
                break
            flags = FLAG_END_OF_UTTERANCE if n == len(frames) - 1 else 0
            self._sock.sendto(pack_audio(SPEAKER_MAGIC, self._seq, frame, flags), (robot_ip, self.cfg.robot_audio_port))
            self._seq += 1
            due = start + (n + 1 - self.PREBUFFER_FRAMES) * self.FRAME / SAMPLE_RATE
            delay = due - time.monotonic()
            if delay > 0:
                await asyncio.sleep(delay)
        return len(pcm) / SAMPLE_RATE
