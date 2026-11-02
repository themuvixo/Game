"""Collectibles, gameplay objects, projectiles, world map, UI and logo (original, CC0)."""
import math
from gfx_common import *
from PIL import ImageFont

INK = (30, 16, 40)
FONT_BOLD = os.path.join(ROOT, 'fonts', 'NotoSansDisplay-Bold.ttf')

def collectibles():
    # Ting: spinning sparkle orb (8 frames, 32px)
    fr = []
    for i in range(8):
        c = Canvas(32, 32); sx = abs(math.cos(i / 8 * math.pi))
        c.glow(16, 16, 14, (255, 220, 120), 0.7)
        c.ellipse(16, 16, 9 * max(0.2, sx), 9, (255, 200, 60)); c.ellipse(16, 16, 6 * max(0.15, sx), 6, (255, 240, 150))
        c.star(16, 16, 5 * max(0.3, sx), 2, (255, 255, 255), 4)
        fr.append(c.final())
    sheet([fr], 32, 32).save(out('collectibles', 'ting.png'))
    # Gem (6 frames, 40px)
    fr = []
    for i in range(6):
        c = Canvas(40, 40); sx = 0.55 + 0.45 * abs(math.cos(i / 6 * math.pi))
        c.glow(20, 20, 18, (120, 220, 255), 0.6)
        pts = [(20, 4), (20 + 14 * sx, 14), (20, 36), (20 - 14 * sx, 14)]
        c.poly(pts, (80, 160, 255), (20, 40, 120), 1.2)
        c.poly([(20, 4), (20 + 14 * sx, 14), (20, 14)], (170, 220, 255)); c.poly([(20, 14), (20 - 14 * sx, 14), (20, 36)], (50, 110, 230))
        fr.append(c.final())
    sheet([fr], 40, 40).save(out('collectibles', 'gem.png'))
    # Key (4 frames, 40px)
    fr = []
    for i in range(4):
        c = Canvas(40, 40); c.glow(20, 20, 18, (255, 230, 100), 0.4 + 0.2 * math.sin(i))
        c.ellipse(13, 20, 8, 8, (255, 210, 60), INK, 1.2); c.ellipse(13, 20, 3.5, 3.5, (0, 0, 0, 0))
        c.rect(19, 18, 36, 22, (255, 210, 60)); c.rect(30, 22, 33, 28, (255, 210, 60)); c.rect(25, 22, 28, 26, (255, 210, 60))
        fr.append(outline_image(c.final(), INK, 1))
    sheet([fr], 40, 40).save(out('collectibles', 'key.png'))
    # Relics, one per world (48px)
    fr = []
    for w in range(1, 6):
        c = Canvas(48, 48)
        if w == 1:
            c.glow(24, 24, 20, (255, 200, 255), 0.6)
            for k in range(5):
                a = k / 5 * 2 * math.pi
                c.rot_ellipse(24 + math.cos(a) * 9, 24 + math.sin(a) * 9, 9, 5, a, (255, 170, 230))
            c.ellipse(24, 24, 6, 6, (255, 250, 200))
        elif w == 2:
            c.glow(24, 24, 20, (255, 230, 120), 0.6); c.ellipse(18, 34, 8, 6, (255, 210, 60)); c.rect(24, 8, 28, 34, (255, 210, 60)); c.poly([(28, 8), (40, 14), (28, 18)], (255, 210, 60))
        elif w == 3:
            c.glow(24, 24, 20, (0, 255, 230), 0.6); c.rect(10, 10, 38, 38, (30, 40, 70), (0, 255, 230), 2, radius=3)
            for k in range(4):
                c.rect(12 + k * 7, 4, 14 + k * 7, 10, (200, 200, 220)); c.rect(12 + k * 7, 38, 14 + k * 7, 44, (200, 200, 220))
            c.rect(18, 18, 30, 30, (255, 60, 200))
        elif w == 4:
            c.glow(24, 24, 20, (200, 245, 255), 0.6); c.poly([(24, 4), (36, 20), (24, 44), (12, 20)], (190, 235, 255), (90, 150, 220), 1.5)
            c.line([(24, 4), (24, 44)], (255, 255, 255), 1)
        elif w == 5:
            c.glow(24, 24, 20, (255, 120, 40), 0.7)
            c.ellipse(17, 20, 9, 9, (230, 40, 50)); c.ellipse(31, 20, 9, 9, (230, 40, 50)); c.poly([(9, 23), (39, 23), (24, 42)], (230, 40, 50))
            c.ellipse(18, 17, 3, 3, (255, 200, 200))
        fr.append(outline_image(c.final(), INK, 1))
    sheet([fr], 48, 48).save(out('collectibles', 'relics.png'))
    # Lumling (freed creature, 6 frames 40px) — original Electoon-equivalent
    fr = []
    for i in range(6):
        c = Canvas(40, 40); ph = i / 6 * 2 * math.pi; fl = math.sin(ph * 2) * 5
        c.glow(20, 20, 18, (255, 240, 160), 0.6)
        c.rot_ellipse(10, 18 - fl, 7, 3, -0.6, (255, 255, 255, 200)); c.rot_ellipse(30, 18 - fl, 7, 3, 0.6, (255, 255, 255, 200))
        c.ellipse(20, 22, 9, 9, (255, 230, 90)); c.ellipse(17, 21, 2, 2.6, INK); c.ellipse(23, 21, 2, 2.6, INK)
        c.arc(20, 24, 3, 2, 20, 160, INK, 1); c.star(20, 10, 4, 1.6, (255, 255, 255), 4)
        fr.append(c.final())
    sheet([fr], 40, 40).save(out('collectibles', 'lumling.png'))
    # heart pickup
    c = Canvas(32, 32); c.glow(16, 16, 14, (255, 100, 140), 0.5)
    c.ellipse(11, 13, 6, 6, (255, 70, 110)); c.ellipse(21, 13, 6, 6, (255, 70, 110)); c.poly([(5, 15), (27, 15), (16, 28)], (255, 70, 110))
    c.ellipse(10, 11, 2, 2, (255, 220, 230)); outline_image(c.final(), INK, 1).save(out('collectibles', 'heart.png'))
    # upgrade orb (double jump)
    fr = []
    for i in range(6):
        c = Canvas(48, 48); c.glow(24, 24, 22, (180, 140, 255), 0.6 + 0.2 * math.sin(i))
        c.ellipse(24, 24, 13, 13, (150, 110, 255)); c.poly([(24, 12), (32, 24), (27, 24), (27, 34), (21, 34), (21, 24), (16, 24)], (255, 255, 255))
        fr.append(c.final())
    sheet([fr], 48, 48).save(out('collectibles', 'upgrade.png'))

def objects():
    # checkpoint (2 states x 4 frames 48x96)
    rows = []
    for on in (0, 1):
        r = []
        for i in range(4):
            c = Canvas(48, 96); c.rect(8, 10, 13, 96, (200, 190, 210), INK, 1)
            wave = math.sin(i / 4 * 2 * math.pi) * 4
            col = (120, 120, 140) if not on else (255, 120, 210)
            c.poly([(13, 12), (44, 18 + wave), (40, 30 + wave), (13, 36)], col, INK, 1)
            if on:
                c.glow(10, 10, 12, (255, 240, 160), 0.9); c.star(10, 10, 6, 2.5, (255, 240, 160))
            else:
                c.ellipse(10, 10, 4, 4, (150, 150, 160))
            r.append(c.final())
        rows.append(r)
    sheet(rows, 48, 96).save(out('objects', 'checkpoint.png'))
    # exit gate portal (6 frames, 96x144)
    fr = []
    for i in range(6):
        c = Canvas(96, 144); ph = i / 6 * 2 * math.pi
        c.rect(6, 24, 22, 144, (180, 160, 200), INK, 1.5); c.rect(74, 24, 90, 144, (180, 160, 200), INK, 1.5)
        c.rect(0, 10, 96, 28, (210, 190, 230), INK, 1.5, radius=6)
        c.glow(48, 84, 40, (160, 255, 230), 0.6)
        for k in range(5):
            r = 30 - k * 6
            c.rot_ellipse(48, 84, r, r * 1.5, ph + k, (120 + k * 25, 220, 255, 140))
        c.star(48, 19, 7, 3, (255, 240, 130))
        fr.append(c.final())
    sheet([fr], 96, 144).save(out('objects', 'exit_gate.png'))
    # cage (2 frames: intact, broken; 64px)
    fr = []
    for broken in (0, 1):
        c = Canvas(64, 64)
        c.rect(6, 54, 58, 62, (100, 70, 60), INK, 1.5, radius=2); c.ellipse(32, 10, 26, 8, (100, 70, 60), INK, 1.5)
        if not broken:
            c.ellipse(32, 34, 10, 10, (255, 230, 90)); c.ellipse(29, 32, 2, 2.6, INK); c.ellipse(35, 32, 2, 2.6, INK)
            for k in range(6):
                c.line([(10 + k * 9, 12), (10 + k * 9, 54)], (170, 170, 190), 2.5)
        else:
            for k in range(6):
                c.line([(10 + k * 9, 54), (10 + k * 9 + (k - 3) * 4, 44)], (170, 170, 190), 2.5)
        fr.append(outline_image(c.final(), INK, 1))
    sheet([fr], 64, 64).save(out('objects', 'cage.png'))
    # spiked ball and chain link
    c = Canvas(64, 64)
    for k in range(10):
        a = k / 10 * 2 * math.pi
        c.poly([(32 + math.cos(a - 0.25) * 18, 32 + math.sin(a - 0.25) * 18), (32 + math.cos(a + 0.25) * 18, 32 + math.sin(a + 0.25) * 18), (32 + math.cos(a) * 30, 32 + math.sin(a) * 30)], (200, 200, 215), INK, 1)
    c.ellipse(32, 32, 20, 20, (110, 110, 130), INK, 1.5); c.ellipse(26, 26, 6, 5, (180, 180, 200))
    c.final().save(out('objects', 'spikeball.png'))
    c = Canvas(16, 16); c.ellipse(8, 8, 6, 6, None, (160, 160, 180), 2.5); c.final().save(out('objects', 'chain.png'))
    # Fist projectile (4 frames, 32px)
    fr = []
    for i in range(4):
        c = Canvas(32, 32); a = i / 4 * 0.6 - 0.3
        c.glow(16, 16, 14, (255, 240, 180), 0.5)
        c.rot_ellipse(18, 16, 9, 8, a, (255, 255, 255)); c.rot_ellipse(22, 12, 4, 3, a, (240, 240, 250)); c.rot_ellipse(8, 16, 4, 6, a, (255, 80, 140))
        fr.append(outline_image(c.final(), INK, 1))
    sheet([fr], 32, 32).save(out('objects', 'fist.png'))
    # projectiles sheet: seed, note, laser orb, icicle, fireball, shadow orb (6 x 32px)
    fr = []
    for k in range(6):
        c = Canvas(32, 32)
        if k == 0:
            c.ellipse(16, 16, 7, 9, (140, 90, 50)); c.ellipse(14, 12, 2, 3, (200, 150, 100))
        elif k == 1:
            c.ellipse(12, 22, 6, 5, (40, 20, 20)); c.rect(16, 4, 19, 22, (40, 20, 20)); c.poly([(19, 4), (28, 9), (19, 12)], (40, 20, 20))
        elif k == 2:
            c.glow(16, 16, 15, (255, 60, 220), 1.0); c.ellipse(16, 16, 6, 6, (255, 220, 255))
        elif k == 3:
            c.poly([(10, 2), (22, 2), (16, 30)], (200, 240, 255), (90, 150, 220), 1)
        elif k == 4:
            c.glow(16, 16, 15, (255, 120, 20), 1.0); c.ellipse(16, 16, 8, 8, (255, 200, 60)); c.ellipse(16, 16, 4, 4, (255, 255, 200))
        elif k == 5:
            c.glow(16, 16, 15, (150, 40, 200), 1.0); c.ellipse(16, 16, 8, 8, (60, 10, 80)); c.ellipse(16, 16, 3, 3, (220, 120, 255))
        fr.append(c.final())
    sheet([fr], 32, 32).save(out('objects', 'projectiles.png'))
    # particle textures: soft dot, star, dust puff, leaf/snow
    c = Canvas(32, 32); c.glow(16, 16, 15, (255, 255, 255), 1.0); c.final().save(out('objects', 'particle_soft.png'))
    c = Canvas(32, 32); c.star(16, 16, 14, 4, (255, 255, 255), 4); c.final().save(out('objects', 'particle_star.png'))
    c = Canvas(32, 32)
    for k in range(4):
        c.ellipse(10 + k * 4, 18 - (k % 2) * 4, 8, 7, (255, 255, 255, 200))
    c.final().save(out('objects', 'particle_dust.png'))
    # shockwave / telegraph marker
    c = Canvas(64, 16); c.ellipse(32, 8, 30, 6, (255, 60, 60, 160)); c.ellipse(32, 8, 18, 3, (255, 200, 200, 200)); c.final().save(out('objects', 'telegraph.png'))
    c = Canvas(96, 48)
    for k in range(3):
        c.arc(48, 48, 46 - k * 10, 40 - k * 9, 180, 360, (255, 255, 255, 220 - k * 50), 4)
    c.final().save(out('objects', 'shockwave.png'))
    # wind streak
    c = Canvas(64, 8); c.line([(2, 4), (62, 4)], (255, 255, 255, 160), 2); c.final().save(out('objects', 'wind.png'))
    # boss laser beam segment
    c = Canvas(32, 32); c.rect(0, 8, 32, 24, (255, 60, 220, 160)); c.rect(0, 13, 32, 19, (255, 230, 255)); c.final().save(out('objects', 'laser.png'))

def worldmap():
    W, H = 2560, 1600
    base = vgradient(W, H, [(0, (40, 110, 190)), (1, (20, 60, 140))])
    q = 4
    n = noise_2d(W // q, H // q, 30, 5, 5)
    n2 = noise_2d(W // q, H // q, 50, 9, 3)
    yy, xx = np.mgrid[0:H // q, 0:W // q]
    cx, cy = W / (2 * q), H / (2 * q)
    dist = np.sqrt(((xx - cx) / (W / (2 * q) * 0.9)) ** 2 + ((yy - cy) / (H / (2 * q) * 0.86)) ** 2)
    land = (1.05 - dist + (n - 0.5) * 0.4) > 0.2
    centers = [(620, 1060), (820, 520), (1520, 400), (2030, 820), (1480, 1160)]
    wx = xx + (n2 - 0.5) * 180 / q * 4
    wy = yy + (n - 0.5) * 180 / q * 4
    dd = np.stack([np.hypot(wx - c[0] / q, wy - c[1] / q) for c in centers])
    region = np.argmin(dd, axis=0)
    cols = [(160, 110, 210), (240, 160, 80), (80, 70, 130), (225, 238, 252), (110, 55, 45)]
    small = np.zeros((H // q, W // q, 4), dtype=np.uint8)
    for r in range(5):
        m = land & (region == r)
        shade = (0.82 + 0.32 * n2)[m]
        for k in range(3):
            small[:, :, k][m] = np.clip(cols[r][k] * shade, 0, 255)
        small[:, :, 3][m] = 255
    inner = land.copy()
    for s in (3, -3):
        inner &= np.roll(land, s, 0) & np.roll(land, s, 1)
    small[land & ~inner] = (240, 220, 160, 255)
    li = Image.fromarray(small, 'RGBA').resize((W, H), Image.BICUBIC).filter(ImageFilter.GaussianBlur(3))
    img = base.copy(); img.alpha_composite(li)
    c = Canvas(W, H); c.img = img.resize((W * SS // 2 * 2 // 2, H * SS // 2 * 2 // 2)) if False else None
    d = ImageDraw.Draw(img); rng = random.Random(3)
    for _ in range(320):
        x, y = rng.randrange(W), rng.randrange(H)
        if not land[min(H // q - 1, y // q), min(W // q - 1, x // q)]:
            d.arc([x, y, x + 24, y + 8], 200, 340, fill=(160, 210, 255), width=2)
    # painted landmarks: snowy peaks (W4), volcano (W5), neon towers (W3), drums (W2), giant mushrooms (W1)
    for k in range(6):
        x, y = 1950 + (k % 3) * 120 - 60, 600 + (k // 3) * 120
        d.polygon([(x - 70, y + 60), (x + 70, y + 60), (x, y - 70)], fill=(150, 170, 205))
        d.polygon([(x - 26, y - 20), (x + 26, y - 20), (x, y - 70)], fill=(255, 255, 255))
    x, y = 1500, 1080
    d.polygon([(x - 200, y + 120), (x + 200, y + 120), (x + 50, y - 110), (x - 50, y - 110)], fill=(70, 30, 30))
    d.polygon([(x - 50, y - 110), (x + 50, y - 110), (x + 30, y - 80), (x - 30, y - 80)], fill=(255, 120, 30))
    d.line([(x + 10, y - 90), (x + 40, y), (x + 20, y + 110)], fill=(255, 110, 30), width=10)
    for k in range(7):
        bx = 1330 + k * 55; bh = 60 + (k * 37) % 90
        d.rectangle([bx, 360 - bh, bx + 40, 380], fill=(35, 30, 60), outline=(0, 240, 255) if k % 2 else (255, 60, 200), width=3)
    for k in range(4):
        bx, by = 700 + k * 90, 420 + (k % 2) * 60
        d.rectangle([bx - 30, by - 20, bx + 30, by + 30], fill=(200, 60, 40), outline=(90, 20, 10), width=3)
        d.ellipse([bx - 30, by - 30, bx + 30, by - 10], fill=(250, 235, 210), outline=(90, 20, 10), width=2)
    for k in range(5):
        bx, by = 440 + k * 70, 1150 + (k % 2) * 50
        d.rectangle([bx - 6, by - 10, bx + 6, by + 30], fill=(250, 235, 250))
        d.ellipse([bx - 34, by - 34, bx + 34, by - 4], fill=(255, 140, 210), outline=(120, 40, 100), width=2)
    img.save(out('worldmap', 'map.png'))
    # node layout data (35 stages + secret branches), guaranteed on land
    def on_land(px, py):
        return land[int(np.clip(py // q, 0, H // q - 1)), int(np.clip(px // q, 0, W // q - 1))] and inner[int(np.clip(py // q, 0, H // q - 1)), int(np.clip(px // q, 0, W // q - 1))]
    # hand-placed stage markers (map pixels): normal 1-5, secret (6), boss (7)
    P = {1: [(180, 470), (260, 520), (380, 560), (440, 480), (380, 410), (480, 620), (260, 340)],
         2: [(370, 330), (460, 300), (520, 250), (470, 180), (560, 170), (290, 250), (610, 240)],
         3: [(670, 260), (740, 320), (820, 300), (780, 230), (710, 190), (750, 350), (760, 165)],
         4: [(890, 250), (960, 230), (1040, 420), (1090, 480), (1010, 520), (1150, 370), (890, 480)],
         5: [(850, 600), (790, 650), (690, 640), (620, 580), (600, 480), (500, 670), (740, 560)]}
    lines = []
    for w in range(1, 6):
        for s, (px, py) in enumerate(P[w]):
            kind = 'normal' if s < 5 else ('secret' if s == 5 else 'boss')
            mx, my = px * 2.0, py * 2.0
            moved = 0
            while not on_land(mx, my):
                mx += (W / 2 - mx) * 0.04; my += (H / 2 - my) * 0.04; moved += 1
            if moved:
                print('nudged', w, s + 1, moved)
            lines.append('%d %d %s %d %d' % (w, s + 1, kind, mx, my))
    with open(out('worldmap', 'nodes.txt'), 'w') as f:
        f.write('# world stage kind x y   (stage 6 = secret, stage 7 = boss)\n')
        f.write('\n'.join(lines) + '\n')
    # node markers (6 kinds x 64px): locked, open, completed, boss, secret, challenge
    fr = []
    for k in range(6):
        c = Canvas(64, 64)
        col = [(110, 110, 120), (255, 210, 80), (110, 230, 120), (230, 60, 60), (190, 120, 255), (60, 200, 255)][k]
        c.ellipse(32, 40, 26, 14, darken(col, 0.45)); c.ellipse(32, 34, 26, 14, col, INK, 2); c.ellipse(32, 31, 18, 8, lighten(col, 0.3))
        if k == 0:
            c.rect(25, 22, 39, 34, (70, 70, 80), INK, 1, radius=2); c.arc(32, 22, 5, 6, 180, 360, INK, 2)
        if k == 3:
            c.ellipse(32, 26, 9, 8, (255, 255, 255)); c.ellipse(28, 26, 2.5, 2.5, INK); c.ellipse(36, 26, 2.5, 2.5, INK)
        if k == 4:
            c.star(32, 28, 9, 4, (255, 255, 255))
        if k == 5:
            c.ellipse(32, 28, 8, 8, None, (255, 255, 255), 2); c.line([(32, 28), (32, 22)], (255, 255, 255), 2)
        fr.append(c.final())
    sheet([fr], 64, 64).save(out('worldmap', 'nodes.png'))
    # animated flag (6 frames 48x72)
    fr = []
    for i in range(6):
        c = Canvas(48, 72); c.rect(6, 6, 10, 72, (220, 210, 200), INK, 1)
        pts = [(10, 8)] + [(10 + k * 6, 8 + math.sin(i / 6 * 2 * math.pi + k * 0.8) * 3) for k in range(1, 7)]
        pts += [(10 + k * 6, 30 + math.sin(i / 6 * 2 * math.pi + k * 0.8) * 3) for k in range(6, 0, -1)] + [(10, 30)]
        c.poly(pts, (255, 90, 160), INK, 1); c.star(24, 19, 5, 2, (255, 240, 140))
        fr.append(c.final())
    sheet([fr], 48, 72).save(out('worldmap', 'flag.png'))
    # clouds
    for k in range(3):
        c = Canvas(320, 160); rng = random.Random(k)
        for j in range(8):
            c.ellipse(60 + j * 28 + rng.randint(-10, 10), 90 + rng.randint(-25, 15), rng.randint(36, 60), rng.randint(26, 40), (255, 255, 255, 230))
        c.final().filter(ImageFilter.GaussianBlur(2)).save(out('worldmap', 'cloud%d.png' % k))
    # map tree (pivot at bottom) and waterfall strip
    for k, (leaf, trunk) in enumerate([((240, 120, 210), (110, 70, 60)), ((80, 160, 80), (90, 60, 40)), ((240, 250, 255), (100, 80, 70))]):
        c = Canvas(64, 96); c.rect(28, 50, 36, 96, trunk, INK, 1)
        if k == 2:
            c.poly([(8, 76), (56, 76), (32, 6)], (60, 120, 90), INK, 1); c.poly([(16, 50), (48, 50), (32, 10)], leaf)
        else:
            c.ellipse(32, 40, 26, 24, leaf, INK, 1); c.ellipse(24, 32, 12, 10, lighten(leaf, 0.3))
        c.final().save(out('worldmap', 'tree%d.png' % k))
    fr = []
    for i in range(4):
        c = Canvas(48, 192); c.rect(4, 0, 44, 192, (120, 190, 255, 200))
        for j in range(10):
            y = (j * 24 + i * 6) % 192
            c.line([(10 + (j % 3) * 10, y), (10 + (j % 3) * 10, y + 14)], (240, 250, 255, 220), 2)
        c.ellipse(24, 188, 26, 8, (255, 255, 255, 230))
        fr.append(c.final())
    sheet([fr], 48, 192).save(out('worldmap', 'waterfall.png'))

def ui():
    # icons sheet 48px: 0 heart full, 1 heart empty, 2 ting, 3 life, 4 star full, 5 star empty, 6 lock, 7 check,
    # 8 gem, 9 cage, 10 key, 11 trophy, 12 clock, 13 skull, 14 flag, 15 selector arrow
    fr = []
    for k in range(16):
        c = Canvas(48, 48)
        if k in (0, 1):
            col = (255, 70, 110) if k == 0 else (90, 60, 80)
            c.ellipse(17, 19, 10, 10, col, INK, 2); c.ellipse(31, 19, 10, 10, col, INK, 2); c.poly([(8, 22), (40, 22), (24, 42)], col)
            c.ellipse(17, 19, 9, 9, col); c.ellipse(31, 19, 9, 9, col)
            if k == 0:
                c.ellipse(15, 16, 3, 3, (255, 220, 230))
        elif k == 2:
            c.glow(24, 24, 20, (255, 220, 120), 0.6); c.ellipse(24, 24, 12, 12, (255, 200, 60), INK, 1.5); c.ellipse(24, 24, 8, 8, (255, 240, 150))
        elif k == 3:
            c.ellipse(24, 26, 16, 14, (255, 196, 150), INK, 1.5); c.poly([(8, 22), (40, 22), (14, 4)], (60, 130, 255), INK, 1.5)
            c.ellipse(20, 26, 2.5, 3.5, INK); c.ellipse(30, 26, 2.5, 3.5, INK); c.star(13, 5, 5, 2, (255, 236, 120))
        elif k in (4, 5):
            c.star(24, 25, 21, 9, (255, 215, 60) if k == 4 else (80, 70, 100), 5, outline=INK, width=2)
        elif k == 6:
            c.rect(12, 22, 36, 42, (200, 180, 120), INK, 2, radius=3); c.arc(24, 22, 8, 10, 180, 360, INK, 3)
        elif k == 7:
            c.line([(10, 26), (20, 36), (38, 12)], (110, 230, 120), 6)
        elif k == 8:
            c.poly([(24, 6), (38, 18), (24, 42), (10, 18)], (80, 160, 255), INK, 1.5)
        elif k == 9:
            c.rect(10, 8, 38, 40, None, (170, 170, 190), 3, radius=4); c.ellipse(24, 24, 7, 7, (255, 230, 90))
        elif k == 10:
            c.ellipse(16, 24, 8, 8, (255, 210, 60), INK, 1.5); c.rect(22, 22, 40, 26, (255, 210, 60))
        elif k == 11:
            c.rect(14, 8, 34, 26, (255, 210, 60), INK, 1.5, radius=6); c.rect(21, 26, 27, 36, (255, 210, 60)); c.rect(14, 36, 34, 42, (200, 150, 40), INK, 1)
        elif k == 12:
            c.ellipse(24, 24, 17, 17, (240, 240, 250), INK, 2); c.line([(24, 24), (24, 13)], INK, 2.5); c.line([(24, 24), (32, 28)], INK, 2.5)
        elif k == 13:
            c.ellipse(24, 20, 15, 14, (240, 240, 250), INK, 1.5); c.rect(16, 30, 32, 40, (240, 240, 250), INK, 1.5)
            c.ellipse(18, 20, 4, 4, INK); c.ellipse(30, 20, 4, 4, INK)
        elif k == 14:
            c.rect(12, 6, 15, 44, (220, 210, 200)); c.poly([(15, 8), (38, 14), (15, 24)], (255, 90, 160), INK, 1)
        elif k == 15:
            c.poly([(8, 8), (40, 24), (8, 40), (16, 24)], (255, 236, 120), INK, 2)
        fr.append(c.final())
    sheet([fr[:8], fr[8:]], 48, 48).save(out('ui', 'icons.png'))
    # panel (9-slice source 96x96, 24px borders)
    c = Canvas(96, 96); c.rect(2, 2, 94, 94, (30, 20, 60, 225), (255, 220, 140), 3, radius=18)
    c.rect(8, 8, 88, 88, None, (140, 100, 220, 180), 1.5, radius=13); c.final().save(out('ui', 'panel.png'))
    # button states (normal, hover) 320x64
    fr = []
    for k in range(2):
        c = Canvas(320, 64)
        top = (110, 70, 220) if k == 0 else (255, 120, 210)
        c.rect(4, 6, 316, 60, darken(top, 0.4), radius=26); c.rect(4, 2, 316, 54, top, (255, 240, 200) if k else (40, 20, 70), 2.5, radius=26)
        c.rect(18, 8, 302, 22, lighten(top, 0.25), radius=10)
        fr.append(c.final())
    sheet([[f] for f in fr], 320, 64).save(out('ui', 'button.png'))
    # slider bar / knob
    c = Canvas(256, 24); c.rect(2, 6, 254, 18, (40, 30, 70), (255, 220, 140), 1.5, radius=6); c.final().save(out('ui', 'slider_bar.png'))
    c = Canvas(32, 32); c.ellipse(16, 16, 13, 13, (255, 236, 120), INK, 2); c.ellipse(13, 12, 4, 3, (255, 255, 230)); c.final().save(out('ui', 'slider_knob.png'))
    # vignette (fallback when shaders are unavailable)
    W, H = 640, 360
    yy, xx = np.mgrid[0:H, 0:W]
    d = np.sqrt(((xx - W / 2) / (W / 2)) ** 2 + ((yy - H / 2) / (H / 2)) ** 2)
    a = np.clip((d - 0.65) / 0.75, 0, 1) ** 1.5 * 190
    arr = np.zeros((H, W, 4), dtype=np.uint8); arr[:, :, 0] = 10; arr[:, :, 1] = 0; arr[:, :, 2] = 25; arr[:, :, 3] = a.astype(np.uint8)
    Image.fromarray(arr, 'RGBA').save(out('ui', 'vignette.png'))
    # Logo
    W, H = 900, 300
    font = ImageFont.truetype(FONT_BOLD, 150); font2 = ImageFont.truetype(FONT_BOLD, 46)
    img = Image.new('RGBA', (W, H), (0, 0, 0, 0)); d = ImageDraw.Draw(img)
    text = 'LUMO'
    tw = d.textlength(text, font=font)
    x0 = (W - tw) / 2
    glow = Image.new('RGBA', (W, H), (0, 0, 0, 0)); gd = ImageDraw.Draw(glow)
    gd.text((x0, 10), text, font=font, fill=(255, 120, 220, 255), stroke_width=18, stroke_fill=(255, 120, 220, 255))
    glow = glow.filter(ImageFilter.GaussianBlur(14)); img.alpha_composite(glow)
    d.text((x0, 10), text, font=font, fill=(255, 255, 255), stroke_width=9, stroke_fill=(40, 16, 70))
    # gradient fill
    mask = Image.new('L', (W, H), 0); ImageDraw.Draw(mask).text((x0, 10), text, font=font, fill=255)
    grad = vgradient(W, H, [(0, (255, 250, 190)), (0.35, (255, 200, 80)), (0.6, (255, 110, 190)), (1, (140, 80, 255))])
    img.paste(grad, (0, 0), mask)
    sub = 'AND THE SHATTERED DREAM'
    sw = d.textlength(sub, font=font2)
    d.text(((W - sw) / 2, 205), sub, font=font2, fill=(255, 240, 200), stroke_width=5, stroke_fill=(40, 16, 70))
    c = Canvas(W, H); c.img = img.resize((W * SS, H * SS), Image.LANCZOS); c.d = ImageDraw.Draw(c.img)
    c.glow(x0 + tw - 4, 52, 30, (255, 240, 140), 0.9); c.star(x0 + tw - 4, 52, 18, 7, (255, 245, 170), 5, outline=(40, 16, 70), width=2)
    c.final().save(out('ui', 'logo.png'))
    # app icon (png for window icon)
    c = Canvas(64, 64); c.ellipse(32, 36, 22, 20, (255, 196, 150), INK, 2); c.poly([(10, 30), (54, 30), (16, 4)], (60, 130, 255), INK, 2)
    c.ellipse(27, 37, 3, 4.5, INK); c.ellipse(39, 37, 3, 4.5, INK); c.star(16, 5, 7, 3, (255, 236, 120)); c.final().save(out('ui', 'icon.png'))

if __name__ == '__main__':
    collectibles(); objects(); worldmap(); ui()
