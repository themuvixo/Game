"""Original enemies (6 behaviours x 5 worlds) and 5 bosses, procedurally drawn."""
import math
from gfx_common import *

INK = (30, 16, 40)
WORLD_PAL = {
    1: dict(main=(120, 200, 90), acc=(255, 110, 180), dark=(60, 110, 60), eye=(255, 240, 120), orn='leaf'),
    2: dict(main=(240, 140, 50), acc=(255, 220, 90), dark=(150, 60, 30), eye=(255, 255, 255), orn='band'),
    3: dict(main=(60, 70, 110), acc=(0, 240, 255), dark=(30, 30, 60), eye=(255, 40, 200), orn='neon'),
    4: dict(main=(200, 230, 255), acc=(120, 190, 255), dark=(110, 140, 190), eye=(30, 80, 160), orn='ice'),
    5: dict(main=(110, 40, 40), acc=(255, 130, 30), dark=(50, 20, 25), eye=(255, 230, 80), orn='fire'),
}
NAMES = {
    1: ['Mossling', 'Pollen Moth', 'Barkback', 'Puffcap', 'Thornboar', 'Seedspitter'],
    2: ['Drumbug', 'Flutternote', 'Tuba Turtle', 'Cymbomb', 'Bass Bull', 'Horn Turret'],
    3: ['Glitchbot', 'Neon Drone', 'Riot Cube', 'Volt Bomb', 'Turbo Rat', 'Laser Eye'],
    4: ['Snowpup', 'Frost Bat', 'Glacier Crab', 'Snowbomb', 'Yakcharger', 'Icicle Mage'],
    5: ['Ember Imp', 'Ash Wraith', 'Magma Golem', 'Lava Blob', 'Fire Hound', 'Pyre Totem'],
}
TYPES = ['walker', 'flyer', 'armored', 'exploder', 'charger', 'shooter']
FW = 64

def ornament(c, w, cx, cy, r, ph):
    P = WORLD_PAL[w]
    if P['orn'] == 'leaf':
        c.rot_ellipse(cx - 4, cy - r - 2, 6, 3, -0.6 + ph * 0.1, (90, 190, 80))
        c.rot_ellipse(cx + 4, cy - r - 3, 6, 3, 0.6 - ph * 0.1, (110, 210, 90))
        for k in range(3):
            c.ellipse(cx - r * 0.5 + k * r * 0.5, cy - r * 0.45, 2.2, 2.2, (255, 230, 240))
    elif P['orn'] == 'band':
        c.rect(cx - r, cy - 3, cx + r, cy + 3, P['acc'])
        for k in range(-2, 3):
            c.line([(cx + k * r * 0.4, cy - 3), (cx + k * r * 0.4 + 3, cy + 3)], P['dark'], 1.2)
    elif P['orn'] == 'neon':
        c.line([(cx, cy - r), (cx + 3, cy - r - 9)], P['acc'], 1.5)
        c.glow(cx + 3, cy - r - 9, 5, P['acc'], 0.9)
        c.ellipse(cx + 3, cy - r - 9, 2, 2, (255, 255, 255))
    elif P['orn'] == 'ice':
        for k in range(-2, 3):
            c.poly([(cx + k * 5 - 2.5, cy - r + 3), (cx + k * 5 + 2.5, cy - r + 3), (cx + k * 5, cy - r - 6 - (k % 2) * 3)], (235, 250, 255))
    elif P['orn'] == 'fire':
        for k in range(-1, 2):
            h = 9 + 4 * math.sin(ph * 2 + k)
            c.poly([(cx + k * 6 - 4, cy - r + 4), (cx + k * 6 + 4, cy - r + 4), (cx + k * 6 + math.sin(ph + k) * 2, cy - r - h)], (255, 160 + k * 30, 40))
        c.glow(cx, cy - r, 10, (255, 120, 20), 0.6)

def eyes(c, w, x, y, s, look=1.5, mode='normal'):
    P = WORLD_PAL[w]
    for ox in (-s * 0.9, s * 0.9):
        if mode == 'hit':
            c.line([(x + ox - 2.5, y - 2.5), (x + ox + 2.5, y + 2.5)], INK, 1.6)
            c.line([(x + ox - 2.5, y + 2.5), (x + ox + 2.5, y - 2.5)], INK, 1.6)
        elif mode == 'angry':
            c.ellipse(x + ox, y, s * 0.75, s * 0.75, P['eye'])
            c.ellipse(x + ox + look, y + 0.5, s * 0.35, s * 0.4, INK)
            c.line([(x + ox - s * 0.8, y - s * 1.0 + (1 if ox < 0 else -1) * -1.5), (x + ox + s * 0.8, y - s * 1.0 + (1 if ox < 0 else -1) * 1.5)], INK, 1.6)
        else:
            c.ellipse(x + ox, y, s * 0.75, s * 0.85, P['eye'])
            c.ellipse(x + ox + look, y + 0.5, s * 0.35, s * 0.45, INK)

def creature(w, t, state, i, n):
    P = WORLD_PAL[w]
    c = Canvas(FW, FW)
    ph = i / max(1, n) * 2 * math.pi
    s = math.sin(ph)
    alpha = 1.0
    scale = 1.0
    emode = 'angry' if state in ('attack',) else ('hit' if state in ('hit', 'death') else 'normal')
    tint = None
    if state == 'hit':
        tint = (255, 255, 255)
    if state == 'death':
        tint = None
        scale = 1 - i / n * 0.6
        alpha = 1 - i / n * 0.85
    cx, cy = 32, 38
    def S(v):
        return v * scale
    main = mix(P['main'], (255, 255, 255), 0.6) if tint else P['main']
    if t == 'walker':
        bob = abs(s) * 2 if state == 'move' else s * 0.8
        c.ellipse(cx - 8 + (s * 5 if state == 'move' else 0), 56, 7, 4, P['dark'])
        c.ellipse(cx + 8 - (s * 5 if state == 'move' else 0), 56, 7, 4, P['dark'])
        c.ellipse(cx, cy + 4 - bob, S(17), S(15), main)
        c.ellipse(cx - 4, cy + 8 - bob, S(11), S(7), darken(main, 0.12))
        if w == 1:  # mushroom cap
            c.ellipse(cx, cy - 8 - bob, S(21), S(11), P['acc'])
            for k in range(3):
                c.ellipse(cx - 10 + k * 10, cy - 11 - bob, 3, 2.5, (255, 240, 250))
        else:
            ornament(c, w, cx, cy + 4 - bob, S(15), ph)
        eyes(c, w, cx + 4, cy + 3 - bob, 4.5, 1.5, emode)
        if state == 'attack':
            c.ellipse(cx + 8, cy + 12 - bob, 4, 3 + i, (120, 20, 40))
    elif t == 'flyer':
        fl = math.sin(ph * 2) * 10
        c.poly([(cx - 4, cy), (cx - 24, cy - 12 - fl), (cx - 18, cy + 6)], lighten(P['acc'], 0.3))
        c.poly([(cx + 4, cy), (cx + 24, cy - 12 - fl), (cx + 18, cy + 6)], P['acc'])
        c.ellipse(cx, cy + s * 2, S(12), S(12), main)
        ornament(c, w, cx, cy + s * 2, S(12), ph)
        eyes(c, w, cx + 3, cy - 1 + s * 2, 3.8, 1.2, emode)
        if state == 'attack':
            c.poly([(cx - 3, cy + 11), (cx + 3, cy + 11), (cx, cy + 20)], P['dark'])
    elif t == 'armored':
        c.ellipse(cx - 12 + s * 3, 56, 6, 4, P['dark'])
        c.ellipse(cx + 12 - s * 3, 56, 6, 4, P['dark'])
        c.ellipse(cx + 12, cy + 6, S(10), S(10), lighten(main, 0.1))
        eyes(c, w, cx + 16, cy + 4, 3.3, 1.5, emode)
        shell = (150, 150, 165) if w == 3 else darken(P['acc'], 0.25)
        c.ellipse(cx - 3, cy + 4, S(22), S(17), shell)
        c.ellipse(cx - 3, cy + 2, S(16), S(11), lighten(shell, 0.25))
        for k in range(-1, 2):
            c.arc(cx - 3 + k * 9, cy + 4, 6, 10, 180, 360, darken(shell, 0.4), 1.4)
        ornament(c, w, cx - 3, cy + 4, S(17), ph)
    elif t == 'exploder':
        pulse = 1 + (0.1 * math.sin(ph * 3) if state in ('attack', 'move') else 0.02 * s)
        if state == 'attack':
            main = mix(main, (255, 60, 40), i / max(1, n - 1))
        c.ellipse(cx, cy + 2, S(16 * pulse), S(16 * pulse), main)
        c.ellipse(cx - 5, cy - 3, 5, 4, lighten(main, 0.4))
        c.line([(cx + 4, cy - 14), (cx + 9, cy - 21)], (90, 70, 50), 2)
        c.glow(cx + 9, cy - 22, 6 + 2 * abs(s), (255, 200, 60), 1.0)
        ornament(c, w, cx, cy + 2, S(16), ph) if P['orn'] in ('band', 'leaf') else None
        eyes(c, w, cx + 2, cy + 3, 4, 1.0, emode)
        c.ellipse(cx - 6, 56, 5, 3, P['dark']); c.ellipse(cx + 6, 56, 5, 3, P['dark'])
    elif t == 'charger':
        run = state in ('move', 'attack')
        k = 2.0 if state == 'attack' else 1.0
        for j, ox in enumerate((-14, -6, 8, 16)):
            off = (s if j % 2 else -s) * 4 * k if run else 0
            c.rect(cx + ox - 2 + off, 46, cx + ox + 3 + off, 57, P['dark'], radius=2)
        c.rot_ellipse(cx, cy + 4, S(22), S(12), 0.1 if state == 'attack' else 0, main)
        c.ellipse(cx + 18, cy + 4, S(10), S(9), lighten(main, 0.12))
        horn = (255, 245, 220) if w != 3 else P['acc']
        c.poly([(cx + 16, cy - 2), (cx + 30, cy - 12), (cx + 22, cy + 1)], horn)
        ornament(c, w, cx - 4, cy + 2, S(11), ph)
        eyes(c, w, cx + 20, cy + 2, 3, 1.5, 'angry' if run else emode)
        if state == 'attack':
            for j in range(3):
                c.line([(cx - 28 - j * 3, cy - 4 + j * 6), (cx - 36 - j * 3, cy - 4 + j * 6)], (255, 255, 255, 180), 1.5)
    elif t == 'shooter':
        recoil = -3 if state == 'attack' and i in (1, 2) else 0
        c.rect(cx - 14, 44, cx + 14, 60, P['dark'], radius=4)
        c.ellipse(cx + recoil, cy, S(15), S(15), main)
        ornament(c, w, cx + recoil, cy, S(15), ph)
        c.rect(cx + 8 + recoil, cy - 4, cx + 26 + recoil, cy + 4, darken(main, 0.3), radius=3)
        c.ellipse(cx + 26 + recoil, cy, 3, 5, P['acc'])
        if state == 'attack' and i == 2:
            c.glow(cx + 28, cy, 10, P['acc'], 1.0)
        eyes(c, w, cx - 2 + recoil, cy - 2, 4, 2.0, emode)
    img = outline_image(c.final(), INK, 1)
    if state == 'death':
        img = img.rotate(i * 25, resample=Image.BICUBIC, center=(32, 40))
    if alpha < 1:
        img.putalpha(img.split()[3].point(lambda v: int(v * alpha)))
    return img

STATES = [('idle', 4, 0.15, 1), ('move', 6, 0.09, 1), ('attack', 4, 0.1, 0), ('hit', 2, 0.08, 0), ('death', 6, 0.08, 0)]

def build_enemies():
    for w in range(1, 6):
        for t in TYPES:
            rows = [[creature(w, t, st, i, n) for i in range(n)] for st, n, _, _ in STATES]
            sheet(rows, FW, FW).save(out('enemies', 'w%d_%s.png' % (w, t)))
    with open(out('enemies', 'enemy.anim'), 'w') as f:
        f.write('frame %d %d\n' % (FW, FW))
        for r, (st, n, d, l) in enumerate(STATES):
            f.write('%s %d %d %.3f %d\n' % (st, r, n, d, l))
    with open(out('enemies', 'names.txt'), 'w') as f:
        for w in range(1, 6):
            for k, t in enumerate(TYPES):
                f.write('%d %s %s\n' % (w, t, NAMES[w][k]))

# ---------------- Bosses ----------------
BF = 192
BOSS_NAMES = ['Queen Thornmother', 'Maestro Bongobrute', 'Neon Sentinel X', 'Glacius the Frost Wyrm', 'Umbra, Ember King']
BSTATES = [('idle', 4, 0.15, 1), ('attack', 4, 0.1, 0), ('telegraph', 4, 0.08, 1), ('vulnerable', 4, 0.12, 1), ('hit', 2, 0.07, 0), ('death', 6, 0.12, 0)]

def boss_frame(b, st, i, n):
    c = Canvas(BF, BF)
    ph = i / max(1, n) * 2 * math.pi
    s = math.sin(ph)
    cx, cy = 96, 104
    flash = st == 'hit'
    tele = st == 'telegraph'
    vul = st == 'vulnerable'
    def col(x):
        x = mix(x, (255, 255, 255), 0.65) if flash else x
        x = mix(x, (255, 60, 60), 0.25 * (1 + s)) if tele else x
        return x
    eyemode = 'dizzy' if vul else ('angry' if st in ('attack', 'telegraph') else 'normal')
    def beye(x, y, r):
        if eyemode == 'dizzy':
            c.ellipse(x, y, r, r, (255, 255, 255))
            c.arc(x, y, r * 0.6, r * 0.6, ph * 57, ph * 57 + 270, INK, 2)
        else:
            c.ellipse(x, y, r, r * (0.7 if eyemode == 'angry' else 1), (255, 250, 200))
            c.ellipse(x + 2, y + 1, r * 0.45, r * 0.5, INK)
            if eyemode == 'angry':
                c.line([(x - r, y - r), (x + r, y - r * 0.4)], INK, 3)
    if b == 1:  # giant carnivorous flower queen
        for k in range(6):
            a = k / 6 * 2 * math.pi + s * 0.1
            c.rot_ellipse(cx + math.cos(a) * 38, cy - 20 + math.sin(a) * 38, 30, 16, a, col((255, 100, 170) if k % 2 else (230, 70, 150)))
        c.rect(cx - 10, cy + 20, cx + 10, 186, col((70, 140, 60)), radius=6)
        c.rot_ellipse(cx - 34, cy + 50, 26, 10, -0.5 + s * 0.1, col((90, 170, 70)))
        c.rot_ellipse(cx + 34, cy + 56, 26, 10, 0.5 - s * 0.1, col((90, 170, 70)))
        c.ellipse(cx, cy - 20, 34, 34, col((255, 210, 90)))
        open_ = 14 if st == 'attack' else (24 if vul else 4)
        c.ellipse(cx, cy - 4, 20, open_, (110, 20, 40))
        for k in range(5):
            c.poly([(cx - 16 + k * 8, cy - 4 - open_ + 2), (cx - 12 + k * 8, cy - 4 - open_ + 2), (cx - 14 + k * 8, cy - open_ + 6)], (255, 255, 255))
        if vul:
            c.glow(cx, cy - 4, 18, (255, 255, 160), 0.9)
        beye(cx - 13, cy - 32, 8); beye(cx + 13, cy - 32, 8)
    elif b == 2:  # drum giant
        c.ellipse(cx, cy + 10, 62, 56, col((200, 70, 40)))
        c.ellipse(cx, cy - 34, 56, 14, col((250, 235, 210)))
        for k in range(-3, 4):
            c.line([(cx + k * 16, cy - 30), (cx + k * 18, cy + 60)], col((255, 210, 80)), 3)
        c.ellipse(cx - 70 - s * 6, cy + 10, 16, 16, col((255, 255, 255)))
        c.ellipse(cx + 70 + s * 6, cy + 10, 16, 16, col((255, 255, 255)))
        c.rect(cx - 48, 160, cx - 22, 184, col((90, 40, 20)), radius=6)
        c.rect(cx + 22, 160, cx + 48, 184, col((90, 40, 20)), radius=6)
        beye(cx - 20, cy, 11); beye(cx + 20, cy, 11)
        c.arc(cx, cy + 22, 22, 12, 10 if st != 'attack' else 190, 170 if st != 'attack' else 350, INK, 4)
        for k in range(3):
            c.ellipse(cx - 30 + k * 30, cy - 52 - abs(s) * 6, 6, 5, col((40, 30, 30)))
            c.line([(cx - 25 + k * 30, cy - 52 - abs(s) * 6), (cx - 25 + k * 30, cy - 70 - abs(s) * 6)], col((40, 30, 30)), 2)
    elif b == 3:  # neon sentinel drone
        c.glow(cx, cy, 80, (0, 200, 255), 0.35)
        c.poly([(cx - 70, cy), (cx - 30, cy - 46), (cx + 30, cy - 46), (cx + 70, cy), (cx + 30, cy + 46), (cx - 30, cy + 46)], col((40, 40, 70)))
        c.poly([(cx - 56, cy), (cx - 24, cy - 36), (cx + 24, cy - 36), (cx + 56, cy), (cx + 24, cy + 36), (cx - 24, cy + 36)], None, col((0, 240, 255)), 3)
        core = (255, 60, 200) if not vul else (255, 255, 120)
        r = 18 if not vul else 26
        c.glow(cx, cy, r * 2, core, 0.9)
        c.ellipse(cx, cy, r, r, col(core))
        c.ellipse(cx, cy, r * 0.5, r * 0.5, (255, 255, 255))
        for k in range(4):
            a = k * math.pi / 2 + ph * 0.25
            c.ellipse(cx + math.cos(a) * 80, cy + math.sin(a) * 60, 10, 10, col((0, 240, 255)))
        if not vul:
            c.rect(cx - 30, cy - 6, cx + 30, cy + 6, col((60, 60, 100)), radius=3)
    elif b == 4:  # frost wyrm head + coils
        for k in range(5):
            c.ellipse(cx - 60 + k * 22, cy + 50 + math.sin(ph + k) * 6, 22, 20, col((150, 200, 245) if k % 2 else (180, 220, 255)))
        c.ellipse(cx + 20, cy - 10, 50, 40, col((190, 230, 255)))
        c.poly([(cx + 50, cy - 30), (cx + 92, cy - 10), (cx + 88, cy + 14), (cx + 50, cy + 18)], col((170, 215, 250)))
        for k in range(4):
            c.poly([(cx - 10 + k * 14, cy - 44), (cx - 2 + k * 14, cy - 44), (cx - 6 + k * 14, cy - 70 - (k % 2) * 10)], col((235, 250, 255)))
        mouth = 12 if st == 'attack' else (4 if not vul else 2)
        c.rect(cx + 50, cy + 8, cx + 90, cy + 8 + mouth, (40, 60, 120))
        beye(cx + 30, cy - 16, 10)
        if vul:
            for k in range(3):
                c.star(cx + 10 + math.cos(ph + k * 2) * 26, cy - 50 + math.sin(ph + k * 2) * 8, 6, 2.5, (255, 255, 160))
    elif b == 5:  # Ember King
        c.glow(cx, cy, 90, (255, 80, 20), 0.4 + 0.1 * s)
        c.poly([(cx - 50, 186), (cx - 40, cy - 10), (cx + 40, cy - 10), (cx + 50, 186)], col((40, 16, 28)))
        c.poly([(cx - 70, cy + 10 + s * 4), (cx - 40, cy - 10), (cx - 30, cy + 40)], col((60, 20, 40)))
        c.poly([(cx + 70, cy + 10 - s * 4), (cx + 40, cy - 10), (cx + 30, cy + 40)], col((60, 20, 40)))
        c.ellipse(cx, cy - 34, 36, 34, col((70, 25, 45)))
        for k in range(5):
            c.poly([(cx - 30 + k * 15, cy - 60), (cx - 22 + k * 15, cy - 60), (cx - 26 + k * 15, cy - 92 - (k % 2) * 8 + s * 3)], col((255, 200, 60)))
        beye(cx - 12, cy - 34, 9); beye(cx + 14, cy - 34, 9)
        c.line([(cx - 40, cy + 30), (cx - 40, cy + 70)], (255, 120, 30), 3)
        c.line([(cx + 40, cy + 30), (cx + 40, cy + 70)], (255, 120, 30), 3)
        c.ellipse(cx, cy + 30, 14, 14, col((255, 140, 40)) if not vul else (255, 255, 160))
        if st == 'attack':
            c.glow(cx + 70, cy + 20, 24, (255, 150, 40), 1.0)
            c.ellipse(cx + 70, cy + 20, 12, 12, (255, 220, 120))
    img = outline_image(c.final(), INK, 2)
    if st == 'death':
        img = img.rotate(i * 8 * (1 if b % 2 else -1), resample=Image.BICUBIC)
        img.putalpha(img.split()[3].point(lambda v: int(v * (1 - i / n * 0.8))))
    return img

def build_bosses():
    for b in range(1, 6):
        rows = [[boss_frame(b, st, i, n) for i in range(n)] for st, n, _, _ in BSTATES]
        sheet(rows, BF, BF).save(out('bosses', 'boss%d.png' % b))
    with open(out('bosses', 'boss.anim'), 'w') as f:
        f.write('frame %d %d\n' % (BF, BF))
        for r, (st, n, d, l) in enumerate(BSTATES):
            f.write('%s %d %d %.3f %d\n' % (st, r, n, d, l))
    with open(out('bosses', 'names.txt'), 'w') as f:
        for i, nme in enumerate(BOSS_NAMES):
            f.write('%d %s\n' % (i + 1, nme))

if __name__ == '__main__':
    build_enemies()
    build_bosses()
