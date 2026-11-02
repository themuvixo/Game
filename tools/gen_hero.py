"""Lumo the Wisp-Keeper: original limbless hero, drawn procedurally as a multi-part puppet."""
import math
from gfx_common import *

FW = FH = 96
BODY = (255, 196, 150)
TUNIC = (122, 70, 220)
TUNIC_D = (82, 40, 170)
HOOD = (60, 130, 255)
HOOD_D = (36, 80, 200)
GLOVE = (255, 255, 255)
SHOE = (255, 138, 40)
SHOE_D = (200, 90, 20)
STAR = (255, 236, 120)
SCARF = (255, 80, 140)
INK = (34, 20, 48)

def draw_frame(p):
    c = Canvas(FW, FH)
    cx = 48 + p.get('dx', 0)
    cy = 52 + p.get('dy', 0)
    sq = p.get('squash', 1.0)  # >1 wide/short
    tilt = p.get('tilt', 0.0)
    scale = p.get('scale', 1.0)
    alpha = p.get('alpha', 1.0)
    rx, ry = 15 * sq * scale, 19 / sq * scale
    # back shoe / back hand
    def shoe(x, y, ang, back=False):
        col = darken(SHOE, 0.25) if back else SHOE
        c.rot_ellipse(cx + x * scale, cy + y * scale, 10 * scale, 6 * scale, ang, col)
        c.rot_ellipse(cx + x * scale - 2 * scale, cy + y * scale + 3 * scale, 9 * scale, 2.5 * scale, ang, SHOE_D)
    def hand(x, y, ang, back=False, fist=False):
        col = (220, 222, 235) if back else GLOVE
        if fist:
            c.ellipse(cx + x * scale, cy + y * scale, 7 * scale, 7 * scale, col)
            c.ellipse(cx + x * scale + 3 * scale, cy + y * scale - 2 * scale, 3 * scale, 3 * scale, lighten(col, 0.5))
        else:
            c.rot_ellipse(cx + x * scale, cy + y * scale, 7 * scale, 5.5 * scale, ang, col)
            c.rot_ellipse(cx + x * scale + math.cos(ang - 1.2) * 6 * scale, cy + y * scale + math.sin(ang - 1.2) * 6 * scale,
                          2.5 * scale, 4 * scale, ang, col)
        # cuff
        c.rot_ellipse(cx + x * scale - math.cos(ang) * 6 * scale, cy + y * scale - math.sin(ang) * 6 * scale,
                      3 * scale, 5 * scale, ang, SCARF if not back else darken(SCARF, 0.3))
    fb = p.get('foot_b', (-6, 26)); ff = p.get('foot_f', (8, 26))
    hb = p.get('hand_b', (-20, 4)); hf = p.get('hand_f', (20, 4))
    shoe(fb[0], fb[1], p.get('foot_b_ang', 0.0), back=True)
    if hb is not None:
        hand(hb[0], hb[1], p.get('hand_b_ang', 0.0), back=True)
    # body (tunic)
    c.rot_ellipse(cx, cy + 4 * scale, rx, ry, tilt, TUNIC)
    c.rot_ellipse(cx - 3 * scale, cy + 8 * scale, rx * 0.75, ry * 0.6, tilt, TUNIC_D)
    c.rot_ellipse(cx + 3 * scale, cy - 1 * scale, rx * 0.55, ry * 0.45, tilt, lighten(TUNIC, 0.2))
    # belt rune
    c.star(cx + math.sin(tilt) * 4, cy + 10 * scale, 4 * scale, 1.8 * scale, STAR, 4)
    # head
    hx = cx + math.sin(tilt) * 18 * scale + p.get('head_dx', 0)
    hy = cy - 20 * scale / (sq ** 0.5) + p.get('head_dy', 0)
    c.ellipse(hx, hy, 17 * scale, 15 * scale, BODY)
    c.ellipse(hx - 4 * scale, hy + 4 * scale, 12 * scale, 9 * scale, darken(BODY, 0.06))
    c.ellipse(hx + 2 * scale, hy - 4 * scale, 9 * scale, 6 * scale, lighten(BODY, 0.25))
    # hood with long floppy tip and glowing star
    sway = p.get('hood_sway', 0.0)
    tip = (hx - 26 * scale + sway * 10, hy - 22 * scale + abs(sway) * 4)
    hood_pts = [(hx - 18 * scale, hy - 2 * scale), (hx - 14 * scale, hy - 14 * scale), (hx - 2 * scale, hy - 19 * scale),
                (hx + 12 * scale, hy - 15 * scale), (hx + 18 * scale, hy - 4 * scale),
                (hx + 8 * scale, hy - 9 * scale), (hx - 6 * scale, hy - 10 * scale)]
    c.poly(hood_pts, HOOD)
    c.poly([(hx - 10 * scale, hy - 15 * scale), (hx + 2 * scale, hy - 18 * scale), tip], HOOD)
    c.poly([(hx - 8 * scale, hy - 14 * scale), (hx - 2 * scale, hy - 16 * scale), tip], HOOD_D)
    if p.get('propeller'):
        ph = p['propeller']
        for k in range(2):
            a = ph + k * math.pi
            c.rot_ellipse(hx + math.cos(a) * 18 * scale, hy - 24 * scale, 18 * scale * abs(math.cos(a)) + 3, 3.5 * scale, 0, (190, 220, 255, 200))
        c.ellipse(hx, hy - 22 * scale, 4 * scale, 4 * scale, HOOD_D)
    c.glow(tip[0], tip[1], 10 * scale, STAR, 0.8)
    c.star(tip[0], tip[1], 6 * scale, 2.6 * scale, STAR, 5, rot=-math.pi / 2 + sway)
    # face
    eye = p.get('eyes', 'open')
    ex = hx + 5 * scale
    ey = hy + 1 * scale
    look = p.get('look', (1.5, 0))
    for k, ox in enumerate((-6, 6)):
        x = ex + ox * scale
        if eye == 'closed':
            c.arc(x, ey + 1, 4 * scale, 3 * scale, 200, 340, INK, 1.6)
        elif eye == 'x':
            c.line([(x - 3, ey - 3), (x + 3, ey + 3)], INK, 1.8)
            c.line([(x - 3, ey + 3), (x + 3, ey - 3)], INK, 1.8)
        else:
            h = 7 if eye == 'wide' else 6
            c.ellipse(x, ey, 4.2 * scale, h * scale, (255, 255, 255))
            c.ellipse(x + look[0] * scale, ey + look[1] * scale + 1, 2.4 * scale, 3.4 * scale, INK)
            c.ellipse(x + look[0] * scale + 0.9, ey + look[1] * scale - 0.6, 0.9 * scale, 0.9 * scale, (255, 255, 255))
    # nose + mouth
    c.ellipse(ex + 1 * scale, ey + 6 * scale, 3.5 * scale, 2.6 * scale, (255, 150, 120))
    m = p.get('mouth', 'smile')
    if m == 'smile':
        c.arc(ex + 1 * scale, ey + 8 * scale, 5 * scale, 3.5 * scale, 20, 160, INK, 1.4)
    elif m == 'open':
        c.ellipse(ex + 1 * scale, ey + 10 * scale, 3.4 * scale, 2.8 * scale, (130, 30, 50))
    elif m == 'grit':
        c.line([(ex - 3, ey + 10), (ex + 5, ey + 10)], INK, 1.6)
    elif m == 'o':
        c.ellipse(ex + 1 * scale, ey + 10 * scale, 2.2 * scale, 2.6 * scale, (130, 30, 50))
    # cheek
    c.ellipse(hx - 6 * scale, hy + 6 * scale, 3 * scale, 2 * scale, (255, 140, 150, 160))
    # scarf
    c.rot_ellipse(cx + math.sin(tilt) * 10, cy - 8 * scale, 12 * scale, 4 * scale, tilt, SCARF)
    c.poly([(cx - 8 * scale, cy - 8 * scale), (cx - 22 * scale + sway * 6, cy - 4 * scale + p.get('scarf_lift', 0)),
            (cx - 20 * scale + sway * 6, cy + 1 * scale + p.get('scarf_lift', 0)), (cx - 6 * scale, cy - 4 * scale)], darken(SCARF, 0.12))
    # front parts
    shoe(ff[0], ff[1], p.get('foot_f_ang', 0.0))
    if hf is not None:
        hand(hf[0], hf[1], p.get('hand_f_ang', 0.0), fist=p.get('fist', False))
    for sp in p.get('sparkles', []):
        c.glow(sp[0], sp[1], sp[2] * 2, STAR, 0.7)
        c.star(sp[0], sp[1], sp[2], sp[2] * 0.35, (255, 255, 230), 4)
    img = outline_image(c.final(), INK, 1)
    if alpha < 1.0:
        a = img.split()[3].point(lambda v: int(v * alpha))
        img.putalpha(a)
    return img

def anim_idle(n=6):
    fr = []
    for i in range(n):
        t = i / n * 2 * math.pi
        b = math.sin(t)
        fr.append(dict(dy=b * 1.5, squash=1 + b * 0.03, hand_b=(-21, 4 - b * 3), hand_f=(21, 4 - b * 3),
                       hood_sway=math.sin(t) * 0.15, eyes='closed' if i == 4 else 'open'))
    return fr

def anim_walk(n=8):
    fr = []
    for i in range(n):
        t = i / n * 2 * math.pi
        s = math.sin(t)
        fr.append(dict(dy=-abs(math.cos(t)) * 2, foot_b=(-2 - s * 10, 26 - max(0, s) * 4), foot_f=(4 + s * 10, 26 - max(0, -s) * 4),
                       hand_b=(-18 + s * 6, 6), hand_f=(18 - s * 6, 6), hood_sway=-0.2 + s * 0.1, tilt=0.05))
    return fr

def anim_run(n=8):
    fr = []
    for i in range(n):
        t = i / n * 2 * math.pi
        s = math.sin(t)
        fr.append(dict(dy=-abs(math.cos(t)) * 4, foot_b=(-4 - s * 15, 24 - max(0, s) * 7), foot_f=(6 + s * 15, 24 - max(0, -s) * 7),
                       foot_b_ang=-s * 0.4, foot_f_ang=s * 0.4,
                       hand_b=(-16 + s * 12, 2 + abs(s) * 3), hand_f=(16 - s * 12, 2 + abs(s) * 3),
                       hood_sway=-0.6 + s * 0.15, tilt=0.18, mouth='open', scarf_lift=-3))
    return fr

def anim_jump():
    return [dict(squash=0.85, dy=-2, hand_b=(-20, -10), hand_f=(22, -12), foot_b=(-6, 30), foot_f=(6, 30), hood_sway=0.4, mouth='open'),
            dict(squash=0.9, dy=-3, hand_b=(-22, -14), hand_f=(24, -16), foot_b=(-8, 24), foot_f=(8, 22), hood_sway=0.5, mouth='open'),
            dict(squash=0.95, dy=-2, hand_b=(-22, -10), hand_f=(23, -10), foot_b=(-8, 22), foot_f=(9, 20), hood_sway=0.45, mouth='smile')]

def anim_fall():
    fr = []
    for i in range(3):
        w = math.sin(i * 2.1) * 4
        fr.append(dict(squash=1.02, hand_b=(-24, -16 + w), hand_f=(24, -16 - w), foot_b=(-8, 28), foot_f=(8, 30),
                       hood_sway=0.7 + w * 0.03, eyes='wide', mouth='o', scarf_lift=-8, look=(0.5, -1.5)))
    return fr

def anim_land():
    return [dict(squash=1.3, dy=6, hand_b=(-24, 14), hand_f=(24, 14), foot_b=(-10, 26), foot_f=(10, 26), eyes='closed', mouth='grit'),
            dict(squash=1.15, dy=3, hand_b=(-22, 10), hand_f=(22, 10), foot_b=(-8, 26), foot_f=(9, 26)),
            dict(squash=1.04, dy=1, hand_b=(-21, 6), hand_f=(21, 6))]

def anim_attack():
    return [dict(tilt=-0.15, hand_f=(-4, 0), hand_f_ang=math.pi, hand_b=(-22, 6), mouth='grit', fist=True),
            dict(tilt=-0.2, hand_f=(-12, -2), hand_b=(-20, 8), mouth='grit', fist=True),
            dict(tilt=0.2, hand_f=(34, 0), hand_b=(-18, 4), mouth='open', fist=True, dx=2),
            dict(tilt=0.18, hand_f=None, hand_b=(-18, 4), mouth='open', dx=2),
            dict(tilt=0.06, hand_f=None, hand_b=(-20, 4), mouth='smile')]

def anim_hurt():
    return [dict(tilt=-0.35, dx=-4, eyes='x', mouth='o', hand_b=(-24, -8), hand_f=(18, -10), squash=0.92),
            dict(tilt=-0.3, dx=-3, eyes='x', mouth='o', hand_b=(-24, -4), hand_f=(18, -6)),
            dict(tilt=-0.15, dx=-1, eyes='closed', mouth='grit', hand_b=(-22, 2), hand_f=(20, 2))]

def anim_death(n=8):
    fr = []
    for i in range(n):
        t = i / (n - 1)
        fr.append(dict(tilt=t * 6.0, dy=-t * 10, scale=1 - t * 0.55, eyes='x', mouth='o', alpha=1 - t * 0.6,
                       hand_b=(-24 - t * 10, -10), hand_f=(24 + t * 10, -12), foot_b=(-12 - t * 6, 28), foot_f=(12 + t * 6, 28),
                       sparkles=[(48 + math.cos(i + k * 2.1) * (10 + t * 30), 50 + math.sin(i + k * 2.1) * (10 + t * 30), 3 + t * 2) for k in range(3)] if i > 1 else []))
    return fr

def anim_glide(n=6):
    fr = []
    for i in range(n):
        ph = i / n * math.pi
        fr.append(dict(propeller=ph, hand_b=(-22, -6), hand_f=(22, -6), foot_b=(-6, 30), foot_f=(6, 31), hood_sway=0.0,
                       dy=math.sin(i / n * 2 * math.pi) * 1.2, mouth='smile', look=(1, 1)))
    return fr

def anim_swim(n=6):
    fr = []
    for i in range(n):
        t = i / n * 2 * math.pi
        s = math.sin(t)
        fr.append(dict(tilt=0.5, hand_b=(-10 + s * 10, -8 - s * 6), hand_f=(24 - s * 6, -2 + s * 6),
                       foot_b=(-20 - s * 4, 20 + s * 4), foot_f=(-16 + s * 4, 26 - s * 4), hood_sway=-0.8, mouth='o'))
    return fr

ANIMS = [('idle', anim_idle(), 0.14, 1), ('walk', anim_walk(), 0.09, 1), ('run', anim_run(), 0.06, 1),
         ('jump', anim_jump(), 0.08, 0), ('fall', anim_fall(), 0.1, 1), ('land', anim_land(), 0.05, 0),
         ('attack', anim_attack(), 0.05, 0), ('hurt', anim_hurt(), 0.08, 0), ('death', anim_death(), 0.1, 0),
         ('glide', anim_glide(), 0.04, 1), ('swim', anim_swim(), 0.1, 1)]

def build():
    rows = [[draw_frame(p) for p in a[1]] for a in ANIMS]
    sheet(rows, FW, FH).save(out('characters', 'lumo.png'))
    with open(out('characters', 'lumo.anim'), 'w') as f:
        f.write('# name row frames frameDuration loop  (frame size %dx%d)\n' % (FW, FH))
        f.write('frame %d %d\n' % (FW, FH))
        for r, a in enumerate(ANIMS):
            f.write('%s %d %d %.3f %d\n' % (a[0], r, len(a[1]), a[2], a[3]))
    # portrait for menus/extras
    big = draw_frame(dict(eyes='open', mouth='smile', hand_b=(-22, -4), hand_f=(24, -14), hand_f_ang=-0.8))
    big.resize((288, 288), Image.LANCZOS).save(out('characters', 'lumo_portrait.png'))

if __name__ == '__main__':
    build()
