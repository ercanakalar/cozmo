"""Text to speech back-ends. Output is always 16 kHz mono int16 for the robot.

* espeak: `espeak-ng` command line (tiny, robotic, supports Turkish).
* piper:  `piper` neural TTS (much more natural; Turkish voices available).
* none:   no speech, the robot only uses its chirps and animations.
"""

from __future__ import annotations

import logging
import shutil
import subprocess
from typing import Protocol

import numpy as np

from .audio_utils import read_wav, resample, robot_voice, to_int16
from .config import TtsConfig
from .protocol import SAMPLE_RATE

log = logging.getLogger(__name__)


class Synthesizer(Protocol):
    def synthesize(self, text: str) -> np.ndarray: ...


class NullSynthesizer:
    def synthesize(self, text: str) -> np.ndarray:
        return np.zeros(0, dtype=np.int16)


class _Base:
    def __init__(self, cfg: TtsConfig) -> None:
        self.cfg = cfg

    def _finish(self, audio: np.ndarray, rate: int) -> np.ndarray:
        if self.cfg.robot_effect:
            audio = robot_voice(audio, rate, self.cfg.pitch)
        audio = resample(audio, rate, SAMPLE_RATE) * self.cfg.volume
        return to_int16(audio)


class EspeakSynthesizer(_Base):
    def __init__(self, cfg: TtsConfig, language: str) -> None:
        super().__init__(cfg)
        self._binary = shutil.which("espeak-ng") or shutil.which("espeak")
        if self._binary is None:
            raise RuntimeError("espeak-ng not found. Install it (see server/README.md).")
        self._voice = cfg.voice or language

    def synthesize(self, text: str) -> np.ndarray:
        result = subprocess.run(
            [self._binary, "-v", self._voice, "-s", str(self.cfg.rate_wpm), "--stdout", text],
            capture_output=True,
            check=True,
            timeout=20,
        )
        audio, rate = read_wav(result.stdout)
        return self._finish(audio, rate)


class PiperSynthesizer(_Base):
    def __init__(self, cfg: TtsConfig) -> None:
        super().__init__(cfg)
        self._binary = shutil.which(cfg.piper_binary) or cfg.piper_binary
        self._rate = 22050
        config_path = cfg.piper_model + ".json"
        try:
            import json

            with open(config_path, encoding="utf-8") as fh:
                self._rate = int(json.load(fh)["audio"]["sample_rate"])
        except (OSError, KeyError, ValueError):
            log.warning("Could not read %s, assuming %d Hz", config_path, self._rate)

    def synthesize(self, text: str) -> np.ndarray:
        result = subprocess.run(
            [self._binary, "--model", self.cfg.piper_model, "--output_raw"],
            input=text.encode("utf-8"),
            capture_output=True,
            check=True,
            timeout=30,
        )
        audio = np.frombuffer(result.stdout, dtype="<i2").astype(np.float32) / 32768.0
        return self._finish(audio, self._rate)


def create_synthesizer(cfg: TtsConfig, language: str) -> Synthesizer:
    if cfg.backend == "espeak":
        return EspeakSynthesizer(cfg, language)
    if cfg.backend == "piper":
        return PiperSynthesizer(cfg)
    if cfg.backend == "none":
        return NullSynthesizer()
    raise ValueError(f"Unknown TTS backend: {cfg.backend}")
