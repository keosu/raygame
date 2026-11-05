"""Synthesize every sound effect and the three music loops into assets/audio.

Usage: python tools/make_audio.py
All audio is generated from scratch (no samples), 22.05 kHz mono. Sound effects are 16-bit
WAV (played from memory, no decoding); the music loops are Ogg Vorbis to keep the package
small. The synth helpers and song renderer follow examples/ultra_guard/tools/make_audio.py.
Requires numpy and soundfile (pip install soundfile).
"""
import wave
from pathlib import Path

import numpy as np
import soundfile

SR = 22050
root = Path(__file__).resolve().parents[1]
out = root / "assets" / "audio"
out.mkdir(parents=True, exist_ok=True)
rng = np.random.default_rng(2020)


def t_axis(dur):
    return np.arange(int(dur * SR)) / SR


def phase(freq):
    """Integrate a per-sample frequency curve into a phase in cycles."""
    return np.cumsum(np.asarray(freq, dtype=float)) / SR


def osc(kind, ph):
    frac = ph % 1.0
    if kind == "sine":
        return np.sin(2 * np.pi * ph)
    if kind == "square":
        return np.where(frac < 0.5, 1.0, -1.0)
    if kind == "pulse":
        return np.where(frac < 0.25, 1.0, -1.0)
    if kind == "saw":
        return 2 * frac - 1
    if kind == "tri":
        return 4 * np.abs(frac - 0.5) - 1
    raise ValueError(kind)


def sweep(f0, f1, dur, curve=1.0):
    t = t_axis(dur) / dur
    return f0 + (f1 - f0) * t**curve


def env(n, attack=0.005, release=0.1, hold=None):
    e = np.ones(n)
    a = max(1, int(attack * SR))
    r = max(1, int(release * SR))
    e[:a] = np.linspace(0, 1, a)
    if hold is None:
        e[-r:] *= np.linspace(1, 0, r)
    return e


def decay(n, rate):
    return np.exp(-np.arange(n) / SR * rate)


def lowpass(x, cutoff):
    """One-pole low-pass; cutoff may be an array for sweeps."""
    cutoff = np.broadcast_to(np.asarray(cutoff, dtype=float), x.shape)
    alpha = 1 - np.exp(-2 * np.pi * cutoff / SR)
    y = np.empty_like(x)
    acc = 0.0
    for i in range(len(x)):
        acc += alpha[i] * (x[i] - acc)
        y[i] = acc
    return y


def highpass(x, cutoff):
    return x - lowpass(x, cutoff)


def noise(dur):
    return rng.uniform(-1, 1, int(dur * SR))


def noise_n(n):
    return rng.uniform(-1, 1, n)


def mix(*parts):
    n = max(len(p) for p in parts)
    y = np.zeros(n)
    for p in parts:
        y[: len(p)] += p
    return y


def echo(x, delay=0.11, feedback=0.35, taps=4):
    d = int(delay * SR)
    y = np.concatenate([x, np.zeros(d * taps)])
    for k in range(1, taps + 1):
        y[d * k : d * k + len(x)] += x * feedback**k
    return y


def save(name, x, gain=0.9, music=False):
    x = np.asarray(x, dtype=float)
    peak = np.max(np.abs(x)) or 1.0
    x = np.clip(x / peak * gain, -1, 1)
    fade = min(len(x), int(0.004 * SR))
    x[-fade:] *= np.linspace(1, 0, fade)
    if music:
        # libsndfile's Vorbis encoder fails on very large single writes, so feed it in blocks.
        with soundfile.SoundFile(str(out / f"{name}.ogg"), "w", SR, 1, format="OGG", subtype="VORBIS") as f:
            for start in range(0, len(x), 8192):
                f.write(x[start:start + 8192].astype(np.float32))
        return
    data = (x * 32767).astype("<i2").tobytes()
    with wave.open(str(out / f"{name}.wav"), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(data)


def note(n):
    return 440.0 * 2 ** ((n - 69) / 12)



# ---------------------------------------------------------------- effects

def make_effects():
    # Melee: a short noise crack over a falling body thump; the kick is heavier and longer.
    d = 0.16
    n = int(d * SR)
    punch = highpass(noise(d), 900) * decay(n, 45) + 0.9 * osc("sine", phase(sweep(180, 60, d))) * decay(n, 22)
    save("punch", lowpass(punch, 6000), 0.8)
    d = 0.3
    n = int(d * SR)
    kick = highpass(noise(d), 600) * decay(n, 30) + osc("sine", phase(sweep(140, 40, d))) * decay(n, 12)
    kick += 0.3 * lowpass(noise(d), 300) * decay(n, 10)
    save("kick", kick, 0.85)
    d = 0.22
    t = t_axis(d)
    swing = lowpass(noise(d), 900 + 2600 * np.sin(np.pi * t / d)) * np.sin(np.pi * t / d) ** 2
    save("swing", swing, 0.5)
    d = 0.25
    n = int(d * SR)
    clank = sum(osc("square", phase(np.full(n, f))) * decay(n, 18 + i * 6) / (i + 1) for i, f in enumerate([620, 931, 1410]))
    save("guard", lowpass(clank + 0.4 * highpass(noise(d), 3000) * decay(n, 40), 5000), 0.55)
    d = 0.9
    n = int(d * SR)
    ting = sum(osc("sine", phase(np.full(n, f))) * decay(n, 4 + i * 3) / (i + 1) for i, f in enumerate([1568, 2349, 3136, 4699]))
    save("parry", echo(ting + 0.5 * highpass(noise(d), 4000) * decay(n, 50), 0.07, 0.3, 3), 0.65)
    d = 0.3
    t = t_axis(d)
    dash = lowpass(noise(d), sweep(5000, 800, d)) * np.sin(np.pi * t / d) + 0.2 * osc("sine", phase(sweep(700, 300, d))) * decay(int(d * SR), 8)
    save("dash", dash, 0.55)
    d = 0.25
    save("jump", lowpass(noise(d), sweep(600, 2600, d)) * np.sin(np.pi * t_axis(d) / d) * 0.8
         + 0.3 * osc("sine", phase(sweep(200, 420, d))) * decay(int(d * SR), 9), 0.5)
    d = 0.35
    n = int(d * SR)
    save("land", osc("sine", phase(sweep(90, 40, d))) * decay(n, 12) + 0.5 * lowpass(noise(d), 500) * decay(n, 14), 0.7)
    d = 1.2
    n = int(d * SR)
    stomp = osc("sine", phase(sweep(70, 28, d))) * decay(n, 3.5) + 0.8 * lowpass(noise(d), sweep(1500, 120, d)) * decay(n, 4)
    save("stomp", stomp, 0.9)
    d = 0.6
    n = int(d * SR)
    crash = lowpass(noise(d), 3000) * decay(n, 6) + 0.6 * osc("sine", phase(sweep(110, 40, d))) * decay(n, 6)
    for i in range(6):  # falling debris
        s = int((0.12 + i * 0.07) * SR)
        ln = int(0.08 * SR)
        crash[s:s + ln] += 0.4 * highpass(noise_n(ln), 2000) * decay(ln, 40)
    save("crash", crash, 0.85)
    d = 0.4
    n = int(d * SR)
    save("rock", lowpass(noise(d), 1800) * decay(n, 10) + 0.5 * osc("tri", phase(sweep(160, 80, d))) * decay(n, 12), 0.7)
    d = 0.22
    n = int(d * SR)
    save("hurt", lowpass(noise(d), 1200) * decay(n, 20) + osc("saw", phase(sweep(260, 110, d))) * decay(n, 14) * 0.5, 0.65)

    # Red King: a growl whose pitch rises and falls, through a crude vowel formant.
    d = 1.6
    t = t_axis(d)
    f0 = 95 + 70 * np.sin(np.pi * t / d) ** 1.5 + 6 * np.sin(2 * np.pi * 23 * t)
    voice = osc("saw", phase(f0)) + 0.6 * osc("square", phase(f0 * 1.51))
    voice = lowpass(voice, 700 + 900 * np.sin(np.pi * t / d)) + 0.35 * lowpass(noise(d), 1600)
    save("roar", echo(voice * env(len(t), 0.08, 0.5), 0.13, 0.25, 3), 0.85)

    d = 0.6
    t = t_axis(d)
    charge = osc("saw", phase(sweep(200, 1400, d, 1.6))) * np.minimum(1, t / 0.4)
    save("charge", lowpass(charge, sweep(800, 6000, d)) * env(len(t), 0.01, 0.05) * 0.8
         + 0.25 * highpass(noise(d), 5000) * t / d, 0.6)
    d = 1.5
    t = t_axis(d)
    f = 420 * (1 + 0.04 * np.sin(2 * np.pi * 28 * t))
    beam = 0.5 * osc("saw", phase(f)) + 0.4 * osc("square", phase(f * 1.502)) + 0.3 * osc("sine", phase(f * 0.5))
    beam = lowpass(beam, 4200) + 0.25 * highpass(noise(d), 2500)
    save("beam", echo(beam * env(len(t), 0.02, 0.35), 0.09, 0.2, 2), 0.75)
    d = 2.4
    n = int(d * SR)
    boom = lowpass(noise(d), sweep(5000, 80, d, 0.4)) * decay(n, 2.2)
    boom += 0.9 * osc("sine", phase(sweep(80, 26, d))) * decay(n, 2.5)
    for i in range(4):  # secondary blasts
        s = int((0.25 + i * 0.22) * SR)
        ln = n - s
        boom[s:] += 0.5 * lowpass(noise_n(ln), 2500) * decay(ln, 5 + i)
    save("explode", boom, 0.92)

    # The colour timer's "pi-kon": two alternating tones.
    d = 0.18
    a = osc("square", phase(np.full(int(d * SR), 988))) * env(int(d * SR), 0.005, 0.03)
    b = osc("square", phase(np.full(int(d * SR), 740))) * env(int(d * SR), 0.005, 0.03)
    save("timer", lowpass(np.concatenate([a, b]), 3000), 0.45)
    d = 0.5
    t = t_axis(d)
    save("alert", lowpass(osc("square", phase(np.where((t * 8) % 1 < 0.5, 880, 660))), 2500) * env(len(t), 0.01, 0.1), 0.45)
    d = 2.5
    t = t_axis(d)
    rumble = lowpass(noise(d), 180) * (0.6 + 0.4 * np.sin(2 * np.pi * 3.3 * t)) + 0.5 * osc("sine", phase(np.full(len(t), 38)))
    save("quake", rumble * env(len(t), 0.4, 0.9), 0.85)
    d = 0.035
    save("talk", osc("tri", phase(np.full(int(d * SR), 660))) * decay(int(d * SR), 60), 0.35)
    d = 0.04
    save("click", osc("sine", phase(np.full(int(d * SR), 1400))) * decay(int(d * SR), 80), 0.4)
    parts = [osc("tri", phase(np.full(int(0.09 * SR), note(m)))) * decay(int(0.09 * SR), 9) for m in [76, 81, 88]]
    save("clue", echo(np.concatenate(parts), 0.08, 0.3, 2), 0.55)

    # Transformation: medal chimes, a long riser and the burst at 2.6 s, matching the cut-in.
    d = 4.0
    n = int(d * SR)
    t = t_axis(d)
    out_ = np.zeros(n)
    for i, midi in enumerate([79, 83, 86]):  # three medals
        s = int((0.25 + i * 0.45) * SR)
        ln = int(0.5 * SR)
        out_[s:s + ln] += 0.35 * osc("sine", phase(np.full(ln, note(midi)))) * decay(ln, 6)
        out_[s:s + ln] += 0.15 * osc("tri", phase(np.full(ln, note(midi + 12)))) * decay(ln, 9)
    rs, re = int(1.4 * SR), int(2.6 * SR)
    rl = re - rs
    out_[rs:re] += lowpass(noise_n(rl), np.linspace(400, 8000, rl)) * np.linspace(0, 0.6, rl) ** 1.4
    out_[rs:re] += 0.3 * osc("saw", phase(np.geomspace(140, 1100, rl))) * np.linspace(0, 1, rl) ** 2
    bl = n - re
    out_[re:] += lowpass(noise_n(bl), np.linspace(5000, 90, bl)) * decay(bl, 2.2)
    out_[re:] += 0.8 * osc("sine", phase(np.linspace(90, 40, bl))) * decay(bl, 2.6)
    for i, midi in enumerate([84, 88, 91, 96]):
        s = re + int(0.04 * SR) + int(i * 0.08 * SR)
        ln = min(int(0.7 * SR), n - s)
        out_[s:s + ln] += 0.18 * osc("sine", phase(np.full(ln, note(midi)))) * decay(ln, 4)
    save("transform", echo(out_ * env(n, 0.01, 0.6), 0.14, 0.3, 4), 0.92)

    def fanfare(seq, wave_kind="square", bpm=150):
        beat = 60 / bpm
        parts = []
        for m, beats in seq:
            nn = int(beats * beat * SR)
            if m is None:
                parts.append(np.zeros(nn))
                continue
            t2 = np.arange(nn) / SR
            tone = 0.5 * osc(wave_kind, phase(note(m) * (1 + 0.004 * np.sin(2 * np.pi * 6 * t2))))
            tone += 0.4 * osc("tri", phase(np.full(nn, note(m - 12))))
            parts.append(tone * env(nn, 0.01, 0.06))
        return lowpass(np.concatenate(parts), 5000)

    save("victory", echo(fanfare([(67, 0.5), (72, 0.5), (76, 0.5), (79, 1), (76, 0.5), (79, 0.5), (84, 2.5)]),
                         0.13, 0.25, 3), 0.75)
    save("defeat", echo(fanfare([(69, 1), (68, 1), (67, 1), (62, 3)], "tri", 100), 0.15, 0.3, 3), 0.75)


# ---------------------------------------------------------------- music

def drum_kick(n):
    t = np.arange(n) / SR
    return osc("sine", phase(60 + 90 * np.exp(-t * 30))) * np.exp(-t * 9)


def drum_snare(n):
    t = np.arange(n) / SR
    return (0.7 * highpass(noise_n(n), 1200) + 0.3 * osc("tri", phase(np.full(n, 190)))) * np.exp(-t * 18)


def drum_hat(n):
    t = np.arange(n) / SR
    return highpass(noise_n(n), 6000) * np.exp(-t * 60)


def render_song(bpm, bars, chords, melody, style):
    beat = 60 / bpm
    total = int(bars * 4 * beat * SR)
    y = np.zeros(total + SR)

    def place(signal, start_beat, gain):
        s = int(start_beat * beat * SR)
        if s >= len(y):
            return
        e = min(len(y), s + len(signal))
        y[s:e] += signal[: e - s] * gain

    kick = drum_kick(int(0.35 * SR))
    snare = drum_snare(int(0.25 * SR))
    hat = drum_hat(int(0.06 * SR))
    for bar in range(bars):
        root, quality = chords[bar % len(chords)]
        tones = [root, root + (3 if quality == "m" else 4), root + 7]
        b0 = bar * 4
        # Bass
        step = 0.5 if style == "battle" else 1.0
        k = 0
        while k < 4:
            nn = int(step * beat * SR * 0.95)
            m = root - 24 + (12 if style == "battle" and int(k / step) % 4 == 3 else 0)
            tone = osc("tri", phase(np.full(nn, note(m)))) + 0.3 * osc("square", phase(np.full(nn, note(m))))
            place(lowpass(tone, 900) * env(nn, 0.005, 0.05), b0 + k, 0.35)
            k += step
        # Pad / arpeggio
        if style == "battle":
            for s16 in range(16):
                nn = int(0.25 * beat * SR)
                m = tones[s16 % 3] + 12 + (12 if s16 % 8 >= 6 else 0)
                tone = osc("pulse", phase(np.full(nn, note(m)))) * decay(nn, 14)
                place(lowpass(tone, 3500), b0 + s16 * 0.25, 0.08)
        else:
            nn = int(4 * beat * SR)
            t = np.arange(nn) / SR
            pad = sum(osc("saw", phase(np.full(nn, note(m + 12) * det))) for m in tones for det in (0.997, 1.003))
            place(lowpass(pad, 1200) * env(nn, 0.4, 0.6) * (0.85 + 0.15 * np.sin(2 * np.pi * 0.5 * t)), b0, 0.05)
            for s8 in range(8):
                nn2 = int(0.5 * beat * SR)
                m = tones[[0, 1, 2, 1][s8 % 4]] + 24
                bell = osc("sine", phase(np.full(nn2 * 2, note(m)))) * decay(nn2 * 2, 5)
                place(bell, b0 + s8 * 0.5, 0.07)
        # Drums
        if style == "battle":
            for q in range(4):
                place(kick, b0 + q, 0.55 if q in (0, 2) else 0.0)
                if q in (1, 3):
                    place(snare, b0 + q, 0.32)
                place(hat, b0 + q, 0.12)
                place(hat, b0 + q + 0.5, 0.08)
            if bar % 4 == 3:
                for f in range(4):
                    place(snare, b0 + 3 + f * 0.25, 0.18 + f * 0.05)
        else:
            place(kick, b0, 0.3)
            place(kick, b0 + 2.5, 0.2)
            place(hat, b0 + 1, 0.05)
            place(hat, b0 + 3, 0.05)

    pos = 0.0
    for m, beats in melody:
        if m is not None:
            nn = int(beats * beat * SR)
            t = np.arange(nn) / SR
            vib = 1 + 0.006 * np.sin(2 * np.pi * 5.5 * t) * np.minimum(1, t / 0.2)
            kind = "square" if style == "battle" else "tri"
            tone = 0.6 * osc(kind, phase(note(m) * vib)) + 0.4 * osc("saw", phase(note(m) * vib * 1.003))
            place(lowpass(tone, 3000 if style == "battle" else 2000) * env(nn, 0.01, 0.08), pos, 0.16)
        pos += beats

    y = y[:total]
    y = echo(y, beat * 0.75, 0.18, 2)[:total]
    # Crossfade the tail into the head so the stream loops seamlessly.
    xf = int(0.05 * SR)
    y[:xf] = y[:xf] * np.linspace(0, 1, xf) + y[-xf:] * np.linspace(1, 0, xf)
    return y



def make_music():
    R = None
    # Battle: E minor drive.
    E, G, A, B, C, D = 64, 67, 69, 71, 72, 74
    chords = [(E - 12, "m"), (C - 12, ""), (D - 12, ""), (B - 12, "m"), (E - 12, "m"), (C - 12, ""), (A - 12, "m"), (B - 12, "")]
    a = [(76, 1), (79, 0.5), (83, 1.5), (81, 1), (79, 0.5), (78, 0.5), (79, 1), (76, 2), (R, 1),
         (74, 1), (76, 0.5), (78, 1.5), (79, 1), (78, 1), (74, 1), (76, 3), (R, 1)]
    b = [(83, 1.5), (81, 0.5), (79, 1), (81, 1), (83, 1.5), (84, 0.5), (86, 2), (R, 1), (84, 1),
         (83, 1), (81, 1), (79, 1), (78, 2), (79, 1), (76, 3), (R, 1)]
    save("bgm_battle", render_song(146, 32, chords, [(R, 16)] + (a + b) * 3, "battle"), 0.8, music=True)

    # Exploration: a light, curious walk in G.
    G2, C2, D2, Em = 55, 60, 62, 64
    chords = [(G2, ""), (Em - 12, "m"), (C2, ""), (D2, ""), (G2, ""), (Em - 12, "m"), (C2, ""), (D2, "")]
    walk = [(R, 4), (71, 1), (74, 1), (76, 2), (74, 1), (71, 1), (69, 2), (67, 1), (69, 1), (71, 2), (74, 4),
            (76, 1), (78, 1), (79, 2), (78, 1), (76, 1), (74, 2), (71, 1), (69, 1), (67, 2), (69, 4)]
    save("bgm_explore", render_song(104, 24, chords, walk * 2, "menu"), 0.7, music=True)

    # Title: a hymn in D.
    D3, G3, A3, Bm = 62, 67, 69, 71
    chords = [(D3 - 12, ""), (A3 - 12, ""), (Bm - 12, "m"), (G3 - 12, ""), (D3 - 12, ""), (A3 - 12, ""), (G3 - 12, ""), (A3 - 12, "")]
    hymn = [(R, 8), (74, 2), (76, 1), (78, 1), (81, 3), (78, 1), (79, 2), (78, 1), (76, 1), (74, 4),
            (71, 2), (73, 1), (74, 1), (76, 3), (74, 1), (73, 2), (69, 2), (71, 4),
            (74, 2), (76, 1), (78, 1), (81, 3), (83, 1), (81, 2), (79, 1), (78, 1), (76, 4),
            (79, 2), (78, 1), (76, 1), (74, 2), (76, 2), (74, 8)]
    save("bgm_title", render_song(90, 20, chords, hymn, "menu"), 0.75, music=True)


if __name__ == "__main__":
    make_effects()
    make_music()
    print("wrote", len(list(out.glob("*.wav"))) + len(list(out.glob("*.ogg"))), "files to", out)
