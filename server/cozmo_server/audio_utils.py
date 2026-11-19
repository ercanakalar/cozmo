"""Small numpy-only audio helpers (no scipy needed)."""

from __future__ import annotations

import io
import wave

import numpy as np


def to_float(pcm: np.ndarray) -> np.ndarray:
    return pcm.astype(np.float32) / 32768.0


def to_int16(audio: np.ndarray) -> np.ndarray:
    return (np.clip(audio, -1.0, 1.0) * 32767.0).astype(np.int16)


def rms_dbfs(pcm: np.ndarray) -> float:
    if len(pcm) == 0:
        return -120.0
    x = pcm.astype(np.float64)
    rms = np.sqrt(np.mean(x * x))
    return 20.0 * np.log10(max(rms, 1e-3) / 32768.0)


def resample(audio: np.ndarray, src_rate: int, dst_rate: int) -> np.ndarray:
    """Linear-interpolation resampler with a crude anti-alias pre-filter."""
    if src_rate == dst_rate or len(audio) == 0:
        return audio.astype(np.float32)
    audio = audio.astype(np.float32)
    if dst_rate < src_rate:
        # Moving-average low-pass sized to the decimation ratio.
        width = int(np.ceil(src_rate / dst_rate))
        if width > 1:
            kernel = np.ones(width, dtype=np.float32) / width
            audio = np.convolve(audio, kernel, mode="same")
    duration = len(audio) / src_rate
    n_out = max(1, int(round(duration * dst_rate)))
    src_t = np.arange(len(audio)) / src_rate
    dst_t = np.arange(n_out) / dst_rate
    return np.interp(dst_t, src_t, audio).astype(np.float32)


def robot_voice(audio: np.ndarray, rate: int, pitch: float = 1.18) -> np.ndarray:
    """Give TTS a small-robot character.

    Plays the clip back faster (raises pitch and tempo together, like a tiny
    speaker-bot), adds a faint ring modulation for a metallic timbre and
    normalises the level.
    """
    if len(audio) == 0:
        return audio
    if pitch != 1.0:
        n_out = max(1, int(len(audio) / pitch))
        audio = np.interp(np.linspace(0, len(audio) - 1, n_out), np.arange(len(audio)), audio)
    t = np.arange(len(audio)) / rate
    audio = audio * (0.85 + 0.15 * np.sin(2 * np.pi * 55.0 * t))
    peak = float(np.max(np.abs(audio))) or 1.0
    return (audio / peak * 0.95).astype(np.float32)


def read_wav(data: bytes) -> tuple[np.ndarray, int]:
    """Decode a PCM WAV (8/16/32-bit, any channels) into mono float32."""
    with wave.open(io.BytesIO(data), "rb") as wf:
        rate = wf.getframerate()
        width = wf.getsampwidth()
        channels = wf.getnchannels()
        frames = wf.readframes(wf.getnframes())
    if width == 2:
        audio = np.frombuffer(frames, dtype="<i2").astype(np.float32) / 32768.0
    elif width == 4:
        audio = np.frombuffer(frames, dtype="<i4").astype(np.float32) / 2147483648.0
    elif width == 1:
        audio = (np.frombuffer(frames, dtype=np.uint8).astype(np.float32) - 128.0) / 128.0
    else:
        raise ValueError(f"Unsupported WAV sample width: {width}")
    if channels > 1:
        audio = audio.reshape(-1, channels).mean(axis=1)
    return audio, rate


def write_wav(path: str, pcm: np.ndarray, rate: int) -> None:
    with wave.open(path, "wb") as wf:
        wf.setnchannels(1)
        wf.setsampwidth(2)
        wf.setframerate(rate)
        wf.writeframes(np.asarray(pcm, dtype="<i2").tobytes())
