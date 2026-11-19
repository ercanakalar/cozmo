import random

import pytest

from cozmo_server import intents
from cozmo_server.intents import Context, normalize, parse


def ctx(lang="tr"):
    return Context(language=lang, name="Cozmo", battery_pct=80, rng=random.Random(0))


def first(text, lang="tr"):
    results = parse(text, ctx(lang))
    assert results, f"nothing parsed for {text!r}"
    return results[0]


def test_normalize_turkish():
    assert normalize("İleri GİT, sağa dön!") == "ileri git saga don"
    assert normalize("KIRMIZI ışık") == "kirmizi isik"


@pytest.mark.parametrize(
    "text,intent",
    [
        ("ileri git", "forward"),
        ("öne git", "forward"),
        ("geri git", "backward"),
        ("sola dön", "turn_left"),
        ("sağa dön", "turn_right"),
        ("arkanı dön", "turn_around"),
        ("kendi etrafında dön", "spin"),
        ("dur", "stop"),
        ("kafanı kaldır", "head_up"),
        ("aşağı bak", "head_down"),
        ("kolunu kaldır", "lift_up"),
        ("kolunu indir", "lift_down"),
        ("dans et", "dance"),
        ("aferin sana", "praise"),
        ("merhaba", "greet"),
        ("nasılsın", "how_are_you"),
        ("adın ne", "name"),
        ("pilin kaç", "battery"),
        ("saat kaç", "time"),
        ("bana bir şaka yap", "joke"),
        ("iyi geceler", "sleep"),
        ("uyan", "wake"),
        ("buraya gel", "come_here"),
        ("etrafına bak", "look_around"),
        ("sesini aç", "volume_up"),
    ],
)
def test_turkish_commands(text, intent):
    assert first(text).intent == intent


@pytest.mark.parametrize(
    "text,intent",
    [
        ("go forward", "forward"),
        ("turn left", "turn_left"),
        ("turn right please", "turn_right"),
        ("turn around", "turn_around"),
        ("stop", "stop"),
        ("look up", "head_up"),
        ("lift up", "lift_up"),
        ("dance for me", "dance"),
        ("hello", "greet"),
        ("what's your name", "name"),
        ("good night", "sleep"),
        ("tell me a joke", "joke"),
    ],
)
def test_english_commands(text, intent):
    assert first(text, "en").intent == intent


def test_distance_and_angle_extraction():
    assert first("ileri 30 santim git").actions == [intents.move(300)]
    assert first("otuz beş santim ileri").actions == [intents.move(350)]
    assert first("bir metre ileri git").actions == [intents.move(1000)]
    assert first("sola 45 derece dön").actions == [intents.turn(45)]
    assert first("sağa doksan derece dön").actions == [intents.turn(-90)]
    assert first("go back twenty centimeters", "en").actions == [intents.move(-200)]


def test_chained_commands():
    results = parse("ileri git sonra sola dön ve sonra dans et", ctx())
    assert [r.intent for r in results] == ["forward", "turn_left", "dance"]


def test_unknown_returns_empty():
    assert parse("mavi gökyüzü", ctx()) == []
    assert parse("", ctx()) == []


def test_wake_word_handling():
    words = ["cozmo", "kozmo"]
    assert intents.contains_wake_word("Kozmo, ileri git", words)
    assert intents.contains_wake_word("hey Cozmo", words)
    assert intents.contains_wake_word("kozmoyu çağır", words)  # Turkish suffix tolerated
    assert not intents.contains_wake_word("ileri git", words)
    assert intents.strip_wake_word("Kozmo, ileri git!", words) == "ileri git"


def test_replies_use_context():
    c = ctx()
    c.battery_pct = 42
    assert "42" in parse("pilin ne durumda", c)[0].reply
    assert "Cozmo" in parse("adın ne", c)[0].reply
