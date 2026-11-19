from pathlib import Path

import numpy as np
import pytest

from cozmo_server.audio_utils import resample, robot_voice
from cozmo_server.config import load_config
from cozmo_server.llm_brain import to_commands

ROOT = Path(__file__).resolve().parent.parent


def test_default_config_file_loads():
    cfg = load_config(ROOT / "config.toml")
    assert cfg.network.control_port == 5008
    assert cfg.persona.language in ("tr", "en")


def test_unknown_config_key_rejected(tmp_path):
    p = tmp_path / "c.toml"
    p.write_text("[network]\nbogus = 1\n")
    with pytest.raises(ValueError):
        load_config(p)


def test_resample_length():
    x = np.sin(np.arange(22050) * 0.01).astype(np.float32)
    y = resample(x, 22050, 16000)
    assert abs(len(y) - 16000) <= 1


def test_robot_voice_normalises():
    x = 0.1 * np.sin(np.arange(16000) * 0.05)
    y = robot_voice(x, 16000, pitch=1.2)
    assert len(y) < len(x)
    assert 0.9 < np.max(np.abs(y)) <= 0.96


def test_llm_actions_are_validated_and_clamped():
    cmds = to_commands(
        [
            {"type": "move", "value": 5000, "name": ""},
            {"type": "turn", "value": -90, "name": ""},
            {"type": "anim", "value": 0, "name": "dance"},
            {"type": "anim", "value": 0, "name": "self_destruct"},
            {"type": "head", "value": 99, "name": ""},
            {"type": "face", "value": 0, "name": "happy"},
            {"type": "launch_rockets", "value": 1, "name": ""},
        ]
    )
    assert cmds == [
        {"cmd": "move", "distance_mm": 500, "speed": 120},
        {"cmd": "turn", "angle_deg": -90, "speed_dps": 150},
        {"cmd": "anim", "name": "dance"},
        {"cmd": "head", "angle_deg": 44, "duration_ms": 400},
        {"cmd": "face", "expression": "happy", "duration_ms": 3000},
    ]
