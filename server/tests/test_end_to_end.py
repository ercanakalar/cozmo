"""Server pipeline against an in-process fake robot over real TCP."""

import asyncio
import json

from cozmo_server.asr import NullRecognizer
from cozmo_server.brain import Brain
from cozmo_server.config import Config
from cozmo_server.robot_link import AudioSender, RobotLink
from cozmo_server.tts import NullSynthesizer


async def fake_robot(port, received, ready):
    reader, writer = await asyncio.open_connection("127.0.0.1", port)
    writer.write(b'{"type":"hello","name":"cozmo","fw":"test"}\n')
    writer.write(b'{"type":"status","battery_pct":55}\n')
    await writer.drain()
    ready.set()
    while True:
        line = await reader.readline()
        if not line:
            return
        msg = json.loads(line)
        if msg.get("cmd") in ("ping", "mode"):
            continue
        received.append(msg)
        if "id" in msg:
            writer.write((json.dumps({"type": "done", "id": msg["id"]}) + "\n").encode())
            await writer.drain()


async def scenario():
    cfg = Config()
    cfg.network.host = "127.0.0.1"
    cfg.network.control_port = 0
    robot = RobotLink(cfg.network)
    await robot.start()
    port = robot._server.sockets[0].getsockname()[1]

    received, ready = [], asyncio.Event()
    task = asyncio.create_task(fake_robot(port, received, ready))
    await ready.wait()
    await asyncio.sleep(0.05)
    assert robot.connected

    brain = Brain(cfg, robot, NullRecognizer(), NullSynthesizer(), AudioSender(cfg.network))
    # Without the wake word and outside a conversation nothing happens.
    await brain.handle_text("ileri git", require_wake=True)
    assert received == []
    # With the wake word the commands are executed in order.
    await brain.handle_text("Kozmo ileri 20 santim git sonra sağa dön", require_wake=True)
    # Follow-up inside the conversation window needs no name.
    await brain.handle_text("dans et", require_wake=True)
    task.cancel()
    return received


def test_pipeline_sends_commands_in_order():
    received = asyncio.run(scenario())
    cmds = [(m["cmd"], m.get("distance_mm") or m.get("angle_deg") or m.get("name")) for m in received]
    assert cmds == [("move", 200), ("turn", -90), ("anim", "dance")]
