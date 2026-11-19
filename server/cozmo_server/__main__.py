"""Entry point: python -m cozmo_server [--config config.toml] [--keyboard]"""

from __future__ import annotations

import argparse
import asyncio
import logging
import sys
from pathlib import Path

from .asr import NullRecognizer, create_recognizer
from .brain import Brain
from .config import load_config
from .robot_link import AudioReceiver, AudioSender, Discovery, RobotLink
from .tts import NullSynthesizer, create_synthesizer

log = logging.getLogger("cozmo_server")


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Cozmo replica PC server (speech brain)")
    default_cfg = Path(__file__).resolve().parent.parent / "config.toml"
    p.add_argument("--config", default=str(default_cfg) if default_cfg.exists() else None)
    p.add_argument("--keyboard", action="store_true", help="type commands instead of speaking")
    p.add_argument("--no-asr", action="store_true", help="disable speech recognition")
    p.add_argument("--no-tts", action="store_true", help="disable speech output")
    p.add_argument("--llm", action="store_true", help="enable the Claude conversation brain")
    p.add_argument("-v", "--verbose", action="store_true")
    return p.parse_args(argv)


async def keyboard_loop(brain: Brain) -> None:
    print("Keyboard mode: type a command (e.g. 'ileri git', 'dance') and press Enter.")
    while True:
        line = await asyncio.to_thread(sys.stdin.readline)
        if not line:
            return
        if line.strip():
            brain.on_text(line.strip())


async def main_async(args: argparse.Namespace) -> None:
    cfg = load_config(args.config)
    if args.llm:
        cfg.llm.enabled = True
    persona = cfg.persona

    recognizer = NullRecognizer() if (args.no_asr or args.keyboard) else create_recognizer(
        cfg.asr, persona.language, [persona.name, *persona.wake_words[:2]]
    )
    try:
        synthesizer = NullSynthesizer() if args.no_tts else create_synthesizer(cfg.tts, persona.language)
    except RuntimeError as exc:
        log.warning("%s - continuing without speech output", exc)
        synthesizer = NullSynthesizer()

    llm = None
    if cfg.llm.enabled:
        from .llm_brain import LlmBrain

        llm = LlmBrain(cfg.llm, persona.name, persona.language)
        log.info("Claude conversation brain enabled (%s)", cfg.llm.model)

    robot = RobotLink(cfg.network)
    sender = AudioSender(cfg.network)
    brain = Brain(cfg, robot, recognizer, synthesizer, sender, llm)
    robot.on_event = brain.on_event

    loop = asyncio.get_running_loop()
    await robot.start()
    receiver = AudioReceiver(brain.on_audio)
    await loop.create_datagram_endpoint(lambda: receiver, local_addr=(cfg.network.host, cfg.network.audio_port))
    log.info("Listening for robot audio on UDP %d", cfg.network.audio_port)

    tasks = [asyncio.create_task(Discovery(cfg.network).run()), asyncio.create_task(brain.run())]
    if args.keyboard:
        tasks.append(asyncio.create_task(keyboard_loop(brain)))
    await asyncio.gather(*tasks)


def main(argv: list[str] | None = None) -> None:
    args = parse_args(argv)
    logging.basicConfig(
        level=logging.DEBUG if args.verbose else logging.INFO,
        format="%(asctime)s %(levelname)-7s %(name)s: %(message)s",
        datefmt="%H:%M:%S",
    )
    try:
        asyncio.run(main_async(args))
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()
