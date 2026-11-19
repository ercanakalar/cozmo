"""Wire protocol shared with the firmware (firmware/src/net/Protocol.h).

See docs/PROTOCOL.md for the full description.
"""

from __future__ import annotations

import struct
from dataclasses import dataclass

import numpy as np

SAMPLE_RATE = 16_000
MIC_MAGIC = b"CZMU"
SPEAKER_MAGIC = b"CZSP"
BEACON_PREFIX = "COZMO_SERVER"
MAX_SPEAKER_SAMPLES = 700

FLAG_SPEAKER_ACTIVE = 1 << 0  # mic packet: robot speaker was playing
FLAG_MOTORS_ACTIVE = 1 << 1  # mic packet: motors/servos moving
FLAG_END_OF_UTTERANCE = 1 << 0  # speaker packet: last packet of a clip

_HEADER = struct.Struct("<4sIHH")
HEADER_SIZE = _HEADER.size  # 12 bytes


@dataclass(frozen=True)
class AudioPacket:
    magic: bytes
    seq: int
    flags: int
    samples: np.ndarray  # int16 mono


def pack_audio(magic: bytes, seq: int, samples: np.ndarray, flags: int = 0) -> bytes:
    pcm = np.asarray(samples, dtype="<i2")
    return _HEADER.pack(magic, seq & 0xFFFFFFFF, len(pcm), flags) + pcm.tobytes()


def unpack_audio(data: bytes) -> AudioPacket | None:
    if len(data) < HEADER_SIZE:
        return None
    magic, seq, count, flags = _HEADER.unpack_from(data)
    if magic not in (MIC_MAGIC, SPEAKER_MAGIC):
        return None
    available = (len(data) - HEADER_SIZE) // 2
    count = min(count, available)
    samples = np.frombuffer(data, dtype="<i2", count=count, offset=HEADER_SIZE)
    return AudioPacket(magic=magic, seq=seq, flags=flags, samples=samples)


def beacon_message(control_port: int) -> bytes:
    return f"{BEACON_PREFIX} {control_port}\n".encode()
