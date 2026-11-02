"""Original procedurally synthesised music, SFX and ambience (CC0). Requires numpy + ffmpeg (libvorbis)."""
import os, subprocess, wave, math
import numpy as np
from gfx_common import out

SR = 44100
rng = np.random.default_rng(1234)

def write_wav(path, x, sr=SR):
    x = np.clip(x, -1, 1)
    data = (x * 32000).astype(np.int16)
    with wave.open(path, 'wb') as w:
        w.setnchannels(1 if data.ndim == 1 else 2); w.setsampwidth(2); w.setframerate(sr)
        w.writeframes(data.tobytes())

def write_ogg(path, x):
    tmp = path + '.tmp.wav'
    write_wav(tmp, x)
    subprocess.run(['ffmpeg', '-y', '-loglevel', 'error', '-i', tmp, '-c:a', 'libvorbis', '-q:a', '4', path], check=True)
    os.remove(tmp)

def env(n, a=0.005, d=0.1, s=0.6, r=0.1, sr=SR):
    t = np.arange(n) / sr
    dur = n / sr
    e = np.ones(n) * s
    ai = t < a
    e[ai] = t[ai] / a
    di = (t >= a) & (t < a + d)
    e[di] = 1 - (1 - s) * (t[di] - a) / d
    ri = t > dur - r
    e[ri] *= np.clip((dur - t[ri]) / r, 0, 1)
    return e

def osc(kind, f, n, sr=SR, phase=0):
    t = np.arange(n) / sr
    if np.isscalar(f):
        ph = 2 * np.pi * f * t + phase
    else:
        ph = 2 * np.pi * np.cumsum(f) / sr + phase
    if kind == 'sine':
        return np.sin(ph)
    if kind == 'square':
        return np.sign(np.sin(ph)) * 0.6
    if kind == 'pulse':
        return np.where((ph % (2 * np.pi)) < 2 * np.pi * 0.25, 0.6, -0.6)
    if kind == 'saw':
        return ((ph / (2 * np.pi)) % 1.0) * 2 - 1
    if kind == 'tri':
        return 2 * np.abs(((ph / (2 * np.pi)) % 1.0) * 2 - 1) - 1
    if kind == 'bell':
        return np.sin(ph) * 0.6 + np.sin(ph * 2.76) * 0.25 * np.exp(-t * 6) + np.sin(ph * 5.4) * 0.1 * np.exp(-t * 10)
    if kind == 'organ':
        return np.sin(ph) * 0.5 + np.sin(ph * 2) * 0.3 + np.sin(ph * 3) * 0.2
    raise ValueError(kind)

def lowpass(x, alpha):
    y = np.zeros_like(x)
    acc = 0.0
    # vectorised one-pole via lfilter equivalent
    from scipy.signal import lfilter
    return lfilter([alpha], [1, alpha - 1], x)

def noise(n):
    return rng.uniform(-1, 1, n)

def midi(m):
    return 440.0 * 2 ** ((m - 69) / 12)

# ---------------------------------------------------------------- SFX
def sfx():
    S = {}
    n = lambda d: int(SR * d)
    # jump: rising chirp
    d = n(0.18); f = np.linspace(380, 900, d); S['jump'] = osc('square', f, d) * env(d, 0.002, 0.05, 0.4, 0.08) * 0.5
    d = n(0.16); f = np.linspace(600, 1300, d); S['double_jump'] = (osc('tri', f, d) + osc('sine', f * 1.5, d) * 0.5) * env(d, 0.002, 0.05, 0.4, 0.08) * 0.5
    d = n(0.12); S['land'] = lowpass(noise(d), 0.08) * env(d, 0.001, 0.04, 0.2, 0.06) * 1.6 + osc('sine', np.linspace(140, 60, d), d) * env(d, 0.001, 0.05, 0.1, 0.05) * 0.6
    d = n(0.22); S['attack'] = lowpass(noise(d), np.float64(0.25)) * env(d, 0.01, 0.08, 0.3, 0.1) * 0.9 + osc('sine', np.linspace(300, 700, d), d) * env(d, 0.002, 0.1, 0.0, 0.05) * 0.3
    d = n(0.15); S['enemy_hit'] = (osc('square', np.linspace(500, 150, d), d) * 0.5 + noise(d) * 0.3) * env(d, 0.001, 0.05, 0.2, 0.08)
    d = n(0.4); S['enemy_die'] = (osc('square', np.linspace(700, 80, d), d) * 0.4 + lowpass(noise(d), 0.2) * 0.5) * env(d, 0.001, 0.2, 0.2, 0.15)
    d = n(0.25); S['ting'] = (osc('bell', 1568, d) * 0.5 + osc('bell', 2093, d) * 0.4 * (np.arange(d) > n(0.05))) * env(d, 0.001, 0.15, 0.0, 0.05)
    d = n(0.6); S['gem'] = sum(osc('bell', midi(m), d) * (np.arange(d) > n(0.07 * k)) * 0.3 for k, m in enumerate([76, 79, 84, 88])) * env(d, 0.001, 0.5, 0.0, 0.1)
    d = n(0.9); S['checkpoint'] = sum(osc('tri', midi(m), d) * (np.arange(d) > n(0.1 * k)) * 0.35 for k, m in enumerate([67, 72, 76, 79])) * env(d, 0.002, 0.7, 0.0, 0.2)
    d = n(1.2); S['death'] = sum(osc('square', midi(m) * np.linspace(1, 0.97, d), d) * ((np.arange(d) > n(0.18 * k)) & (np.arange(d) < n(0.18 * k + 0.3))) * 0.3 for k, m in enumerate([72, 68, 65, 60, 56])) * env(d, 0.001, 1.0, 0.3, 0.2)
    d = n(2.2); notes = [60, 64, 67, 72, 67, 72, 76, 79, 84]
    S['level_complete'] = sum((osc('pulse', midi(m), d) * 0.3 + osc('tri', midi(m - 12), d) * 0.3) * ((np.arange(d) > n(0.12 * k)) & (np.arange(d) < n(0.12 * k + (0.9 if k == 8 else 0.14)))) for k, m in enumerate(notes)) * env(d, 0.001, 2.0, 0.6, 0.3)
    d = n(0.06); S['menu_move'] = osc('square', 880, d) * env(d, 0.001, 0.03, 0.2, 0.02) * 0.35
    d = n(0.2); S['menu_select'] = (osc('square', 660, d) * (np.arange(d) < n(0.07)) + osc('square', 990, d) * (np.arange(d) >= n(0.07))) * env(d, 0.001, 0.1, 0.4, 0.05) * 0.35
    d = n(0.15); S['menu_back'] = osc('square', np.linspace(660, 330, d), d) * env(d, 0.001, 0.08, 0.3, 0.05) * 0.35
    d = n(0.5); S['hurt'] = (osc('saw', np.linspace(500, 200, d), d) * 0.4 + noise(d) * 0.15) * env(d, 0.001, 0.2, 0.2, 0.2)
    d = n(0.35); S['spring'] = osc('sine', 300 + 250 * np.sin(np.arange(d) / SR * 2 * np.pi * 18) + np.linspace(0, 500, d), d) * env(d, 0.001, 0.2, 0.3, 0.1) * 0.6
    d = n(0.35); S['break'] = lowpass(noise(d), 0.3) * env(d, 0.001, 0.15, 0.2, 0.15) + osc('square', np.linspace(200, 60, d), d) * env(d, 0.001, 0.1, 0.0, 0.1) * 0.3
    d = n(0.25); S['boss_hit'] = (osc('square', np.linspace(300, 90, d), d) * 0.6 + lowpass(noise(d), 0.3) * 0.5) * env(d, 0.001, 0.1, 0.3, 0.1)
    d = n(1.0); S['boss_roar'] = lowpass(noise(d), 0.05) * 2.0 * env(d, 0.05, 0.4, 0.6, 0.4) + osc('saw', 70 + 15 * np.sin(np.arange(d) / SR * 40), d) * 0.4 * env(d, 0.05, 0.4, 0.6, 0.4)
    d = n(0.7); S['explosion'] = lowpass(noise(d), 0.12) * 2.2 * env(d, 0.001, 0.3, 0.3, 0.3) + osc('sine', np.linspace(120, 30, d), d) * env(d, 0.001, 0.4, 0.0, 0.2) * 0.8
    d = n(0.5); S['key'] = sum(osc('bell', midi(m), d) * (np.arange(d) > n(0.06 * k)) * 0.35 for k, m in enumerate([84, 88, 91])) * env(d, 0.001, 0.4, 0.0, 0.1)
    d = n(0.6); S['door'] = (osc('saw', np.linspace(90, 110, d), d) * 0.4 + lowpass(noise(d), 0.1) * 0.8) * env(d, 0.05, 0.3, 0.6, 0.2)
    d = n(1.0); S['oneup'] = sum(osc('pulse', midi(m), d) * ((np.arange(d) > n(0.1 * k)) & (np.arange(d) < n(0.1 * k + 0.12))) * 0.35 for k, m in enumerate([76, 79, 88, 84, 86, 91])) * env(d, 0.001, 0.9, 0.5, 0.1)
    d = n(0.6); S['cage_break'] = lowpass(noise(d), 0.3) * env(d, 0.001, 0.2, 0.1, 0.2) * 0.8 + sum(osc('bell', midi(m), d) * (np.arange(d) > n(0.1 + 0.06 * k)) * 0.3 for k, m in enumerate([79, 83, 86, 91])) * env(d, 0.001, 0.5, 0.0, 0.1)
    d = n(0.12); S['glide'] = lowpass(noise(d), 0.15) * env(d, 0.02, 0.05, 0.6, 0.05) * 0.5
    d = n(0.25); S['shoot'] = osc('square', np.linspace(1200, 300, d), d) * env(d, 0.001, 0.1, 0.1, 0.1) * 0.35
    d = n(0.35); S['splash'] = lowpass(noise(d), 0.35) * env(d, 0.001, 0.25, 0.1, 0.1) * 0.8
    d = n(0.8); S['upgrade'] = sum(osc('tri', midi(m), d) * (np.arange(d) > n(0.05 * k)) * 0.25 for k, m in enumerate(range(60, 96, 4))) * env(d, 0.001, 0.7, 0.0, 0.1)
    d = n(0.3); S['laser'] = osc('saw', 900 + 400 * np.sin(np.arange(d) / SR * 2 * np.pi * 30), d) * env(d, 0.001, 0.2, 0.3, 0.1) * 0.3
    d = n(0.5); S['shockwave'] = osc('sine', np.linspace(90, 40, d), d) * env(d, 0.001, 0.4, 0.0, 0.1) * 1.0 + lowpass(noise(d), 0.06) * env(d, 0.001, 0.3, 0.0, 0.1) * 1.4
    d = n(0.2); S['achievement'] = 0
    d = n(1.0); S['achievement'] = sum(osc('bell', midi(m), d) * (np.arange(d) > n(0.08 * k)) * 0.3 for k, m in enumerate([72, 76, 79, 84, 88])) * env(d, 0.001, 0.9, 0.0, 0.1)
    for k, v in S.items():
        v = np.asarray(v, dtype=np.float64)
        peak = np.max(np.abs(v)) + 1e-9
        v = v / peak * 0.8
        write_wav(out('sounds', k + '.wav'), v)

# ---------------------------------------------------------------- MUSIC
SCALES = {'major': [0, 2, 4, 5, 7, 9, 11], 'minor': [0, 2, 3, 5, 7, 8, 10], 'dorian': [0, 2, 3, 5, 7, 9, 10],
          'lydian': [0, 2, 4, 6, 7, 9, 11], 'phrygian': [0, 1, 3, 5, 7, 8, 10], 'mixo': [0, 2, 4, 5, 7, 9, 10], 'harm': [0, 2, 3, 5, 7, 8, 11]}

def drum_kick(n):
    t = np.arange(n) / SR
    return np.sin(2 * np.pi * np.cumsum(60 + 120 * np.exp(-t * 30)) / SR) * np.exp(-t * 9)

def drum_snare(n):
    t = np.arange(n) / SR
    return (noise(n) * 0.7 + np.sin(2 * np.pi * 190 * t) * 0.4) * np.exp(-t * 18)

def drum_hat(n):
    t = np.arange(n) / SR
    from scipy.signal import lfilter
    h = lfilter([1, -1], [1], noise(n))
    return h * np.exp(-t * 60) * 0.4

def song(name, bpm, root, scale, prog, lead, bass, pad, bars=16, drums='basic', swing=0.0, seed=0, lead_oct=0, density=0.7, arp=False):
    r = np.random.default_rng(seed)
    beat = 60.0 / bpm
    step = beat / 4
    nsteps = bars * 16
    total = int(nsteps * step * SR) + SR
    mixL = np.zeros(total); mixR = np.zeros(total)
    sc = SCALES[scale]
    def deg(dg, octv=0):
        o, i = divmod(dg, 7)
        return root + 12 * (o + octv) + sc[i]
    def place(sig, t0, pan=0.0, vol=1.0):
        i0 = int(t0 * SR)
        i1 = min(total, i0 + len(sig))
        mixL[i0:i1] += sig[:i1 - i0] * vol * (1 - pan) * 0.5
        mixR[i0:i1] += sig[:i1 - i0] * vol * (1 + pan) * 0.5
    # melody motif generation: 2-bar motifs repeated with variation (AABA form)
    motifs = []
    for m in range(3):
        mot = []
        for s in range(32):
            if s % 2 == 0 and r.random() < density or s % 8 == 0:
                mot.append(int(r.integers(0, 8)))
            else:
                mot.append(None)
        motifs.append(mot)
    form = [0, 0, 1, 0, 2, 2, 1, 0]
    for bar in range(bars):
        chord = prog[bar % len(prog)]
        cstart = bar * 16 * step
        # pad chord
        cn = int(16 * step * SR)
        notes = [deg(chord), deg(chord + 2), deg(chord + 4)]
        for k, mn in enumerate(notes):
            sig = osc(pad, midi(mn), cn) * env(cn, 0.3, 0.5, 0.7, 0.4) * 0.12
            place(sig, cstart, pan=(k - 1) * 0.5)
        # bass
        for b in range(4 if drums != 'none' else 2):
            bn = int((4 if drums != 'none' else 8) * step * SR)
            pat = [0, 0, 4, 0][b] if drums != 'none' else 0
            sig = osc(bass, midi(deg(chord + pat, -2)), bn) * env(bn, 0.005, 0.15, 0.5, 0.05) * 0.35
            place(sig, cstart + b * (4 if drums != 'none' else 8) * step)
        # arpeggio
        if arp:
            for s in range(16):
                an = int(step * SR * 0.9)
                mn = notes[s % 3] + 12 * (1 + (s // 3) % 2)
                place(osc('pulse', midi(mn), an) * env(an, 0.002, 0.05, 0.3, 0.03) * 0.07, cstart + s * step, pan=0.4 * math.sin(s))
        # melody
        mot = motifs[form[(bar // 2) % len(form)]]
        for s in range(16):
            v = mot[(bar % 2) * 16 + s]
            if v is None:
                continue
            length = 1
            while s + length < 16 and mot[(bar % 2) * 16 + s + length] is None and length < 4:
                length += 1
            ln = int(length * step * SR)
            dg = chord + v
            sw = swing * step if s % 2 == 1 else 0
            sig = osc(lead, midi(deg(dg, 1 + lead_oct)), ln) * env(ln, 0.005, 0.1, 0.6, 0.05) * 0.22
            # vibrato-free echo for depth
            place(sig, cstart + s * step + sw, pan=-0.15)
            place(sig * 0.35, cstart + s * step + sw + step * 3, pan=0.35)
        # drums
        if drums != 'none':
            for s in range(16):
                t0 = cstart + s * step + (swing * step if s % 2 else 0)
                if drums in ('basic', 'rock', 'tribal', 'electro'):
                    if s in (0, 8) or (drums in ('rock', 'electro') and s in (6, 10, 14) and drums == 'electro') or (drums == 'tribal' and s in (3, 6, 11)):
                        place(drum_kick(int(0.3 * SR)), t0, vol=0.8)
                    if s in (4, 12):
                        place(drum_snare(int(0.25 * SR)), t0, vol=0.45 if drums != 'tribal' else 0.25)
                    if s % 2 == 0 or drums == 'electro':
                        place(drum_hat(int(0.06 * SR)), t0, pan=0.3, vol=0.5)
                elif drums == 'soft':
                    if s == 0:
                        place(drum_kick(int(0.3 * SR)), t0, vol=0.4)
                    if s % 4 == 2:
                        place(drum_hat(int(0.05 * SR)), t0, pan=0.3, vol=0.35)
    loop_len = int(nsteps * step * SR)
    # fold the tail into the start for a seamless loop
    tail = total - loop_len
    mixL[:tail] += mixL[loop_len:]; mixR[:tail] += mixR[loop_len:]
    st = np.stack([mixL[:loop_len], mixR[:loop_len]], axis=1)
    st = st / (np.max(np.abs(st)) + 1e-9) * 0.85
    write_ogg(out('music', name + '.ogg'), st)

def music():
    song('menu', 96, 62, 'lydian', [0, 4, 5, 3], 'bell', 'tri', 'sine', bars=16, drums='soft', seed=1, arp=True)
    song('worldmap', 112, 60, 'major', [0, 5, 3, 4], 'pulse', 'tri', 'organ', bars=16, drums='basic', seed=2, swing=0.1)
    song('world1', 104, 64, 'dorian', [0, 3, 5, 4], 'bell', 'tri', 'sine', bars=16, drums='soft', seed=11, arp=True)
    song('world2', 128, 57, 'mixo', [0, 6, 3, 0], 'square', 'saw', 'organ', bars=16, drums='tribal', seed=12, swing=0.25)
    song('world3', 134, 61, 'minor', [0, 5, 2, 6], 'saw', 'square', 'sine', bars=16, drums='electro', seed=13, arp=True)
    song('world4', 88, 66, 'minor', [0, 5, 3, 4], 'bell', 'sine', 'tri', bars=16, drums='soft', seed=14)
    song('world5', 120, 52, 'phrygian', [0, 1, 0, 6], 'saw', 'square', 'organ', bars=16, drums='rock', seed=15)
    song('boss', 150, 57, 'harm', [0, 0, 5, 4], 'square', 'saw', 'organ', bars=16, drums='rock', seed=21, arp=True, density=0.9)
    song('boss_final', 160, 52, 'harm', [0, 1, 5, 4], 'saw', 'saw', 'organ', bars=16, drums='electro', seed=22, arp=True, density=0.9)
    song('victory', 116, 60, 'major', [0, 3, 4, 0], 'pulse', 'tri', 'organ', bars=12, drums='basic', seed=31, arp=True)
    song('gameover', 70, 57, 'minor', [0, 5, 3, 4], 'bell', 'sine', 'sine', bars=4, drums='none', seed=41)

def ambience():
    from scipy.signal import lfilter
    d = SR * 12
    t = np.arange(d) / SR
    def loopify(x):
        f = SR
        x[:f] = x[:f] * np.linspace(0, 1, f) + x[-f:] * np.linspace(1, 0, f)
        return x[:-f]
    # forest: soft wind + birdsong chirps
    x = lfilter([0.02], [1, -0.98], noise(d)) * (0.6 + 0.4 * np.sin(t * 0.7))
    for k in range(18):
        t0 = int(rng.uniform(0, 10.5) * SR); n = int(0.12 * SR)
        f = 2500 + 1200 * np.sin(np.linspace(0, 3 * np.pi, n)) + rng.uniform(-300, 300)
        x[t0:t0 + n] += osc('sine', f, n) * env(n, 0.01, 0.05, 0.4, 0.05) * 0.15
    write_ogg(out('sounds', 'amb_forest.ogg'), loopify(x / np.max(np.abs(x)) * 0.5))
    # band land: distant festival drums
    x = lfilter([0.03], [1, -0.97], noise(d)) * 0.3
    for k in range(int(12 / 0.5)):
        t0 = int(k * 0.5 * SR); n = int(0.3 * SR)
        x[t0:t0 + n] += drum_kick(n)[:len(x[t0:t0 + n])] * 0.25
    write_ogg(out('sounds', 'amb_band.ogg'), loopify(x / np.max(np.abs(x)) * 0.5))
    # city: electric hum + traffic swells
    x = osc('saw', 60, d) * 0.05 + osc('sine', 120, d) * 0.05 + lfilter([0.01], [1, -0.99], noise(d)) * (0.5 + 0.5 * np.sin(t * 0.9)) * 2
    write_ogg(out('sounds', 'amb_city.ogg'), loopify(x / np.max(np.abs(x)) * 0.5))
    # snow: howling wind
    w = lfilter([0.05], [1, -0.95], noise(d))
    x = w * (0.5 + 0.5 * np.sin(t * 0.5 + np.sin(t * 1.3))) + osc('sine', 400 + 120 * np.sin(t * 0.8), d) * 0.04
    write_ogg(out('sounds', 'amb_snow.ogg'), loopify(x / np.max(np.abs(x)) * 0.5))
    # lava: deep rumble + bubbles
    x = lfilter([0.005], [1, -0.995], noise(d)) * 6
    for k in range(40):
        t0 = int(rng.uniform(0, 10.8) * SR); n = int(0.08 * SR)
        x[t0:t0 + n] += osc('sine', np.linspace(200, 500, n), n) * env(n, 0.005, 0.03, 0.2, 0.03) * 0.2
    write_ogg(out('sounds', 'amb_lava.ogg'), loopify(x / np.max(np.abs(x)) * 0.5))

if __name__ == '__main__':
    sfx(); ambience(); music()
