"""Synthesize every sound effect and both music loops into assets/audio.

Usage: python tools/make_audio.py
All audio is generated from scratch (no samples), 22.05 kHz mono. Sound effects are 16-bit
WAV (played from memory, no decoding); the two music loops are Ogg Vorbis to keep the
package small. Requires numpy and soundfile (pip install soundfile).
"""
import wave
from pathlib import Path

import numpy as np
import soundfile

SR = 22050
root = Path(__file__).resolve().parents[1]
out = root / "assets" / "audio"
out.mkdir(parents=True, exist_ok=True)
rng = np.random.default_rng(1966)


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
    d = 0.5
    f = sweep(500, 1500, d, 0.6) * (1 + 0.02 * np.sin(2 * np.pi * 30 * t_axis(d)))
    beam = 0.6 * osc("saw", phase(f)) + 0.4 * osc("square", phase(f * 1.005))
    beam = lowpass(beam, 3500) * env(len(beam), 0.01, 0.25) + 0.15 * highpass(noise(d), 3000) * decay(len(beam), 6)
    save("beam", beam, 0.7)

    d = 0.38
    t = t_axis(d)
    whoosh = lowpass(noise(d), 1800 + 1400 * np.sin(2 * np.pi * 9 * t) ** 2) * np.sin(np.pi * t / d)
    whoosh += 0.3 * osc("sine", phase(sweep(320, 620, d))) * np.sin(np.pi * t / d)
    save("slugger", whoosh, 0.7)

    charge = osc("square", phase(sweep(200, 900, 0.22, 2))) * env(int(0.22 * SR), 0.01, 0.02) * 0.35
    zap = osc("saw", phase(sweep(1600, 300, 0.4, 0.5))) * decay(int(0.4 * SR), 7)
    zap = lowpass(zap, 4000) + 0.4 * lowpass(noise(0.4), 2500) * decay(int(0.4 * SR), 9)
    save("snipe", np.concatenate([charge, zap]), 0.8)

    d = 0.7
    chime = mix(*[osc("sine", phase(np.full(int(d * SR), f))) * decay(int(d * SR), 6 + i * 2) / (i + 1)
                  for i, f in enumerate([880, 1320, 1760, 2640])])
    save("pulse", echo(chime, 0.09, 0.3, 3), 0.6)

    d = 0.4
    fire = lowpass(noise(d), sweep(2500, 400, d)) * decay(int(d * SR), 6)
    fire += 0.5 * osc("sine", phase(sweep(180, 70, d))) * decay(int(d * SR), 8)
    save("fire", fire, 0.8)

    d = 0.16
    zero = osc("square", phase(sweep(1300, 500, d))) * decay(int(d * SR), 18)
    save("zero", lowpass(zero, 5000), 0.6)

    d = 0.07
    save("hit", highpass(noise(d), 1500) * decay(int(d * SR), 60) + osc("sine", phase(np.full(int(d * SR), 900))) * decay(int(d * SR), 50), 0.6)

    d = 0.9
    n = int(d * SR)
    boom = lowpass(noise(d), sweep(3000, 150, d, 0.5)) * decay(n, 4.5)
    boom += 0.8 * osc("sine", phase(sweep(90, 35, d))) * decay(n, 5)
    save("explode", boom, 0.85)

    a = osc("square", phase(np.full(int(0.07 * SR), note(83)))) * decay(int(0.07 * SR), 20)
    b = osc("square", phase(np.full(int(0.2 * SR), note(88)))) * decay(int(0.2 * SR), 14)
    save("coin", lowpass(np.concatenate([a, b]), 6000), 0.5)

    notes = [72, 76, 79, 84]
    parts = [osc("tri", phase(np.full(int(0.09 * SR), note(m)))) * decay(int(0.09 * SR), 10) for m in notes]
    shimmer = highpass(noise(0.3), 5000) * decay(int(0.3 * SR), 10) * 0.2
    save("build", mix(np.concatenate(parts), np.concatenate([np.zeros(int(0.2 * SR)), shimmer])), 0.7)

    notes = [72, 76, 79, 84, 88]
    parts = [0.6 * osc("square", phase(np.full(int(0.08 * SR), note(m)))) * decay(int(0.08 * SR), 8) for m in notes]
    tail = osc("tri", phase(np.full(int(0.4 * SR), note(91)))) * decay(int(0.4 * SR), 6)
    save("upgrade", echo(lowpass(np.concatenate(parts + [tail]), 6000), 0.08, 0.25, 2), 0.6)

    parts = [osc("tri", phase(np.full(int(0.08 * SR), note(m)))) * decay(int(0.08 * SR), 12) for m in [79, 74, 70]]
    save("sell", np.concatenate(parts), 0.6)

    d = 1.1
    t = t_axis(d)
    horn = mix(*[osc("saw", phase(note(m) * (1 + 0.006 * np.sin(2 * np.pi * 5 * t)))) for m in (62, 69, 74)])
    horn = lowpass(horn, 1800) * env(len(horn), 0.05, 0.4)
    save("wave", horn, 0.7)

    d = 1.8
    t = t_axis(d)
    siren_f = np.where((t * 2.5) % 1 < 0.5, 520, 390)
    siren = lowpass(osc("square", phase(siren_f)), 2200) * 0.5
    drone = osc("saw", phase(np.full(len(t), 55))) * 0.6
    save("boss", (siren + lowpass(drone, 400)) * env(len(t), 0.05, 0.4), 0.8)

    d = 0.6
    n = int(d * SR)
    leak = osc("sine", phase(sweep(220, 60, d))) * decay(n, 5) + 0.4 * lowpass(noise(d), 400) * decay(n, 8)
    save("leak", leak, 0.85)

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

    save("victory", echo(fanfare([(72, 0.5), (76, 0.5), (79, 0.5), (84, 1.5), (None, 0.25), (81, 0.5), (84, 0.5),
                                  (88, 2.5)]), 0.13, 0.25, 3), 0.75)
    save("defeat", echo(fanfare([(69, 1), (68, 1), (67, 1), (62, 3)], "tri", 110), 0.15, 0.3, 3), 0.75)

    d = 0.04
    save("click", osc("sine", phase(np.full(int(d * SR), 1400))) * decay(int(d * SR), 80), 0.4)

    d = 1.6
    n = int(d * SR)
    t = t_axis(d)
    rise = osc("saw", phase(sweep(150, 1200, d, 1.5))) * np.minimum(1, t / 0.9)
    rise = lowpass(rise, sweep(500, 6000, d))
    blast_start = int(0.85 * SR)
    blast = np.zeros(n)
    bl = n - blast_start
    blast[blast_start:] = (lowpass(noise_n(bl), 4000) * 0.8 + osc("saw", phase(np.full(bl, 110))) * 0.6) * decay(bl, 3)
    save("skill", echo(rise * env(n, 0.01, 0.75) + blast, 0.12, 0.3, 3), 0.85)

    d = 1.8
    t = t_axis(d)
    jet = lowpass(noise(d), 600 + 3000 * np.exp(-((t - 0.8) ** 2) / 0.08)) * np.exp(-((t - 0.8) ** 2) / 0.25)
    jet += 0.25 * osc("saw", phase(sweep(420, 260, d))) * np.exp(-((t - 0.8) ** 2) / 0.2)
    save("jet", jet, 0.8)

    d = 0.2
    save("error", lowpass(osc("square", phase(np.full(int(d * SR), 110))), 900) * env(int(d * SR), 0.005, 0.05), 0.5)

    a = osc("square", phase(sweep(600, 900, 0.08))) * decay(int(0.08 * SR), 20)
    save("split", lowpass(np.concatenate([a, np.zeros(int(0.04 * SR)), a, np.zeros(int(0.04 * SR)), a]), 3000), 0.5)

    d = 0.6
    t = t_axis(d)
    hum = osc("sine", phase(np.full(len(t), 330) * (1 + 0.03 * np.sin(2 * np.pi * 12 * t)))) + 0.5 * osc("sine", phase(np.full(len(t), 495)))
    save("shield", hum * np.sin(np.pi * t / d), 0.6)

    d = 0.45
    t = t_axis(d)
    tele = osc("sine", phase(800 + 600 * np.sin(2 * np.pi * 7 * t))) * np.sin(np.pi * t / d)
    save("teleport", tele, 0.6)

    d = 2.6
    n = int(d * SR)
    t = t_axis(d)
    choir = np.zeros(n)
    for i, midi in enumerate([60, 64, 67, 72, 76, 79, 84]):
        start = int(i * 0.05 * SR)
        nn = n - start
        tone = osc("sine", phase(note(midi) * (1 + 0.01 * np.sin(2 * np.pi * (2 + i) * t[:nn]))))
        tone += 0.4 * osc("tri", phase(np.full(nn, note(midi + 12))))
        choir[start:] += tone * np.minimum(1, np.arange(nn) / (0.4 * SR)) * 0.2
    blast_at = int(1.25 * SR)
    bl = n - blast_at
    boom = lowpass(noise_n(bl), np.linspace(4200, 70, bl)) * decay(bl, 2.0)
    boom += 0.85 * osc("sine", phase(np.linspace(78, 34, bl))) * decay(bl, 2.4)
    boom += 0.2 * highpass(noise_n(bl), 3500) * decay(bl, 3.5)
    choir[blast_at:] += boom
    for i, midi in enumerate([84, 88, 91, 96, 100]):
        s = blast_at + int(0.05 * SR) + int(i * 0.07 * SR)
        ln = min(int(0.5 * SR), n - s)
        if ln > 0:
            choir[s:s + ln] += 0.16 * osc("sine", phase(np.full(ln, note(midi)))) * decay(ln, 5)
    save("fusion", echo(choir * env(n, 0.02, 0.8), 0.14, 0.32, 4), 0.92)


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
    D, F, G, A, Bb, C, E = 62, 65, 67, 69, 70, 72, 64
    battle_chords = [(D, "m"), (Bb - 12, ""), (C - 12, ""), (A - 12, "m"), (D, "m"), (Bb - 12, ""), (F, ""), (C - 12, "")]
    R = None
    phrase_a = [(74, 1), (77, 0.5), (81, 1.5), (79, 1), (77, 0.5), (76, 0.5), (77, 1), (74, 2), (R, 1),
                (72, 1), (74, 0.5), (76, 1.5), (77, 1), (76, 1), (72, 1), (74, 3), (R, 1)]
    phrase_b = [(81, 1.5), (79, 0.5), (77, 1), (79, 1), (81, 1.5), (82, 0.5), (84, 2), (R, 1), (82, 1),
                (81, 1), (79, 1), (77, 1), (76, 2), (77, 1), (74, 3), (R, 1)]
    melody = [(R, 16)] + (phrase_a + phrase_b) * 3
    save("bgm_battle", render_song(138, 32, battle_chords, melody, "battle"), 0.8, music=True)

    menu_chords = [(F, ""), (C - 12, ""), (D, "m"), (Bb - 12, ""), (F, ""), (C - 12, ""), (Bb - 12, ""), (C - 12, "")]
    hymn = [(R, 8), (77, 2), (79, 1), (81, 1), (84, 3), (81, 1), (82, 2), (81, 1), (79, 1), (77, 4),
            (74, 2), (76, 1), (77, 1), (79, 3), (77, 1), (76, 2), (72, 2), (74, 4),
            (77, 2), (79, 1), (81, 1), (84, 3), (86, 1), (84, 2), (82, 1), (81, 1), (79, 4),
            (82, 2), (81, 1), (79, 1), (77, 2), (79, 2), (77, 8)]
    save("bgm_menu", render_song(92, 20, menu_chords, hymn, "menu"), 0.75, music=True)


if __name__ == "__main__":
    make_effects()
    make_music()
    print("wrote", len(list(out.glob("*.wav"))) + len(list(out.glob("*.ogg"))), "files to", out)
