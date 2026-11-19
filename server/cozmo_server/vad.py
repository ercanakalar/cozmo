"""Voice activity detection: turns a stream of 20 ms frames into utterances."""

from __future__ import annotations

import collections
import logging
from dataclasses import dataclass
from enum import Enum, auto

import numpy as np

from .audio_utils import rms_dbfs
from .config import VadConfig
from .protocol import SAMPLE_RATE

log = logging.getLogger(__name__)


class VadEvent(Enum):
    SPEECH_START = auto()
    SPEECH_END = auto()


@dataclass
class VadResult:
    event: VadEvent
    audio: np.ndarray | None = None  # full utterance (int16) on SPEECH_END


class _WebRtcClassifier:
    def __init__(self, aggressiveness: int) -> None:
        import webrtcvad  # optional dependency

        self._vad = webrtcvad.Vad(aggressiveness)

    def is_speech(self, frame: np.ndarray) -> bool:
        # webrtcvad accepts 10/20/30 ms frames.
        return self._vad.is_speech(frame.astype("<i2").tobytes(), SAMPLE_RATE)


class Segmenter:
    """Energy (or WebRTC) VAD with an adaptive noise floor and hysteresis."""

    def __init__(self, config: VadConfig, frame_ms: int = 20) -> None:
        self.cfg = config
        self.frame_ms = frame_ms
        self.noise_db = -60.0
        self._in_speech = False
        self._speech_frames = 0
        self._silence_frames = 0
        self._utterance: list[np.ndarray] = []
        self._preroll: collections.deque[np.ndarray] = collections.deque(
            maxlen=max(1, config.preroll_ms // frame_ms)
        )
        self._pending: list[np.ndarray] = []
        self._webrtc = None
        if config.backend == "webrtc":
            try:
                self._webrtc = _WebRtcClassifier(config.webrtc_aggressiveness)
            except ImportError:
                log.warning("webrtcvad not installed - falling back to energy VAD")

    @property
    def in_speech(self) -> bool:
        return self._in_speech

    def reset(self) -> None:
        self._in_speech = False
        self._speech_frames = 0
        self._silence_frames = 0
        self._utterance.clear()
        self._pending.clear()
        self._preroll.clear()

    def _frame_is_speech(self, frame: np.ndarray, level: float) -> bool:
        margin = self.cfg.stop_margin_db if self._in_speech else self.cfg.start_margin_db
        loud = level > self.noise_db + margin
        if self._webrtc is not None:
            return loud and self._webrtc.is_speech(frame)
        return loud

    def process(self, frame: np.ndarray) -> VadResult | None:
        level = rms_dbfs(frame)
        speech = self._frame_is_speech(frame, level)

        # Track the noise floor only on non-speech frames (fast down, slow up).
        if not speech and not self._in_speech:
            alpha = 0.2 if level < self.noise_db else 0.01
            self.noise_db = (1 - alpha) * self.noise_db + alpha * level
            self.noise_db = max(self.noise_db, -75.0)

        min_speech = max(1, self.cfg.min_speech_ms // self.frame_ms)
        end_silence = max(1, self.cfg.end_silence_ms // self.frame_ms)
        max_frames = int(self.cfg.max_utterance_s * 1000 / self.frame_ms)

        if not self._in_speech:
            if speech:
                self._speech_frames += 1
                self._pending.append(frame)
                if self._speech_frames >= min_speech:
                    self._in_speech = True
                    self._silence_frames = 0
                    self._utterance = list(self._preroll) + self._pending
                    self._pending = []
                    self._preroll.clear()
                    return VadResult(VadEvent.SPEECH_START)
            else:
                self._speech_frames = 0
                for f in self._pending:
                    self._preroll.append(f)
                self._pending = []
                self._preroll.append(frame)
            return None

        self._utterance.append(frame)
        self._silence_frames = 0 if speech else self._silence_frames + 1
        if self._silence_frames >= end_silence or len(self._utterance) >= max_frames:
            # Trim most of the trailing silence, keep ~200 ms.
            keep_tail = max(0, self._silence_frames - 200 // self.frame_ms)
            frames = self._utterance[: len(self._utterance) - keep_tail]
            audio = np.concatenate(frames) if frames else np.zeros(0, dtype=np.int16)
            self.reset()
            return VadResult(VadEvent.SPEECH_END, audio)
        return None
