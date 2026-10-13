"""Rebuild original short campaign sound effects; Python standard library only."""
from pathlib import Path
import math
import struct
import wave

root = Path(__file__).resolve().parents[1]
notes = {"click": (700, 0.07), "shot": (440, 0.12), "clear": (880, 0.22), "hurt": (110, 0.12), "win": (660, 0.6)}
for name in ("marble_temple", "garden_guard"):
    assets = root / name / "assets"
    assets.mkdir(parents=True, exist_ok=True)
    for effect, (frequency, duration) in notes.items():
        count = int(duration * 22050)
        data = []
        for i in range(count):
            t = i / 22050
            f = frequency * (1 + (t * 2 if effect in ("clear", "win") else -t))
            envelope = math.sin(math.pi * i / count) ** 2
            data.append(struct.pack("<h", int(10000 * envelope * math.sin(2 * math.pi * f * t))))
        with wave.open(str(assets / (effect + ".wav")), "wb") as sound:
            sound.setparams((1, 2, 22050, 0, "NONE", "not compressed"))
            sound.writeframes(b"".join(data))
