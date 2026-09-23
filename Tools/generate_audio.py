"""Generates the original placeholder sound set for The Final Take.

Every sound is synthesised from scratch (oscillators, filtered noise, envelopes),
so the project ships no third-party audio. Output: RawAudio/*.wav (16-bit mono).
Run:  python Tools/generate_audio.py
"""
import os
import wave
import numpy as np

SR = 44100
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "RawAudio")
rng = np.random.default_rng(1234)


def t_axis(dur):
    return np.arange(int(dur * SR)) / SR


def osc(freq, dur, shape="sine"):
    n = int(dur * SR)
    f = np.broadcast_to(np.asarray(freq, dtype=np.float64), (n,)) if np.ndim(freq) == 0 else np.asarray(freq)
    phase = np.cumsum(f) / SR
    p = phase % 1.0
    if shape == "sine":
        return np.sin(2 * np.pi * phase)
    if shape == "square":
        return np.where(p < 0.5, 1.0, -1.0)
    if shape == "saw":
        return 2.0 * p - 1.0
    if shape == "tri":
        return 4.0 * np.abs(p - 0.5) - 1.0
    raise ValueError(shape)


def noise(dur):
    return rng.uniform(-1, 1, int(dur * SR))


def band(x, lo=20.0, hi=20000.0, soft=1.0):
    """Smooth FFT band-pass."""
    X = np.fft.rfft(x)
    f = np.fft.rfftfreq(len(x), 1 / SR)
    g = np.ones_like(f)
    if lo > 0:
        g *= 1 / (1 + (lo / np.maximum(f, 1e-3)) ** (2 * soft))
    g *= 1 / (1 + (f / hi) ** (2 * soft))
    return np.fft.irfft(X * g, len(x))


def env(n, a=0.005, d=0.1, s=0.7, r=0.1, dur=None):
    total = n
    a_n, d_n, r_n = int(a * SR), int(d * SR), int(r * SR)
    s_n = max(0, total - a_n - d_n - r_n)
    e = np.concatenate([
        np.linspace(0, 1, max(a_n, 1)),
        np.linspace(1, s, max(d_n, 1)),
        np.full(s_n, s),
        np.linspace(s, 0, max(r_n, 1)),
    ])
    return e[:total] if len(e) >= total else np.pad(e, (0, total - len(e)))


def expdec(n, tau):
    return np.exp(-np.arange(n) / SR / tau)


def mix(*parts):
    n = max(len(p) for p in parts)
    out = np.zeros(n)
    for p in parts:
        out[: len(p)] += p
    return out


def place(buf, x, at):
    i = int(at * SR)
    end = min(len(buf), i + len(x))
    buf[i:end] += x[: end - i]


def norm(x, peak=0.85):
    m = np.max(np.abs(x)) or 1.0
    return x / m * peak


def loopify(x, fade=0.25):
    """Crossfade the tail into the head so the file loops seamlessly."""
    n = int(fade * SR)
    head, tail = x[:n].copy(), x[-n:].copy()
    w = np.linspace(0, 1, n)
    x = x[:-n].copy()
    x[:n] = head * w + tail * (1 - w)
    return x


def save(name, x, peak=0.85):
    x = norm(x, peak)
    os.makedirs(OUT, exist_ok=True)
    data = (np.clip(x, -1, 1) * 32767).astype(np.int16)
    with wave.open(os.path.join(OUT, name + ".wav"), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(data.tobytes())
    print("wrote", name, f"{len(x)/SR:.2f}s")


# ----------------------------------------------------------------- UI / slate
def clapper():
    n = noise(0.3)
    click = band(n, 1500, 9000) * expdec(len(n), 0.012)
    body = osc(420, 0.3, "tri") * expdec(len(n), 0.03) * 0.6
    wood = band(noise(0.3), 300, 1800) * expdec(len(n), 0.05) * 0.5
    return mix(click * 1.2, body, wood)


def ui_click():
    d = 0.07
    f = np.linspace(1400, 700, int(d * SR))
    return osc(f, d) * expdec(int(d * SR), 0.015)


def ui_confirm():
    a = osc(660, 0.14, "tri") * env(int(0.14 * SR), 0.003, 0.05, 0.5, 0.05)
    b = osc(990, 0.22, "tri") * env(int(0.22 * SR), 0.003, 0.08, 0.5, 0.1)
    buf = np.zeros(int(0.36 * SR))
    place(buf, a, 0)
    place(buf, b, 0.1)
    return buf


def ui_error():
    d = 0.28
    x = osc(140, d, "square") * 0.5 + osc(147, d, "square") * 0.5
    return band(x, 60, 1500) * env(int(d * SR), 0.005, 0.05, 0.6, 0.12)


def stamp():
    d = 0.3
    thump = osc(np.linspace(160, 60, int(d * SR)), d) * expdec(int(d * SR), 0.06)
    paper = band(noise(d), 2000, 8000) * expdec(int(d * SR), 0.02) * 0.4
    return mix(thump, paper)


def rec_beep():
    d = 0.18
    return osc(1320, d) * env(int(d * SR), 0.002, 0.02, 0.8, 0.04)


# ----------------------------------------------------------------- production cues
def siren():
    d = 3.6
    t = t_axis(d)
    f = 720 + 380 * (0.5 - 0.5 * np.cos(2 * np.pi * t / 1.8))
    x = osc(f, d, "saw") * 0.6 + osc(f * 1.005, d, "square") * 0.3
    x = band(x, 250, 4000)
    return x * env(len(t), 0.25, 0.1, 1.0, 0.5)


def storm():
    d = 4.5
    rain = band(noise(d), 800, 7000) * 0.35
    rumble = band(noise(d), 25, 160) * 1.6
    t = t_axis(d)
    rumble *= (0.3 + 0.7 * np.exp(-((t - 1.2) ** 2) / 0.6))
    crack = band(noise(d), 300, 5000) * np.exp(-np.maximum(t - 0.9, 0) / 0.25) * (t > 0.9) * 0.9
    return (rain + rumble + crack) * env(len(t), 0.2, 0.1, 1.0, 1.0)


def monster_sting():
    # Original descending minor stab with timpani, deliberately not a semitone "dun-dun".
    d = 2.4
    buf = np.zeros(int(d * SR))
    chord = [98.0, 116.5, 146.8, 196.0]  # G minor voicing
    for i, f in enumerate(chord):
        v = sum(osc(f * dt, 1.6, "saw") for dt in (0.995, 1.0, 1.006)) / 3
        v = band(v, 60, 2500) * env(len(v), 0.02, 0.4, 0.5, 0.8)
        place(buf, v * 0.5, 0.0)
    timp = osc(np.linspace(90, 55, int(1.2 * SR)), 1.2) * expdec(int(1.2 * SR), 0.35)
    place(buf, timp * 1.3, 0.0)
    low = osc(np.linspace(73.4, 49, int(1.0 * SR)), 1.0, "saw")
    low = band(low, 40, 900) * env(len(low), 0.01, 0.3, 0.6, 0.5)
    place(buf, low * 0.8, 1.1)
    return buf


def projector_loop():
    d = 2.0
    buf = np.zeros(int(d * SR))
    for i in range(int(d * 24)):
        c = band(noise(0.02), 1500, 6000) * expdec(int(0.02 * SR), 0.004)
        place(buf, c * (0.8 if i % 2 else 0.5), i / 24)
    hum = osc(120, d) * 0.12 + osc(240, d) * 0.05
    return loopify(buf + hum + band(noise(d), 200, 1200) * 0.05, 0.1)


def rain_loop():
    d = 4.0
    x = band(noise(d), 900, 8000) * 0.8
    drops = np.zeros(len(x))
    for _ in range(90):
        p = band(noise(0.03), 2000, 7000) * expdec(int(0.03 * SR), 0.005)
        place(drops, p * rng.uniform(0.2, 0.6), rng.uniform(0, d - 0.05))
    return loopify(x + drops, 0.4)


def wind_loop():
    d = 5.0
    t = t_axis(d)
    x = band(noise(d), 150, 1400)
    mod = 0.55 + 0.45 * np.sin(2 * np.pi * t / 2.5)
    return loopify(x * mod, 0.6)


def leak_loop():
    d = 3.0
    x = band(noise(d), 1500, 9000) * 0.6 + band(noise(d), 200, 900) * 0.5
    bubbles = np.zeros(len(x))
    for _ in range(40):
        bd = 0.05
        f = np.linspace(rng.uniform(300, 700), rng.uniform(800, 1400), int(bd * SR))
        place(bubbles, osc(f, bd) * expdec(int(bd * SR), 0.015) * 0.4, rng.uniform(0, d - 0.1))
    return loopify(x + bubbles, 0.3)


def splash():
    d = 0.9
    x = band(noise(d), 400, 6000) * expdec(int(d * SR), 0.18)
    low = band(noise(d), 60, 400) * expdec(int(d * SR), 0.08) * 0.8
    bub = np.zeros(int(d * SR))
    for _ in range(12):
        bd = 0.06
        f = np.linspace(rng.uniform(400, 800), rng.uniform(900, 1600), int(bd * SR))
        place(bub, osc(f, bd) * expdec(int(bd * SR), 0.02) * 0.25, rng.uniform(0.1, 0.7))
    return mix(x, low, bub)


def alarm():
    d = 2.4
    buf = np.zeros(int(d * SR))
    for i in range(6):
        f = 880 if i % 2 == 0 else 660
        b = osc(f, 0.34, "square")
        b = band(b, 300, 3500) * env(len(b), 0.005, 0.02, 0.9, 0.03)
        place(buf, b, i * 0.4)
    return buf


def power_down():
    d = 1.6
    f = np.geomspace(900, 60, int(d * SR))
    x = osc(f, d, "saw") * 0.5 + osc(f * 0.5, d) * 0.5
    x = band(x, 40, 3000) * env(int(d * SR), 0.01, 0.2, 0.8, 0.6)
    thunk = osc(70, 0.3) * expdec(int(0.3 * SR), 0.06)
    return mix(thunk * 1.2, x)


def power_up():
    d = 1.3
    f = np.geomspace(80, 1100, int(d * SR))
    x = osc(f, d, "saw") * 0.4 + osc(f, d) * 0.5
    x = band(x, 60, 4000) * env(int(d * SR), 0.05, 0.2, 0.8, 0.2)
    clunk = band(noise(0.2), 200, 2000) * expdec(int(0.2 * SR), 0.03)
    buf = np.zeros(int(1.5 * SR))
    place(buf, clunk, 0)
    place(buf, x, 0.1)
    return buf


def door_slide():
    d = 1.6
    rumble = band(noise(d), 40, 300) * env(int(d * SR), 0.1, 0.2, 0.8, 0.4)
    squeal = osc(np.linspace(900, 1100, int(d * SR)), d, "saw")
    squeal = band(squeal, 700, 3000) * 0.08 * env(int(d * SR), 0.3, 0.3, 0.5, 0.4)
    clunk = band(noise(0.25), 80, 1200) * expdec(int(0.25 * SR), 0.05)
    buf = np.zeros(int(1.9 * SR))
    place(buf, rumble + squeal, 0)
    place(buf, clunk * 1.3, 1.5)
    return buf


def keycard():
    buf = np.zeros(int(0.35 * SR))
    place(buf, osc(1046, 0.1, "square") * env(int(0.1 * SR), 0.002, 0.02, 0.7, 0.02) * 0.4, 0)
    place(buf, osc(1568, 0.16, "square") * env(int(0.16 * SR), 0.002, 0.02, 0.7, 0.05) * 0.4, 0.12)
    return band(buf, 200, 5000)


def lever():
    d = 0.35
    c = band(noise(d), 150, 2500) * expdec(int(d * SR), 0.04)
    k = osc(np.linspace(300, 120, int(d * SR)), d) * expdec(int(d * SR), 0.05)
    return mix(c, k)


# ----------------------------------------------------------------- comedy / props
def whoosh():
    d = 0.55
    t = t_axis(d)
    x = noise(d)
    lo = band(x, 300, 900) * np.sin(np.pi * t / d) ** 2
    hi = band(x, 900, 3500) * np.sin(np.pi * t / d) ** 4
    return lo + hi * 0.7


def boing():
    d = 0.7
    t = t_axis(d)
    f = 180 + 220 * np.exp(-t / 0.12) + 25 * np.sin(2 * np.pi * 9 * t) * np.exp(-t / 0.3)
    return osc(f, d, "tri") * expdec(len(t), 0.25)


def thud():
    d = 0.35
    k = osc(np.linspace(140, 45, int(d * SR)), d) * expdec(int(d * SR), 0.07)
    c = band(noise(d), 200, 2500) * expdec(int(d * SR), 0.02) * 0.5
    return mix(k, c)


def rustle():
    d = 0.45
    buf = np.zeros(int(d * SR))
    for _ in range(10):
        bd = rng.uniform(0.02, 0.06)
        place(buf, band(noise(bd), 1500, 7000) * env(int(bd * SR), 0.005, 0.01, 0.6, 0.01), rng.uniform(0, d - 0.07))
    return buf


def squeak():
    d = 0.3
    t = t_axis(d)
    f = 1800 + 400 * np.sin(2 * np.pi * 12 * t)
    return band(osc(f, d, "saw"), 1200, 5000) * env(len(t), 0.02, 0.05, 0.6, 0.1) * 0.6


def foam_pop():
    d = 0.45
    thump = osc(np.linspace(220, 80, int(d * SR)), d) * expdec(int(d * SR), 0.06)
    air = band(noise(d), 500, 4000) * expdec(int(d * SR), 0.1) * 0.6
    return mix(thump, air)


def camera_motor():
    d = 1.5
    x = osc(310, d, "saw") * 0.2 + osc(620, d) * 0.1 + band(noise(d), 1000, 3000) * 0.05
    return loopify(band(x, 150, 3000), 0.15)


def shark_chomp():
    buf = np.zeros(int(0.8 * SR))
    for at in (0.0, 0.22):
        c = band(noise(0.12), 600, 5000) * expdec(int(0.12 * SR), 0.015)
        k = osc(np.linspace(200, 70, int(0.15 * SR)), 0.15) * expdec(int(0.15 * SR), 0.04)
        place(buf, mix(c, k), at)
    growl = osc(70 + 8 * np.sin(2 * np.pi * 30 * t_axis(0.5)), 0.5, "saw")
    place(buf, band(growl, 50, 700) * env(len(growl), 0.05, 0.1, 0.7, 0.2) * 0.6, 0.3)
    return buf


def smoke_hiss():
    d = 2.0
    return loopify(band(noise(d), 2500, 10000) * 0.7 + band(noise(d), 300, 1200) * 0.3, 0.3)


# ----------------------------------------------------------------- audience
def clap(amount, d, loud=1.0):
    buf = np.zeros(int(d * SR))
    for _ in range(amount):
        bd = 0.03
        c = band(noise(bd), 800, 6000) * expdec(int(bd * SR), 0.006)
        place(buf, c * rng.uniform(0.3, 1.0) * loud, rng.uniform(0, d - 0.05))
    return buf * env(len(buf), 0.3, 0.2, 1.0, 1.2)


def applause_big():
    d = 5.0
    c = clap(1100, d)
    crowd = band(noise(d), 300, 1500) * 0.25 * env(int(d * SR), 0.5, 0.3, 1.0, 1.5)
    whistle = osc(np.linspace(1500, 2600, int(0.6 * SR)), 0.6) * env(int(0.6 * SR), 0.05, 0.1, 0.8, 0.2) * 0.25
    place(c, whistle, 1.0)
    place(c, whistle[::-1] * 0.8, 2.2)
    return c + crowd


def applause_small():
    return clap(220, 3.5, 0.8)


def groan():
    d = 2.5
    t = t_axis(d)
    f = 180 * (1 - 0.35 * t / d)
    voice = sum(osc(f * k * rng.uniform(0.98, 1.02), d, "saw") for k in (1.0, 1.01, 0.99, 1.5))
    voice = band(voice, 150, 900) * env(len(t), 0.3, 0.3, 0.8, 0.8)
    return voice + band(noise(d), 200, 800) * 0.2


def laugh():
    d = 2.6
    buf = np.zeros(int(d * SR))
    for voice in range(7):
        f0 = rng.uniform(170, 320)
        at = rng.uniform(0, 0.4)
        for i in range(rng.integers(5, 9)):
            hd = 0.11
            h = osc(f0 * (1 - 0.02 * i), hd, "saw")
            h = band(h, 250, 2500) * np.sin(np.pi * t_axis(hd) / hd) ** 2
            place(buf, h * rng.uniform(0.2, 0.5), at + i * rng.uniform(0.17, 0.22))
    return buf * env(len(buf), 0.05, 0.2, 1.0, 0.8)


# ----------------------------------------------------------------- music (original)
NOTE = {n: 440 * 2 ** ((i - 9) / 12) for i, n in enumerate(["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"])}


def nf(name, octave):
    return NOTE[name] * 2 ** (octave - 4)


def pluck(f, d, shape="tri"):
    x = osc(f, d, shape) * 0.6 + osc(f * 2, d) * 0.15
    return x * env(int(d * SR), 0.004, 0.12, 0.35, 0.08)


def music_title_loop():
    bpm = 112
    beat = 60 / bpm
    bars = 8
    d = bars * 4 * beat
    buf = np.zeros(int(d * SR))
    prog = [("C", "E", "G"), ("A", "C", "E"), ("F", "A", "C"), ("G", "B", "D")] * 2
    roots = ["C", "A", "F", "G"] * 2
    melody = ["E", "G", "A", "G", "E", "D", "C", "D", "E", "G", "C", "B", "A", "G", "E", "G",
              "F", "A", "C", "A", "G", "E", "D", "E", "D", "G", "B", "A", "G", "F", "E", "D"]
    for bar in range(bars):
        t0 = bar * 4 * beat
        for b in range(4):  # walking-ish bass
            f = nf(roots[bar], 2) * (1.5 if b == 2 else 1.0)
            place(buf, pluck(f, beat * 0.9, "tri") * 0.9, t0 + b * beat)
        for b in (1, 3):  # off-beat chord stabs
            chord = sum(pluck(nf(n, 4), beat * 0.5, "square") * 0.18 for n in prog[bar])
            place(buf, band(chord, 200, 3500), t0 + b * beat)
        for i in range(4):  # melody 8ths
            n = melody[(bar * 4 + i) % len(melody)]
            place(buf, pluck(nf(n, 5), beat * 0.9, "tri") * 0.45, t0 + i * beat + (beat * 0.5 if i % 2 else 0))
        for b in range(8):  # brushed hat
            h = band(noise(0.05), 5000, 12000) * expdec(int(0.05 * SR), 0.012)
            place(buf, h * (0.3 if b % 2 else 0.15), t0 + b * beat / 2)
        kick = osc(np.linspace(120, 45, int(0.18 * SR)), 0.18) * expdec(int(0.18 * SR), 0.05)
        place(buf, kick * 0.9, t0)
        place(buf, kick * 0.7, t0 + 2 * beat)
    return loopify(np.concatenate([buf, buf[: int(0.3 * SR)]]), 0.3)


def fanfare():
    beat = 0.18
    seq = [("C", 5, 1), ("E", 5, 1), ("G", 5, 1), ("C", 6, 3), ("A", 5, 1), ("C", 6, 4)]
    buf = np.zeros(int(3.2 * SR))
    at = 0.0
    for n, o, l in seq:
        f = nf(n, o)
        d = beat * l
        v = sum(osc(f * dt, d + 0.2, "saw") for dt in (0.997, 1.0, 1.004)) / 3
        v = band(v, 150, 3500) * env(len(v), 0.02, 0.1, 0.8, 0.2)
        place(buf, v * 0.6, at)
        low = osc(f / 2, d + 0.2, "tri") * env(int((d + 0.2) * SR), 0.02, 0.1, 0.7, 0.2)
        place(buf, low * 0.4, at)
        at += d
    return buf


SOUNDS = {
    "S_Clapper": clapper, "S_UIClick": ui_click, "S_UIConfirm": ui_confirm, "S_UIError": ui_error,
    "S_Stamp": stamp, "S_RecBeep": rec_beep, "S_Siren": siren, "S_Storm": storm,
    "S_MonsterSting": monster_sting, "S_ProjectorLoop": projector_loop, "S_RainLoop": rain_loop,
    "S_WindLoop": wind_loop, "S_LeakLoop": leak_loop, "S_Splash": splash, "S_Alarm": alarm,
    "S_PowerDown": power_down, "S_PowerUp": power_up, "S_DoorSlide": door_slide, "S_Keycard": keycard,
    "S_Lever": lever, "S_Whoosh": whoosh, "S_Boing": boing, "S_Thud": thud, "S_Rustle": rustle,
    "S_Squeak": squeak, "S_FoamPop": foam_pop, "S_CameraMotor": camera_motor,
    "S_SharkChomp": shark_chomp, "S_SmokeHiss": smoke_hiss, "S_ApplauseBig": applause_big,
    "S_ApplauseSmall": applause_small, "S_Groan": groan, "S_Laugh": laugh,
    "S_MusicTitleLoop": music_title_loop, "S_Fanfare": fanfare,
}

if __name__ == "__main__":
    for name, fn in SOUNDS.items():
        save(name, fn(), 0.6 if name.startswith("S_Music") else 0.85)
