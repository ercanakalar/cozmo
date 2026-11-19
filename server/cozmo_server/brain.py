"""The conversation pipeline: audio -> VAD -> ASR -> intent/LLM -> robot.

    robot mic --UDP--> on_audio() --VAD--> utterance queue --> worker:
        ASR (thread) -> wake word check -> intents.parse() [-> LLM]
        -> robot.command(...) for each action, TTS reply -> AudioSender
"""

from __future__ import annotations

import asyncio
import logging
import random
import time
from typing import Any

import numpy as np

from . import intents
from .asr import Recognizer
from .config import Config
from .protocol import FLAG_SPEAKER_ACTIVE
from .robot_link import AudioSender, RobotLink
from .tts import Synthesizer
from .vad import Segmenter, VadEvent

log = logging.getLogger(__name__)

EVENT_REACTIONS: dict[str, dict[str, list[str]]] = {
    "tr": {
        "picked_up": ["Beni yere indir!", "Yükseklik korkum var!", "Uçuyorum!"],
        "put_down": ["Oh be!", "Yere sağ salim indim."],
        "touched": ["Hihi, gıdıklanıyorum!", "Çok güzel!", ""],
        "battery_low": ["Pilim bitmek üzere, beni şarj eder misin?"],
    },
    "en": {
        "picked_up": ["Put me down!", "Whoa, I'm flying!", "I'm scared of heights!"],
        "put_down": ["Phew!", "Back on solid ground."],
        "touched": ["Hehe, that tickles!", "I like that!", ""],
        "battery_low": ["My battery is almost empty, can you charge me?"],
    },
}


class Brain:
    def __init__(
        self,
        cfg: Config,
        robot: RobotLink,
        recognizer: Recognizer,
        synthesizer: Synthesizer,
        sender: AudioSender,
        llm: Any | None = None,
    ) -> None:
        self.cfg = cfg
        self.robot = robot
        self.recognizer = recognizer
        self.synthesizer = synthesizer
        self.sender = sender
        self.llm = llm
        self.segmenter = Segmenter(cfg.vad)
        self.ctx = intents.Context(language=cfg.persona.language, name=cfg.persona.name)
        self._queue: asyncio.Queue[tuple[str, Any]] = asyncio.Queue(maxsize=2)
        self._speaking_until = 0.0
        self._conversation_until = 0.0
        self._loop: asyncio.AbstractEventLoop | None = None
        self._last_event_reply = 0.0

    # ------------------------------------------------------------------ state
    @property
    def in_conversation(self) -> bool:
        return time.monotonic() < self._conversation_until

    def _extend_conversation(self) -> None:
        self._conversation_until = time.monotonic() + self.cfg.persona.conversation_window_s

    def _listening_allowed(self) -> bool:
        return not self.cfg.persona.require_wake_word or self.in_conversation

    async def set_mode(self, mode: str) -> None:
        await self.robot.send({"cmd": "mode", "value": mode})

    # ------------------------------------------------------------ audio input
    def on_audio(self, samples: np.ndarray, flags: int) -> None:
        """Called for every 20 ms mic frame (from the asyncio loop)."""
        speaking = time.monotonic() < self._speaking_until or bool(flags & FLAG_SPEAKER_ACTIVE)
        if self.cfg.vad.ignore_while_speaking and speaking:
            if self.segmenter.in_speech:
                self.segmenter.reset()
            return
        result = self.segmenter.process(samples)
        if result is None:
            return
        if result.event is VadEvent.SPEECH_START:
            log.debug("Speech start (noise floor %.1f dBFS)", self.segmenter.noise_db)
            if self._listening_allowed():
                asyncio.ensure_future(self.set_mode("listening"))
        elif result.event is VadEvent.SPEECH_END and result.audio is not None:
            duration = len(result.audio) / 16000
            log.debug("Speech end: %.2f s", duration)
            try:
                self._queue.put_nowait(("audio", result.audio))
            except asyncio.QueueFull:
                log.warning("Still busy - dropping utterance")

    def on_text(self, text: str) -> None:
        """Typed input (keyboard mode) skips ASR and the wake word."""
        try:
            self._queue.put_nowait(("text", text))
        except asyncio.QueueFull:
            log.warning("Still busy - dropping input")

    def on_event(self, name: str) -> None:
        try:
            self._queue.put_nowait(("event", name))
        except asyncio.QueueFull:
            pass

    # ----------------------------------------------------------------- worker
    async def run(self) -> None:
        self._loop = asyncio.get_running_loop()
        while True:
            kind, payload = await self._queue.get()
            try:
                if kind == "audio":
                    await self._handle_audio(payload)
                elif kind == "text":
                    await self.handle_text(payload, require_wake=False)
                elif kind == "event":
                    await self._handle_event(payload)
            except Exception:  # noqa: BLE001 - never let one bad turn kill the robot
                log.exception("Error while handling %s", kind)
                await self.set_mode("idle")

    async def _handle_audio(self, pcm: np.ndarray) -> None:
        if self._listening_allowed():
            await self.set_mode("thinking")
        t0 = time.monotonic()
        text = await asyncio.to_thread(self.recognizer.transcribe, pcm)
        log.info("Heard: %r (%.2f s ASR)", text, time.monotonic() - t0)
        if not text:
            await self.set_mode("idle")
            return
        await self.handle_text(text, require_wake=self.cfg.persona.require_wake_word)

    async def handle_text(self, text: str, require_wake: bool) -> None:
        wake_words = self.cfg.persona.wake_words
        has_wake = intents.contains_wake_word(text, wake_words)
        if require_wake and not has_wake and not self.in_conversation:
            log.info("No wake word - ignoring")
            await self.set_mode("idle")
            return
        self._extend_conversation()
        command = intents.strip_wake_word(text, wake_words)
        self.ctx.battery_pct = self.robot.telemetry.get("battery_pct")

        if not command:  # just the name: "Cozmo!"
            await self.set_mode("listening")
            await self.robot.command({"cmd": "anim", "name": "curious"}, wait=False)
            return

        responses = intents.parse(command, self.ctx)
        use_llm = self.llm is not None and (not responses or not self.cfg.llm.use_for_unknown_only)
        if use_llm:
            await self.set_mode("thinking")
            reply, actions = await asyncio.to_thread(self.llm.respond, command, self.robot.telemetry)
            if reply or actions:
                responses = [intents.Response("llm", actions, reply)]
        if not responses:
            responses = [intents.unknown(self.ctx)]

        for response in responses:
            log.info("Intent: %s -> %s | reply: %r", response.intent, response.actions, response.reply)
            # Speak and act at the same time, like a living character.
            await asyncio.gather(self.speak(response.reply), self.execute(response.actions))
        self._extend_conversation()
        await self.set_mode("idle")

    async def _handle_event(self, name: str) -> None:
        if not self.cfg.persona.react_to_events:
            return
        options = EVENT_REACTIONS[self.cfg.persona.language].get(name)
        now = time.monotonic()
        if not options or now - self._last_event_reply < 6.0:
            return
        self._last_event_reply = now
        await self.speak(random.choice(options))

    # ---------------------------------------------------------------- output
    async def execute(self, actions: list[dict[str, Any]]) -> None:
        for action in actions:
            if not self.robot.connected:
                log.warning("Robot not connected, skipping %s", action)
                return
            await self.robot.command(action)

    async def speak(self, text: str) -> None:
        if not text:
            return
        log.info("Say: %s", text)
        pcm = await asyncio.to_thread(self.synthesizer.synthesize, text)
        ip = self.robot.robot_ip
        if len(pcm) == 0 or ip is None:
            return
        duration = len(pcm) / 16000
        self._speaking_until = time.monotonic() + duration + 0.6
        await self.set_mode("speaking")
        await self.sender.play(pcm, ip)
        # Wait for the robot to finish playing its buffered audio.
        remaining = self._speaking_until - 0.6 - time.monotonic()
        if remaining > 0:
            await asyncio.sleep(remaining)
