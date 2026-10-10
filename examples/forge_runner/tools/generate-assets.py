"""Generate FORGE's original glow/icon and synthesized soundtrack/effects.

Requires Pillow. Normal builds use committed outputs and need no Python.
"""
from pathlib import Path
import math
import random
import struct
import wave
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / "assets"
(ASSETS / "audio").mkdir(parents=True, exist_ok=True)
(ROOT / "resources").mkdir(parents=True, exist_ok=True)
glow = Image.new("RGBA", (128, 128))
for y in range(128):
    for x in range(128):
        distance = math.hypot(x - 63.5, y - 63.5) / 63.5
        glow.putpixel((x, y), (255, 255, 255, round(255 * max(0, 1 - distance) ** 2)))
glow.save(ASSETS / "glow.png")

icon = Image.new("RGBA", (512, 512), (9, 22, 28, 255))
draw = ImageDraw.Draw(icon)
draw.rounded_rectangle((24, 24, 488, 488), radius=88, fill=(23, 43, 48), outline=(105, 147, 130), width=8)
draw.polygon([(130, 170), (255, 97), (385, 170), (385, 324), (255, 407), (130, 324)], fill=(222, 184, 115))
draw.polygon([(164, 186), (258, 139), (350, 186), (350, 298), (256, 359), (164, 298)], fill=(16, 35, 41))
draw.polygon([(228, 198), (294, 174), (273, 239), (326, 232), (229, 322), (246, 262), (193, 272)], fill=(150, 245, 199))
draw.line((112, 374, 169, 406), fill=(248, 94, 79), width=19)
icon.resize((256, 256), Image.Resampling.LANCZOS).save(ASSETS / "icon.png")
icon.save(ROOT / "resources/forge.ico", sizes=[(s, s) for s in (16, 20, 24, 32, 48, 64, 128, 256)])
(ROOT / "resources/forge.rc").write_text('1 ICON "forge.ico"\n', encoding="utf-8", newline="\n")

RATE = 22050
rng = random.Random(711)


def export(name, seconds, sample):
    with wave.open(str(ASSETS / "audio" / f"{name}.wav"), "wb") as output:
        output.setparams((1, 2, RATE, 0, "NONE", "not compressed"))
        output.writeframes(b"".join(struct.pack("<h", round(max(-1, min(1, sample(i / RATE))) * 24000)) for i in range(round(seconds * RATE))))


def chirp(seconds, start, end, volume=0.5, noise=0):
    def sample(t):
        envelope = math.sin(min(1, t / 0.009) * math.pi / 2) * (1 - t / seconds) ** 2
        phase = 2 * math.pi * (start * t + (end - start) * t * t / (2 * seconds))
        return envelope * (math.sin(phase) * volume + rng.uniform(-1, 1) * noise)
    return sample


export("jump", 0.18, chirp(0.18, 180, 690, 0.35))
export("dash", 0.22, chirp(0.22, 180, 70, 0.18, 0.32))
export("land", 0.13, chirp(0.13, 115, 35, 0.48, 0.13))
export("death", 0.65, chirp(0.65, 360, 40, 0.4, 0.11))
export("ui", 0.07, chirp(0.07, 690, 420, 0.15))


def arpeggio(duration, notes):
    def sample(t):
        index = min(len(notes) - 1, int(t / duration * len(notes)))
        local = t - index * duration / len(notes)
        freq = notes[index]
        return (math.sin(2 * math.pi * freq * local) + math.sin(4 * math.pi * freq * local) * 0.18) * math.exp(-local * 12) * 0.34
    return sample


export("cell", 0.28, arpeggio(0.28, (659.25, 987.77, 1318.51)))
export("checkpoint", 0.7, arpeggio(0.7, (329.63, 493.88, 659.25, 987.77)))
export("finish", 1.6, arpeggio(1.6, (261.63, 329.63, 392, 523.25, 659.25, 783.99)))

# A seamless 32-second industrial ambient loop: soft pulse, analog bass, metallic
# plucks and a restrained four-chord bed. All waveforms are original synthesis.
chords = ((55, 82.407, 110, 130.813), (43.654, 65.406, 87.307, 110),
          (65.406, 98, 130.813, 164.814), (49, 73.416, 98, 123.471))
melody = (329.63, 293.66, 261.63, 220, 261.63, 293.66, 392, 329.63)


def music(t):
    chord = chords[int(t / 8) % 4]
    within = t % 8
    pad_envelope = math.sin(math.pi * within / 8) ** 0.45
    pad = sum(math.sin(2 * math.pi * f * t) * 0.04 for f in chord[1:]) * pad_envelope
    beat = t % 0.5
    bass = math.sin(2 * math.pi * chord[0] * t) * math.exp(-beat * 5) * 0.11
    kick = math.sin(2 * math.pi * (48 * beat + 40 * (1 - math.exp(-beat * 25)) / 25)) * math.exp(-beat * 26) * 0.14
    plucktime = t % 1
    freq = melody[int(t) % len(melody)]
    pluck = (math.sin(2 * math.pi * freq * plucktime) + math.sin(2 * math.pi * freq * 2.002 * plucktime) * 0.2) * math.exp(-plucktime * 7) * 0.045
    hiss = math.sin(2 * math.pi * 6503 * t) * math.sin(2 * math.pi * 3921 * t) * math.exp(-(t % 0.25) * 80) * 0.018
    return pad + bass + kick + pluck + hiss


export("forge", 32, music)
print("FORGE: generated glow, icon, eight cues and 32-second soundtrack")
