"""Optional free-form conversation using the Claude API.

When enabled, anything the rule-based parser does not understand is sent to
Claude together with a short description of the robot's abilities. Claude
answers with a JSON object (enforced by structured outputs) holding the
spoken reply and a list of robot actions.

Needs `pip install anthropic` and an API key (ANTHROPIC_API_KEY or
`ant auth login`).
"""

from __future__ import annotations

import json
import logging
from typing import Any

from .config import LlmConfig

log = logging.getLogger(__name__)

ANIMATIONS = [
    "happy", "sad", "angry", "surprised", "celebrate", "curious", "sleep", "wake", "nod_yes",
    "shake_no", "dance", "greet", "scared", "bored", "thinking", "purr", "lift_up", "lift_down",
]
EXPRESSIONS = [
    "neutral", "happy", "sad", "angry", "surprised", "sleepy", "suspicious", "scared", "focused",
    "bored", "excited", "squint",
]
ACTION_TYPES = ["move", "turn", "head", "lift", "anim", "face", "stop"]

SCHEMA: dict[str, Any] = {
    "type": "object",
    "properties": {
        "reply": {"type": "string"},
        "actions": {
            "type": "array",
            "items": {
                "type": "object",
                "properties": {
                    "type": {"type": "string", "enum": ACTION_TYPES},
                    "value": {"type": "number"},
                    "name": {"type": "string"},
                },
                "required": ["type", "value", "name"],
                "additionalProperties": False,
            },
        },
    },
    "required": ["reply", "actions"],
    "additionalProperties": False,
}

SYSTEM_PROMPT = """\
You are {name}, a small, curious and playful desk robot with tank treads, a lift \
with two forks at the front, a tilting head and an OLED screen showing two \
expressive eyes. You hear people through a microphone and talk through a tiny \
speaker. Speak {language_name}.

Keep spoken replies short: one or two simple sentences, friendly, like a cheerful \
little robot pet. Never use emoji, markdown or lists in the reply, it is read \
aloud by a text-to-speech engine.

You can act by returning actions, executed in order:
- move: drive straight, value = millimetres (negative = backwards, max 500)
- turn: turn in place, value = degrees (positive = left, negative = right)
- head: tilt head, value = degrees (-25 down .. 44 up)
- lift: raise the forks, value = millimetres (0 .. 60)
- anim: play an animation, name = one of: {animations}
- face: show an expression for 3 s, name = one of: {expressions}
- stop: stop moving
Unused fields: value 0, name "". Prefer an animation that fits your emotion. \
Only move when asked to; you are on a table and could fall off.

Robot status: {status}
"""


class LlmBrain:
    def __init__(self, cfg: LlmConfig, name: str, language: str) -> None:
        import anthropic  # optional dependency

        self._anthropic = anthropic
        self._client = anthropic.Anthropic()
        self._cfg = cfg
        self._name = name
        self._language_name = "Turkish" if language == "tr" else "English"
        self._history: list[dict[str, str]] = []

    def respond(self, text: str, status: dict[str, Any]) -> tuple[str, list[dict[str, Any]]]:
        """Blocking call; run it in a thread. Returns (reply, robot commands)."""
        system = SYSTEM_PROMPT.format(
            name=self._name,
            language_name=self._language_name,
            animations=", ".join(ANIMATIONS),
            expressions=", ".join(EXPRESSIONS),
            status=json.dumps(
                {k: status.get(k) for k in ("battery_pct", "picked_up", "head_deg", "lift_mm")}
            ),
        )
        messages = self._history + [{"role": "user", "content": text}]
        try:
            response = self._client.beta.messages.create(
                model=self._cfg.model,
                max_tokens=16000,
                system=system,
                messages=messages,
                output_config={
                    "effort": self._cfg.effort,
                    "format": {"type": "json_schema", "schema": SCHEMA},
                },
                betas=["server-side-fallback-2026-07-01"],
                fallbacks="default",
            )
        except self._anthropic.APIConnectionError as exc:
            log.warning("Claude API unreachable: %s", exc)
            return "", []
        except self._anthropic.RateLimitError:
            log.warning("Claude API rate limited")
            return "", []
        except self._anthropic.APIStatusError as exc:
            log.error("Claude API error %s: %s", exc.status_code, exc.message)
            return "", []

        if response.stop_reason == "refusal":
            log.info("Claude declined to answer")
            return "", []

        raw = next((b.text for b in response.content if b.type == "text"), "")
        try:
            data = json.loads(raw)
        except json.JSONDecodeError:
            log.warning("Unparseable LLM output: %r", raw[:200])
            return "", []

        reply = str(data.get("reply", "")).strip()
        self._history = (messages + [{"role": "assistant", "content": raw}])[
            -2 * self._cfg.max_history_turns :
        ]
        return reply, to_commands(data.get("actions", []))


def _clamp(v: float, lo: float, hi: float) -> float:
    return max(lo, min(hi, v))


def to_commands(actions: list[dict[str, Any]]) -> list[dict[str, Any]]:
    """Validate LLM actions and convert them to firmware commands."""
    commands: list[dict[str, Any]] = []
    for action in actions[:8]:
        kind = action.get("type")
        try:
            value = float(action.get("value", 0) or 0)
        except (TypeError, ValueError):
            value = 0.0
        name = str(action.get("name", ""))
        if kind == "move" and value:
            commands.append({"cmd": "move", "distance_mm": round(_clamp(value, -500, 500)), "speed": 120})
        elif kind == "turn" and value:
            commands.append({"cmd": "turn", "angle_deg": round(_clamp(value, -720, 720)), "speed_dps": 150})
        elif kind == "head":
            commands.append({"cmd": "head", "angle_deg": _clamp(value, -25, 44), "duration_ms": 400})
        elif kind == "lift":
            commands.append({"cmd": "lift", "height_mm": _clamp(value, 0, 60), "duration_ms": 500})
        elif kind == "anim" and name in ANIMATIONS:
            commands.append({"cmd": "anim", "name": name})
        elif kind == "face" and name in EXPRESSIONS:
            commands.append({"cmd": "face", "expression": name, "duration_ms": 3000})
        elif kind == "stop":
            commands.append({"cmd": "stop"})
    return commands
