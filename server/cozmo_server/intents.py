"""Rule based intent parser for Turkish and English voice commands.

Text from the speech recogniser is normalised (lower case, Turkish
characters folded to ASCII) and matched against an ordered list of rules.
Each rule produces robot actions plus a spoken reply. Several commands can be
chained with "sonra" / "then": "ileri git sonra sola dön".
"""

from __future__ import annotations

import random
import re
from collections.abc import Callable
from dataclasses import dataclass, field
from datetime import datetime
from typing import Any

# --------------------------------------------------------------------------
# Normalisation
# --------------------------------------------------------------------------
_TR_FOLD = str.maketrans(
    {"ç": "c", "ğ": "g", "ı": "i", "ö": "o", "ş": "s", "ü": "u", "â": "a", "î": "i", "û": "u"}
)


def normalize(text: str) -> str:
    """Lower-case with Turkish rules, fold diacritics, strip punctuation."""
    text = text.replace("I", "ı").replace("İ", "i").lower().translate(_TR_FOLD)
    text = re.sub(r"[^a-z0-9\s]", " ", text)
    return re.sub(r"\s+", " ", text).strip()


def contains_wake_word(text: str, wake_words: list[str]) -> bool:
    tokens = normalize(text).split()
    words = {normalize(w) for w in wake_words}
    return any(t in words or t.rstrip("u").rstrip("y") in words for t in tokens)


def strip_wake_word(text: str, wake_words: list[str]) -> str:
    words = {normalize(w) for w in wake_words}
    kept = [t for t in normalize(text).split() if t not in words and t.rstrip("u").rstrip("y") not in words]
    return " ".join(kept)


# --------------------------------------------------------------------------
# Numbers ("otuz bes santim", "twenty degrees", "45")
# --------------------------------------------------------------------------
_NUMBER_WORDS = {
    # Turkish
    "sifir": 0, "bir": 1, "iki": 2, "uc": 3, "dort": 4, "bes": 5, "alti": 6, "yedi": 7,
    "sekiz": 8, "dokuz": 9, "on": 10, "yirmi": 20, "otuz": 30, "kirk": 40, "elli": 50,
    "altmis": 60, "yetmis": 70, "seksen": 80, "doksan": 90, "yuz": 100, "yarim": 0.5,
    # English
    "zero": 0, "one": 1, "two": 2, "three": 3, "four": 4, "five": 5, "six": 6, "seven": 7,
    "eight": 8, "nine": 9, "ten": 10, "eleven": 11, "twelve": 12, "fifteen": 15, "twenty": 20,
    "thirty": 30, "forty": 40, "fifty": 50, "sixty": 60, "seventy": 70, "eighty": 80,
    "ninety": 90, "hundred": 100, "half": 0.5,
}


def extract_number(text: str) -> float | None:
    """First number in `text`, digits or spelled out (TR/EN)."""
    m = re.search(r"\d+(?:[.,]\d+)?", text)
    if m:
        return float(m.group().replace(",", "."))
    total: float | None = None
    for token in text.split():
        value = _NUMBER_WORDS.get(token)
        if value is None:
            if total is not None:
                break
            continue
        if value == 100:
            total = (total or 1) * 100
        else:
            total = (total or 0) + value
    return total


def extract_distance_mm(text: str, default_mm: float) -> float:
    n = extract_number(text)
    if n is None:
        return default_mm
    if re.search(r"\b(metre|metre\w*|meter\w*|m)\b", text):
        return n * 1000
    if re.search(r"\b(mm|milimetre\w*|millimet\w*)\b", text):
        return n
    return n * 10  # centimetres are the natural unit for a small robot


def extract_angle(text: str, default_deg: float) -> float:
    n = extract_number(text)
    if n is None or n == 0:
        return default_deg
    return min(n, 720)


# --------------------------------------------------------------------------
# Results
# --------------------------------------------------------------------------
@dataclass
class Response:
    intent: str
    actions: list[dict[str, Any]] = field(default_factory=list)
    reply: str = ""


def move(mm: float, speed: float = 120) -> dict[str, Any]:
    return {"cmd": "move", "distance_mm": round(mm), "speed": speed}


def turn(deg: float, dps: float = 150) -> dict[str, Any]:
    return {"cmd": "turn", "angle_deg": round(deg), "speed_dps": dps}


def anim(name: str) -> dict[str, Any]:
    return {"cmd": "anim", "name": name}


def head(deg: float, ms: int = 400) -> dict[str, Any]:
    return {"cmd": "head", "angle_deg": deg, "duration_ms": ms}


def lift(mm: float, ms: int = 500) -> dict[str, Any]:
    return {"cmd": "lift", "height_mm": mm, "duration_ms": ms}


def face(expression: str, ms: int = 3000) -> dict[str, Any]:
    return {"cmd": "face", "expression": expression, "duration_ms": ms}


# --------------------------------------------------------------------------
# Replies
# --------------------------------------------------------------------------
REPLIES: dict[str, dict[str, list[str]]] = {
    "tr": {
        "ok": ["Tamam!", "Hemen!", "Peki!", ""],
        "stop": ["Durdum.", "Tamam, duruyorum."],
        "greet": ["Merhaba!", "Selam!", "Merhaba, seni gördüğüme çok sevindim!"],
        "how_are_you": ["Harikayım, teşekkürler!", "Çok iyiyim! Sen nasılsın?", "Enerjim yerinde!"],
        "how_are_you_low": ["Biraz yorgunum, pilim azaldı."],
        "name": ["Benim adım {name}!", "Ben {name}, küçük bir robotum."],
        "battery": ["Pilim yüzde {pct}.", "Şarjım yüzde {pct} civarında."],
        "battery_unknown": ["Pilimi şu an ölçemiyorum."],
        "time": ["Saat {time}."],
        "praise": ["Teşekkürler!", "Çok tatlısın!", "Yaşasın!"],
        "scold": ["Özür dilerim...", "Üzüldüm..."],
        "angry": ["Grrr!"],
        "scared": ["Ay! Korktum!"],
        "sleep": ["İyi geceler.", "Biraz uyuyayım."],
        "wake": ["Uyandım!", "Günaydın!"],
        "dance": ["Haydi dans!", ""],
        "celebrate": ["Yaşasın!", "Harika!"],
        "volume_up": ["Sesimi açtım."],
        "volume_down": ["Sesimi kıstım."],
        "yes": ["Evet!"],
        "no": ["Hayır!"],
        "unknown": ["Anlamadım.", "Hımm, bunu bilmiyorum.", "Tekrar söyler misin?"],
        "jokes": [
            "Robot neden okula gitmiş? Biraz daha akıllanmak için!",
            "Bilgisayar neden üşümüş? Çünkü pencereleri açık kalmış!",
            "Pilim bitince ne olur biliyor musun? Şarj-ı kalmaz!",
        ],
    },
    "en": {
        "ok": ["Okay!", "On it!", "Sure!", ""],
        "stop": ["Stopped.", "Okay, stopping."],
        "greet": ["Hello!", "Hi there!", "Hey, great to see you!"],
        "how_are_you": ["I feel great, thanks!", "Super! How are you?", "Fully charged with happiness!"],
        "how_are_you_low": ["A bit tired, my battery is low."],
        "name": ["My name is {name}!", "I'm {name}, a little robot."],
        "battery": ["My battery is at {pct} percent."],
        "battery_unknown": ["I can't measure my battery right now."],
        "time": ["It's {time}."],
        "praise": ["Thank you!", "You're so nice!", "Yay!"],
        "scold": ["Sorry...", "That makes me sad..."],
        "angry": ["Grrr!"],
        "scared": ["Eek! You scared me!"],
        "sleep": ["Good night.", "Time for a nap."],
        "wake": ["I'm awake!", "Good morning!"],
        "dance": ["Let's dance!", ""],
        "celebrate": ["Hooray!", "Awesome!"],
        "volume_up": ["Louder now."],
        "volume_down": ["Quieter now."],
        "yes": ["Yes!"],
        "no": ["No!"],
        "unknown": ["I didn't get that.", "Hmm, I don't know that one.", "Could you say it again?"],
        "jokes": [
            "Why did the robot go to school? To get a little smarter!",
            "Why was the computer cold? It left its Windows open!",
            "What do robots eat for a snack? Micro-chips!",
        ],
    },
}


@dataclass
class Context:
    """What the parser may know about the robot when building replies."""

    language: str = "tr"
    name: str = "Cozmo"
    battery_pct: int | None = None
    volume: float = 0.7
    rng: random.Random = field(default_factory=random.Random)

    def say(self, key: str, **kwargs: Any) -> str:
        options = REPLIES[self.language].get(key) or REPLIES["en"][key]
        return self.rng.choice(options).format(name=self.name, **kwargs)


Handler = Callable[[str, Context], Response]


@dataclass
class Rule:
    name: str
    pattern: re.Pattern[str]
    handler: Handler


def _rule(name: str, pattern: str) -> Callable[[Handler], Handler]:
    def deco(fn: Handler) -> Handler:
        RULES.append(Rule(name, re.compile(pattern), fn))
        return fn

    return deco


RULES: list[Rule] = []

# ----- Ordered from most to least specific ----------------------------------


@_rule("stop", r"\b(dur|durdur|stop|halt|bekle|freeze)\b")
def _stop(text: str, ctx: Context) -> Response:
    return Response("stop", [{"cmd": "stop"}], ctx.say("stop"))


@_rule("spin", r"(kendi etrafinda|spin|360|tur at)")
def _spin(text: str, ctx: Context) -> Response:
    return Response("spin", [turn(360, 300)], ctx.say("ok"))


@_rule("turn_around", r"(arkan\w* don|geri don|turn around|u turn)")
def _turn_around(text: str, ctx: Context) -> Response:
    return Response("turn_around", [turn(180)], ctx.say("ok"))


@_rule("head_up", r"(\b(kafa\w*|bas\w*)\b.*\b(kaldir|yukari)\w*|yukari bak|look up|head up)")
def _head_up(text: str, ctx: Context) -> Response:
    return Response("head_up", [head(40)], "")


@_rule("head_down", r"(\b(kafa\w*|bas\w*)\b.*\b(indir|asagi)\w*|asagi bak|look down|head down)")
def _head_down(text: str, ctx: Context) -> Response:
    return Response("head_down", [head(-20)], "")


@_rule("lift_up", r"(\bkol\w*\b.*\b(kaldir|yukari)\w*|\blift( it)? up\b|\braise\b|\bkaldir\w*)")
def _lift_up(text: str, ctx: Context) -> Response:
    return Response("lift_up", [anim("lift_up")], "")


@_rule("lift_down", r"(\bkol\w*\b.*\b(indir|asagi)\w*|\blift( it)? down\b|\blower\b|\bindir\w*)")
def _lift_down(text: str, ctx: Context) -> Response:
    return Response("lift_down", [anim("lift_down")], "")


@_rule("turn_left", r"\b(sola|soluna|sol|left)\b")
def _turn_left(text: str, ctx: Context) -> Response:
    return Response("turn_left", [turn(extract_angle(text, 90))], ctx.say("ok"))


@_rule("turn_right", r"\b(saga|sagina|sag|right)\b")
def _turn_right(text: str, ctx: Context) -> Response:
    return Response("turn_right", [turn(-extract_angle(text, 90))], ctx.say("ok"))


@_rule("backward", r"\b(geri|geriye|backward\w*|back up|reverse|go back)\b")
def _backward(text: str, ctx: Context) -> Response:
    return Response("backward", [move(-extract_distance_mm(text, 100))], ctx.say("ok"))


@_rule("forward", r"\b(ileri|ileriye|one|duz git|forward|ahead|go straight|drive)\b")
def _forward(text: str, ctx: Context) -> Response:
    return Response("forward", [move(extract_distance_mm(text, 150))], ctx.say("ok"))


@_rule("come_here", r"\b(buraya gel|yanima gel|gel|come here|come)\b")
def _come_here(text: str, ctx: Context) -> Response:
    return Response("come_here", [anim("happy"), move(200, 150)], ctx.say("ok"))


@_rule("dance", r"\b(dans\w*|oyna|dance|boogie)\b")
def _dance(text: str, ctx: Context) -> Response:
    return Response("dance", [anim("dance")], ctx.say("dance"))


@_rule("celebrate", r"(kutla\w*|yasasin|tebrik\w*|celebrate|hooray|kazandik|we won)")
def _celebrate(text: str, ctx: Context) -> Response:
    return Response("celebrate", [anim("celebrate")], ctx.say("celebrate"))


@_rule(
    "praise",
    r"(aferin|bravo|harika|super|iyi cocuk|seni seviyorum|tatli\w*|good (boy|job|robot)"
    r"|well done|i love you|awesome|cute)",
)
def _praise(text: str, ctx: Context) -> Response:
    return Response("praise", [anim("happy")], ctx.say("praise"))


@_rule("scold", r"\b(kotu|yaramaz|sus|bad (boy|robot)|naughty|shut up)\b")
def _scold(text: str, ctx: Context) -> Response:
    return Response("scold", [anim("sad")], ctx.say("scold"))


@_rule("angry", r"(sinirlen\w*|kizgin|\bkiz\b|angry|get mad)")
def _angry(text: str, ctx: Context) -> Response:
    return Response("angry", [anim("angry")], ctx.say("angry"))


@_rule("scared", r"(korkut\w*|kork\w*|scare\w*|\bboo+\b)")
def _scared(text: str, ctx: Context) -> Response:
    return Response("scared", [anim("scared")], ctx.say("scared"))


@_rule("wake", r"(uyan\w*|wake up|gunaydin|good morning)")
def _wake(text: str, ctx: Context) -> Response:
    return Response("wake", [anim("wake")], ctx.say("wake"))


@_rule("sleep", r"(\buyu\w*|uyku|iyi geceler|good night|go to sleep|\bsleep\b)")
def _sleep(text: str, ctx: Context) -> Response:
    return Response("sleep", [{"cmd": "sleep"}], ctx.say("sleep"))


@_rule("look_around", r"(etrafin\w* bak|etrafa bak|look around|kesfet|explore)")
def _look_around(text: str, ctx: Context) -> Response:
    return Response("look_around", [anim("curious")], "")


@_rule("how_are_you", r"(nasilsin|naber|ne haber|how are you|how do you feel)")
def _how_are_you(text: str, ctx: Context) -> Response:
    if ctx.battery_pct is not None and ctx.battery_pct < 20:
        return Response("how_are_you", [anim("sad")], ctx.say("how_are_you_low"))
    return Response("how_are_you", [anim("happy")], ctx.say("how_are_you"))


@_rule("name", r"(adin ne|ismin ne|kimsin|your name|who are you)")
def _name(text: str, ctx: Context) -> Response:
    return Response("name", [anim("greet")], ctx.say("name"))


@_rule("greet", r"\b(merhaba|selam\w*|hey|hello|hi|hosgeldin|hos geldin)\b")
def _greet(text: str, ctx: Context) -> Response:
    return Response("greet", [anim("greet")], ctx.say("greet"))


@_rule("battery", r"\b(pil\w*|sarj\w*|batarya\w*|battery|charge)\b")
def _battery(text: str, ctx: Context) -> Response:
    if ctx.battery_pct is None:
        return Response("battery", [], ctx.say("battery_unknown"))
    return Response("battery", [], ctx.say("battery", pct=ctx.battery_pct))


@_rule("time", r"(saat kac|what time)")
def _time(text: str, ctx: Context) -> Response:
    return Response("time", [], ctx.say("time", time=datetime.now().strftime("%H:%M")))


@_rule("joke", r"(saka|fikra|komik|joke|funny)")
def _joke(text: str, ctx: Context) -> Response:
    return Response("joke", [anim("happy")], ctx.say("jokes"))


@_rule("volume_up", r"(sesini ac|sesini yukselt|daha yuksek|louder|volume up)")
def _volume_up(text: str, ctx: Context) -> Response:
    ctx.volume = min(1.0, ctx.volume + 0.2)
    return Response("volume_up", [{"cmd": "volume", "value": round(ctx.volume, 2)}], ctx.say("volume_up"))


@_rule("volume_down", r"(sesini kis|sessiz ol|daha sessiz|quieter|volume down)")
def _volume_down(text: str, ctx: Context) -> Response:
    ctx.volume = max(0.1, ctx.volume - 0.2)
    return Response("volume_down", [{"cmd": "volume", "value": round(ctx.volume, 2)}], ctx.say("volume_down"))


@_rule("yes", r"(evet de|say yes|\bevet\b|\byes\b)")
def _yes(text: str, ctx: Context) -> Response:
    return Response("yes", [anim("nod_yes")], ctx.say("yes"))


@_rule("no", r"(hayir de|say no|\bhayir\b|\bno\b)")
def _no(text: str, ctx: Context) -> Response:
    return Response("no", [anim("shake_no")], ctx.say("no"))


# --------------------------------------------------------------------------
_SPLIT = re.compile(r"\b(?:ve sonra|sonra|ardindan|and then|then)\b")


def parse_one(text: str, ctx: Context) -> Response | None:
    for rule in RULES:
        if rule.pattern.search(text):
            return rule.handler(text, ctx)
    return None


def parse(text: str, ctx: Context) -> list[Response]:
    """Parse (possibly chained) commands. Empty list = nothing understood."""
    norm = normalize(text)
    if not norm:
        return []
    results = []
    for part in _SPLIT.split(norm):
        part = part.strip()
        if not part:
            continue
        result = parse_one(part, ctx)
        if result is not None:
            results.append(result)
    return results


def unknown(ctx: Context) -> Response:
    return Response("unknown", [anim("shake_no")], ctx.say("unknown"))
