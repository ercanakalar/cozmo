"""Speech recognition back-ends.

* faster-whisper (default): best accuracy, good Turkish, runs on CPU or GPU.
* vosk: very light, fully offline, needs a downloaded model folder.
* none: disables ASR (use the keyboard mode for testing).
"""

from __future__ import annotations

import json
import logging
from typing import Protocol

import numpy as np

from .config import AsrConfig
from .protocol import SAMPLE_RATE

log = logging.getLogger(__name__)


class Recognizer(Protocol):
    def transcribe(self, pcm: np.ndarray) -> str: ...


class NullRecognizer:
    def transcribe(self, pcm: np.ndarray) -> str:
        return ""


class FasterWhisperRecognizer:
    def __init__(self, cfg: AsrConfig, language: str, hint_words: list[str]) -> None:
        from faster_whisper import WhisperModel

        device = cfg.device
        if device == "auto":
            try:
                import ctranslate2

                device = "cuda" if ctranslate2.get_cuda_device_count() > 0 else "cpu"
            except Exception:  # noqa: BLE001 - any failure means "no GPU"
                device = "cpu"
        compute = cfg.compute_type if device == "cpu" else "float16"
        log.info("Loading faster-whisper model '%s' on %s (%s)...", cfg.model, device, compute)
        self._model = WhisperModel(cfg.model, device=device, compute_type=compute)
        self._language = language
        self._beam = cfg.beam_size
        # Biasing the decoder towards the robot's name helps a lot.
        self._prompt = ", ".join(hint_words)

    def transcribe(self, pcm: np.ndarray) -> str:
        audio = pcm.astype(np.float32) / 32768.0
        segments, _ = self._model.transcribe(
            audio,
            language=self._language,
            beam_size=self._beam,
            vad_filter=False,
            initial_prompt=self._prompt,
            condition_on_previous_text=False,
        )
        return " ".join(s.text.strip() for s in segments).strip()


class VoskRecognizer:
    def __init__(self, cfg: AsrConfig) -> None:
        from vosk import KaldiRecognizer, Model

        log.info("Loading Vosk model from %s...", cfg.vosk_model_path)
        self._model = Model(cfg.vosk_model_path)
        self._recognizer_cls = KaldiRecognizer

    def transcribe(self, pcm: np.ndarray) -> str:
        rec = self._recognizer_cls(self._model, SAMPLE_RATE)
        rec.AcceptWaveform(pcm.astype("<i2").tobytes())
        return json.loads(rec.FinalResult()).get("text", "").strip()


def create_recognizer(cfg: AsrConfig, language: str, hint_words: list[str]) -> Recognizer:
    if cfg.backend == "faster-whisper":
        return FasterWhisperRecognizer(cfg, language, hint_words)
    if cfg.backend == "vosk":
        return VoskRecognizer(cfg)
    if cfg.backend == "none":
        return NullRecognizer()
    raise ValueError(f"Unknown ASR backend: {cfg.backend}")
