"""Configuration loading (TOML file + defaults)."""

from __future__ import annotations

import tomllib
from dataclasses import dataclass, field, fields, is_dataclass
from pathlib import Path
from typing import Any


@dataclass
class NetworkConfig:
    host: str = "0.0.0.0"
    control_port: int = 5008
    audio_port: int = 5005
    robot_audio_port: int = 5007
    discovery_port: int = 5006
    broadcast_address: str = "255.255.255.255"
    command_timeout_s: float = 8.0


@dataclass
class PersonaConfig:
    name: str = "Cozmo"
    language: str = "tr"  # "tr" or "en"
    wake_words: list[str] = field(
        default_factory=lambda: ["cozmo", "kozmo", "kosmo", "kozmos", "cosmo", "kuzmo", "gozmo"]
    )
    require_wake_word: bool = True
    conversation_window_s: float = 12.0
    react_to_events: bool = True


@dataclass
class VadConfig:
    backend: str = "energy"  # energy | webrtc
    start_margin_db: float = 12.0  # how far above the noise floor speech must be
    stop_margin_db: float = 7.0
    min_speech_ms: int = 200
    end_silence_ms: int = 700
    max_utterance_s: float = 10.0
    preroll_ms: int = 300
    webrtc_aggressiveness: int = 2
    ignore_while_speaking: bool = True


@dataclass
class AsrConfig:
    backend: str = "faster-whisper"  # faster-whisper | vosk | none
    model: str = "small"
    device: str = "auto"
    compute_type: str = "int8"
    beam_size: int = 3
    vosk_model_path: str = "models/vosk-model-small-tr-0.3"


@dataclass
class TtsConfig:
    backend: str = "espeak"  # espeak | piper | none
    voice: str = ""  # espeak voice; empty = persona language
    rate_wpm: int = 165
    piper_model: str = "models/tr_TR-dfki-medium.onnx"
    piper_binary: str = "piper"
    robot_effect: bool = True
    pitch: float = 1.18  # >1 = higher, faster, more "small robot"
    volume: float = 0.9


@dataclass
class LlmConfig:
    enabled: bool = False
    model: str = "claude-opus-5-5"
    effort: str = "low"
    max_history_turns: int = 8
    use_for_unknown_only: bool = True


@dataclass
class Config:
    network: NetworkConfig = field(default_factory=NetworkConfig)
    persona: PersonaConfig = field(default_factory=PersonaConfig)
    vad: VadConfig = field(default_factory=VadConfig)
    asr: AsrConfig = field(default_factory=AsrConfig)
    tts: TtsConfig = field(default_factory=TtsConfig)
    llm: LlmConfig = field(default_factory=LlmConfig)


def _apply(target: Any, values: dict[str, Any], path: str = "") -> None:
    known = {f.name: f for f in fields(target)}
    for key, value in values.items():
        if key not in known:
            raise ValueError(f"Unknown config key: {path}{key}")
        current = getattr(target, key)
        if is_dataclass(current):
            if not isinstance(value, dict):
                raise ValueError(f"Config section {path}{key} must be a table")
            _apply(current, value, f"{path}{key}.")
        else:
            setattr(target, key, value)


def load_config(path: str | Path | None) -> Config:
    config = Config()
    if path is None:
        return config
    with open(path, "rb") as fh:
        data = tomllib.load(fh)
    _apply(config, data)
    if config.persona.language not in ("tr", "en"):
        raise ValueError("persona.language must be 'tr' or 'en'")
    return config
