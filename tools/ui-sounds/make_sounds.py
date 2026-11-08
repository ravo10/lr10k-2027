"""Synthesises candidate UI sounds for the LR 10K software (original sounds, no samples): 3 options per event.
44.1 kHz, 16-bit mono WAV. Standard library only.

    python3 make_sounds.py <out folder> [event ...]
"""
import math
import os
import random
import struct
import sys
import wave

RATE = 44100


def silence(seconds):
    return [0.0] * int(RATE * seconds)


def bell(freq, dur, amp=1.0, attack=0.004, decay=0.25, partials=((1, 1.0), (2.0, 0.18), (3.01, 0.06)), detune=0.0):
    """A soft struck tone: a few sine partials, quick attack, exponential decay (higher partials decay faster)."""
    n = int(RATE * dur)
    out = []
    for i in range(n):
        t = i / RATE
        env = min(1.0, t / attack) if attack > 0 else 1.0
        s = 0.0
        for k, (mult, a) in enumerate(partials):
            d = decay / (1 + 1.6 * k)
            f = freq * mult * (1 + detune * k)
            s += a * math.exp(-t / d) * math.sin(2 * math.pi * f * t)
        out.append(amp * env * s)
    return out


def sweep(f0, f1, dur, amp=1.0, decay=0.05, attack=0.002):
    """A sine whose pitch glides from f0 to f1 (exponentially), with a fast decay: a soft 'pop'."""
    n = int(RATE * dur)
    out, phase = [], 0.0
    for i in range(n):
        t = i / RATE
        f = f0 * (f1 / f0) ** (t / dur)
        phase += 2 * math.pi * f / RATE
        env = min(1.0, t / attack) * math.exp(-t / decay)
        out.append(amp * env * math.sin(phase))
    return out


def noise_tick(dur=0.006, amp=0.5, tone=0.6, seed=1):
    """A very short, low-passed noise burst: the 'tactile' part of a click."""
    rnd = random.Random(seed)
    n = int(RATE * dur)
    out, y = [], 0.0
    for i in range(n):
        x = rnd.uniform(-1, 1)
        y += tone * (x - y)          # one-pole low pass
        out.append(amp * y * math.exp(-i / (n / 4)))
    return out


def mix(*tracks):
    """Mix tracks given as (start seconds, samples)."""
    length = max(int(RATE * s) + len(x) for s, x in tracks)
    out = [0.0] * length
    for s, x in tracks:
        o = int(RATE * s)
        for i, v in enumerate(x):
            out[o + i] += v
    return out


def finish(x, peak=0.5, fade=0.004, tail=0.02):
    """Normalise to a common peak (-6 dBFS), short fade-out, a little silence at the end."""
    m = max(abs(v) for v in x) or 1.0
    x = [v * peak / m for v in x]
    nf = int(RATE * fade)
    for i in range(nf):
        x[-1 - i] *= i / nf
    return x + silence(tail)


def write(path, x):
    with wave.open(path, 'wb') as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(b''.join(struct.pack('<h', max(-32767, min(32767, int(v * 32767)))) for v in x))


# Notes (Hz)
C4, E4, G4, A3, C5, D5, E5, G5, A5, B5, C6, D6, E6, G6, A6, B6 = 261.63, 329.63, 392.0, 220.0, 523.25, 587.33, 659.25, 783.99, 880.0, 987.77, 1046.5, 1174.66, 1318.51, 1567.98, 1760.0, 1975.53
GLASS = ((1, 1.0), (2.756, 0.12), (5.404, 0.03))    # glass / small bell
SOFT = ((1, 1.0), (2.0, 0.08))                       # almost pure: calm
MARIMBA = ((1, 1.0), (4.0, 0.22), (9.2, 0.05))       # wooden mallet


def candidates():
    c = {}
    # ---- buttonClick0: ordinary buttons (17 places). Short, quiet, tactile.
    c['buttonClick0'] = {
        'A': ('Soft tick', finish(mix((0, noise_tick(0.005, 0.6)), (0, bell(2200, 0.05, 0.5, 0.001, 0.012, SOFT))), peak=0.35)),
        'B': ('Pop', finish(sweep(1100, 520, 0.05, 1.0, 0.018), peak=0.35)),
        'C': ('Wood tap', finish(mix((0, noise_tick(0.004, 0.4, 0.4, 3)), (0, bell(1400, 0.06, 0.7, 0.001, 0.02, MARIMBA))), peak=0.35)),
    }
    # ---- buttonClick1: confirmations (connected, setup applied, recording on...). Positive, brief.
    c['buttonClick1'] = {
        'A': ('Glass up (two notes)', finish(mix((0, bell(E6, 0.35, 0.8, 0.003, 0.18, GLASS)), (0.07, bell(B6, 0.45, 0.7, 0.003, 0.22, GLASS))))),
        'B': ('Soft chime (one note)', finish(bell(A6, 0.5, 1.0, 0.004, 0.25, GLASS))),
        'C': ('Marimba up', finish(mix((0, bell(G5, 0.3, 0.9, 0.002, 0.12, MARIMBA)), (0.08, bell(D6, 0.35, 0.8, 0.002, 0.14, MARIMBA))))),
    }
    # ---- StartTest: the crosshead starts moving. Rising, a little longer.
    c['startTest'] = {
        'A': ('Rising three notes', finish(mix((0, bell(C5, 0.5, 0.7, 0.004, 0.25, SOFT)), (0.09, bell(E5, 0.5, 0.7, 0.004, 0.25, SOFT)),
                                              (0.18, bell(G5, 0.7, 0.8, 0.004, 0.35, SOFT))))),
        'B': ('Glass rise', finish(mix((0, bell(G5, 0.5, 0.8, 0.004, 0.25, GLASS)), (0.12, bell(D6, 0.8, 0.8, 0.004, 0.4, GLASS))))),
        'C': ('Swell up', finish(mix((0, sweep(440, 880, 0.35, 0.6, 0.25, 0.08)), (0.25, bell(A5, 0.6, 0.8, 0.004, 0.3, SOFT))))),
    }
    # ---- EndTest: the test ended. Falling, resolving.
    c['endTest'] = {
        'A': ('Falling three notes', finish(mix((0, bell(G5, 0.5, 0.8, 0.004, 0.25, SOFT)), (0.09, bell(E5, 0.5, 0.7, 0.004, 0.25, SOFT)),
                                               (0.18, bell(C5, 0.8, 0.8, 0.004, 0.4, SOFT))))),
        'B': ('Glass fall', finish(mix((0, bell(D6, 0.5, 0.8, 0.004, 0.25, GLASS)), (0.12, bell(G5, 0.9, 0.8, 0.004, 0.45, GLASS))))),
        'C': ('Soft chord', finish(mix((0, bell(C5, 1.0, 0.6, 0.01, 0.5, SOFT)), (0.0, bell(E5, 1.0, 0.5, 0.01, 0.5, SOFT)), (0.0, bell(G5, 1.0, 0.5, 0.01, 0.5, SOFT))))),
    }
    # ---- saveFileSuccess: saved (.lr10k and .xlsx). Bright, clearly "done".
    c['saveFileSuccess'] = {
        'A': ('Bright ding', finish(mix((0, bell(B6, 0.7, 1.0, 0.002, 0.35, GLASS)), (0.0, bell(B6 * 1.003, 0.7, 0.4, 0.002, 0.3, GLASS))))),
        'B': ('Success (two notes)', finish(mix((0, bell(E6, 0.35, 0.7, 0.003, 0.15, GLASS)), (0.1, bell(A6, 0.8, 0.9, 0.003, 0.4, GLASS))))),
        'C': ('Marimba three', finish(mix((0, bell(C6, 0.25, 0.8, 0.002, 0.1, MARIMBA)), (0.07, bell(E6, 0.25, 0.8, 0.002, 0.1, MARIMBA)),
                                         (0.14, bell(G6, 0.45, 0.9, 0.002, 0.18, MARIMBA))))),
    }
    # ---- error: something failed. Low, soft, unmistakable but not harsh.
    c['error'] = {
        'A': ('Low double tone', finish(mix((0, bell(A3 * 2, 0.22, 0.9, 0.003, 0.08, SOFT)), (0.14, bell(A3 * 2, 0.3, 0.9, 0.003, 0.1, SOFT))))),
        'B': ('Falling minor', finish(mix((0, bell(E4 * 2, 0.25, 0.9, 0.003, 0.1, MARIMBA)), (0.12, bell(C4 * 2, 0.4, 0.9, 0.003, 0.15, MARIMBA))))),
        'C': ('Muted thud', finish(mix((0, sweep(300, 160, 0.18, 1.0, 0.07)), (0.0, noise_tick(0.01, 0.3, 0.2, 7))))),
    }
    # ======== Machine and test controls (second set) ========
    # ---- moveUp / moveDown: Up (F2) and Down (F3); also Fast up / Fast down. Short mirror images.
    c['moveUp'] = {
        'A': ('Rising pop', finish(sweep(520, 1040, 0.09, 1.0, 0.04), peak=0.38)),
        'B': ('Marimba up (two short notes)', finish(mix((0, bell(C6, 0.12, 0.9, 0.002, 0.05, MARIMBA)), (0.05, bell(G6, 0.16, 0.9, 0.002, 0.06, MARIMBA))), peak=0.38)),
        'C': ('Glide up', finish(mix((0, sweep(660, 990, 0.12, 0.8, 0.07, 0.01)), (0, noise_tick(0.004, 0.25, 0.4, 5))), peak=0.38)),
    }
    c['moveDown'] = {
        'A': ('Falling pop', finish(sweep(1040, 520, 0.09, 1.0, 0.04), peak=0.38)),
        'B': ('Marimba down (two short notes)', finish(mix((0, bell(G6, 0.12, 0.9, 0.002, 0.05, MARIMBA)), (0.05, bell(C6, 0.16, 0.9, 0.002, 0.06, MARIMBA))), peak=0.38)),
        'C': ('Glide down', finish(mix((0, sweep(990, 660, 0.12, 0.8, 0.07, 0.01)), (0, noise_tick(0.004, 0.25, 0.4, 5))), peak=0.38)),
    }
    # ---- stop: Stop (F1). Firm, short, not musical: it must never sound like a confirmation.
    c['stop'] = {
        'A': ('Muted thunk', finish(mix((0, sweep(220, 120, 0.12, 1.0, 0.035)), (0, noise_tick(0.012, 0.6, 0.25, 11))), peak=0.45)),
        'B': ('Wood block (low, dry)', finish(mix((0, bell(330, 0.1, 1.0, 0.001, 0.025, MARIMBA)), (0, noise_tick(0.006, 0.5, 0.35, 12))), peak=0.45)),
        'C': ('Double damper', finish(mix((0, sweep(260, 150, 0.07, 1.0, 0.02)), (0.07, sweep(260, 150, 0.08, 1.0, 0.025)), (0, noise_tick(0.008, 0.4, 0.3, 13))), peak=0.45)),
    }
    # ---- returnToZero: Return to zero point (F5). "Going home": falling, landing on the lowest note.
    c['returnToZero'] = {
        'A': ('Home (three falling notes)', finish(mix((0, bell(G5, 0.2, 0.7, 0.002, 0.08, MARIMBA)), (0.07, bell(E5, 0.2, 0.7, 0.002, 0.08, MARIMBA)),
                                                    (0.14, bell(C5, 0.45, 0.9, 0.002, 0.18, MARIMBA))))),
        'B': ('Glide down to a bell', finish(mix((0, sweep(990, 520, 0.16, 0.5, 0.12, 0.01)), (0.13, bell(C5, 0.6, 0.9, 0.003, 0.28, SOFT))))),
        'C': ('Octave down', finish(mix((0, bell(C6, 0.25, 0.8, 0.003, 0.1, GLASS)), (0.1, bell(C5, 0.6, 0.9, 0.003, 0.3, GLASS))))),
    }
    # ---- recordOn / recordOff: Record > Record tests ticked / unticked. A pair; "off" lower and softer.
    c['recordOn'] = {
        'A': ('Blip up', finish(mix((0, bell(A5, 0.08, 0.8, 0.002, 0.03, SOFT)), (0.06, bell(E6, 0.2, 0.9, 0.002, 0.08, SOFT))), peak=0.42)),
        'B': ('Glass dot (high)', finish(bell(E6 * 1.5, 0.3, 1.0, 0.002, 0.1, GLASS), peak=0.42)),
        'C': ('Tape start (tick + rising tone)', finish(mix((0, noise_tick(0.005, 0.5, 0.5, 21)), (0.02, sweep(880, 1320, 0.14, 0.7, 0.09, 0.01))), peak=0.42)),
    }
    c['recordOff'] = {
        'A': ('Blip down', finish(mix((0, bell(E6, 0.08, 0.8, 0.002, 0.03, SOFT)), (0.06, bell(A5, 0.2, 0.9, 0.002, 0.08, SOFT))), peak=0.32)),
        'B': ('Glass dot (low)', finish(bell(A5, 0.3, 1.0, 0.002, 0.1, GLASS), peak=0.32)),
        'C': ('Tape stop (falling tone + tick)', finish(mix((0, sweep(1320, 880, 0.14, 0.7, 0.09, 0.01)), (0.13, noise_tick(0.005, 0.5, 0.5, 22))), peak=0.32)),
    }
    # ---- pauseTest / continueTest: Pause Test / Continue Test. A pair: "hold" and "resume".
    c['pauseTest'] = {
        'A': ('Hold (two notes down)', finish(mix((0, bell(A5, 0.2, 0.8, 0.003, 0.08, SOFT)), (0.09, bell(E5, 0.4, 0.8, 0.003, 0.16, SOFT))))),
        'B': ('Soft held tone', finish(bell(E5, 0.55, 1.0, 0.03, 0.3, SOFT), peak=0.42)),
        'C': ('Marimba repeat (same note twice)', finish(mix((0, bell(E6, 0.15, 0.8, 0.002, 0.06, MARIMBA)), (0.12, bell(E6, 0.25, 0.7, 0.002, 0.08, MARIMBA))))),
    }
    c['continueTest'] = {
        'A': ('Resume (two notes up)', finish(mix((0, bell(E5, 0.2, 0.8, 0.003, 0.08, SOFT)), (0.09, bell(A5, 0.4, 0.8, 0.003, 0.16, SOFT))))),
        'B': ('Soft rising tone', finish(mix((0, sweep(330, 440, 0.3, 0.6, 0.25, 0.04)), (0.2, bell(A5, 0.4, 0.6, 0.004, 0.2, SOFT))), peak=0.42)),
        'C': ('Marimba step up', finish(mix((0, bell(E6, 0.15, 0.8, 0.002, 0.06, MARIMBA)), (0.12, bell(A6, 0.25, 0.8, 0.002, 0.08, MARIMBA))))),
    }
    # ---- setZeroPoint: Set zero point (before every test). A neutral, recognisable "mark".
    c['setZeroPoint'] = {
        'A': ('Tick and ping', finish(mix((0, noise_tick(0.004, 0.5, 0.5, 31)), (0.03, bell(B6, 0.2, 0.7, 0.002, 0.06, GLASS))), peak=0.4)),
        'B': ('Double tick', finish(mix((0, bell(1800, 0.04, 0.8, 0.001, 0.01, MARIMBA)), (0.06, bell(1800, 0.05, 0.8, 0.001, 0.012, MARIMBA))), peak=0.38)),
        'C': ('Soft click-ding', finish(mix((0, noise_tick(0.004, 0.4, 0.4, 32)), (0.0, bell(C6, 0.3, 0.8, 0.002, 0.12, SOFT))), peak=0.4)),
    }
    # ---- warning: safety and machine warnings (Down stopped above 50 N, the LR 10K stopped answering).
    # Hard to miss, clearly different from "error" (which is low): higher, repeated.
    c['warning'] = {
        'A': ('Two-tone alert', finish(mix((0, bell(E6, 0.15, 0.9, 0.003, 0.08, SOFT)), (0.15, bell(C6, 0.15, 0.9, 0.003, 0.08, SOFT)),
                                            (0.3, bell(E6, 0.15, 0.9, 0.003, 0.08, SOFT)), (0.45, bell(C6, 0.25, 0.9, 0.003, 0.1, SOFT))))),
        'B': ('Triple pulse', finish(mix((0, bell(A5 * 1.5, 0.1, 1.0, 0.003, 0.05, SOFT)), (0.13, bell(A5 * 1.5, 0.1, 1.0, 0.003, 0.05, SOFT)),
                                          (0.26, bell(A5 * 1.5, 0.18, 1.0, 0.003, 0.07, SOFT))))),
        'C': ('Glass alarm (up and down)', finish(mix((0, bell(D6, 0.15, 0.8, 0.003, 0.07, GLASS)), (0.11, bell(A6, 0.15, 0.8, 0.003, 0.07, GLASS)),
                                                    (0.22, bell(D6, 0.3, 0.8, 0.003, 0.12, GLASS))))),
    }
    return c


if __name__ == '__main__':
    out = sys.argv[1]
    only = set(sys.argv[2:])          # optional: only these events
    os.makedirs(out, exist_ok=True)
    for event, options in candidates().items():
        if only and event not in only:
            continue
        for key, (name, samples) in options.items():
            path = os.path.join(out, '%s_%s.wav' % (event, key))
            write(path, samples)
            print('%-28s %-26s %.2f s' % (os.path.basename(path), name, len(samples) / RATE))
