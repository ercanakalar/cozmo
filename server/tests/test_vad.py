import numpy as np

from cozmo_server.config import VadConfig
from cozmo_server.vad import Segmenter, VadEvent

RATE = 16000
FRAME = 320


def frames(signal):
    for i in range(0, len(signal) - FRAME + 1, FRAME):
        yield signal[i : i + FRAME]


def test_detects_one_utterance():
    rng = np.random.default_rng(0)
    noise = (rng.normal(0, 30, RATE * 2)).astype(np.int16)  # quiet room
    t = np.arange(int(RATE * 1.0)) / RATE
    speech = (6000 * np.sin(2 * np.pi * 300 * t)).astype(np.int16)
    signal = np.concatenate([noise, speech, noise])

    seg = Segmenter(VadConfig())
    events = []
    for f in frames(signal):
        r = seg.process(f)
        if r:
            events.append(r)

    assert [e.event for e in events] == [VadEvent.SPEECH_START, VadEvent.SPEECH_END]
    duration = len(events[1].audio) / RATE
    assert 0.9 < duration < 1.8  # speech + pre-roll + short tail


def test_ignores_short_clicks():
    rng = np.random.default_rng(1)
    noise = rng.normal(0, 30, RATE).astype(np.int16)
    click = np.full(FRAME * 2, 8000, dtype=np.int16)  # 40 ms, shorter than min_speech_ms
    signal = np.concatenate([noise, click, noise])
    seg = Segmenter(VadConfig())
    assert all(seg.process(f) is None for f in frames(signal))


def test_max_length_cuts_utterance():
    cfg = VadConfig(max_utterance_s=1.0)
    seg = Segmenter(cfg)
    rng = np.random.default_rng(2)
    for f in frames(rng.normal(0, 30, RATE).astype(np.int16)):
        seg.process(f)
    loud = (6000 * np.sin(np.arange(RATE * 3) * 0.1)).astype(np.int16)
    ends = [r for f in frames(loud) if (r := seg.process(f)) and r.event is VadEvent.SPEECH_END]
    assert ends, "long speech must be split"
