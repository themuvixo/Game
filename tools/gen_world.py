"""Tilesets, parallax backgrounds and world decorations for the five worlds (original, CC0)."""
import math
from gfx_common import *

T = 48
WORLDS = {
    1: dict(name='Dream Forest', sky=[(0, (70, 30, 120)), (0.55, (190, 90, 200)), (1, (255, 180, 210))],
            far=(120, 70, 170), mid=(90, 50, 140), near=(50, 30, 90), ground=(110, 70, 60), top=(120, 220, 120), top2=(255, 140, 200),
            rock=(130, 90, 150), accent=(255, 120, 220), glow=(140, 255, 220), fog=(240, 200, 255)),
    2: dict(name='Band Land', sky=[(0, (120, 30, 40)), (0.5, (240, 110, 50)), (1, (255, 210, 120))],
            far=(200, 90, 60), mid=(160, 60, 50), near=(90, 30, 40), ground=(150, 80, 40), top=(255, 200, 80), top2=(255, 120, 60),
            rock=(190, 110, 60), accent=(255, 230, 120), glow=(255, 240, 160), fog=(255, 210, 160)),
    3: dict(name='Picture City', sky=[(0, (10, 5, 30)), (0.6, (60, 20, 90)), (1, (200, 40, 140))],
            far=(40, 30, 80), mid=(30, 25, 60), near=(15, 12, 35), ground=(50, 50, 80), top=(0, 240, 255), top2=(255, 50, 200),
            rock=(70, 70, 110), accent=(0, 255, 220), glow=(255, 60, 220), fog=(120, 60, 200)),
    4: dict(name='Frozen Kingdom', sky=[(0, (40, 80, 150)), (0.6, (140, 190, 235)), (1, (225, 240, 255))],
            far=(170, 200, 235), mid=(130, 165, 215), near=(90, 120, 175), ground=(110, 130, 170), top=(245, 250, 255), top2=(200, 230, 255),
            rock=(150, 175, 210), accent=(160, 230, 255), glow=(200, 245, 255), fog=(235, 245, 255)),
    5: dict(name='Volcano of Shadows', sky=[(0, (20, 5, 10)), (0.55, (90, 20, 20)), (1, (230, 90, 30))],
            far=(80, 25, 25), mid=(55, 18, 22), near=(30, 10, 15), ground=(70, 40, 40), top=(120, 70, 60), top2=(255, 120, 40),
            rock=(90, 55, 50), accent=(255, 150, 40), glow=(255, 180, 60), fog=(150, 60, 40)),
}

def textured_block(base, seed, w=T, h=T, var=0.18):
    n = noise_2d(w, h, 10, seed, 3)
    arr = np.zeros((h, w, 4), dtype=np.uint8)
    for k in range(3):
        arr[:, :, k] = np.clip(base[k] * (1 - var + 2 * var * n), 0, 255)
    arr[:, :, 3] = 255
    return Image.fromarray(arr, 'RGBA')

def tile_set(w):
    P = WORLDS[w]
    tiles = []
    # 0 ground top
    g = textured_block(P['ground'], w * 10 + 1)
    c = Canvas(T, T); c.img = g.resize((T * SS, T * SS)); c.d = ImageDraw.Draw(c.img)
    for k in range(4):
        c.ellipse(8 + k * 11, 30 + (k % 2) * 8, 4, 3, darken(P['ground'], 0.2))
    c.rect(0, 0, T, 12, P['top'])
    for k in range(9):
        c.ellipse(k * 6 + 2, 12, 4, 4 + (k % 3), P['top'])
    c.rect(0, 0, T, 3, lighten(P['top'], 0.4))
    if w == 1:
        c.ellipse(14, 5, 3, 3, P['top2']); c.ellipse(36, 6, 2.5, 2.5, (255, 255, 255))
    if w == 3:
        c.rect(0, 10, T, 12, P['top2'])
    if w == 5:
        for k in range(3):
            c.line([(6 + k * 16, 14), (10 + k * 16, 30), (8 + k * 16, 44)], (255, 110, 30), 1.4)
    tiles.append(c.final())
    # 1 ground fill
    g = textured_block(P['ground'], w * 10 + 2)
    c = Canvas(T, T); c.img = g.resize((T * SS, T * SS)); c.d = ImageDraw.Draw(c.img)
    for k in range(5):
        c.ellipse((k * 17 + 7) % 44 + 2, (k * 23 + 5) % 40 + 4, 4 + k % 2, 3, darken(P['ground'], 0.22))
    if w == 3:
        c.rect(4, 4, 44, 44, None, (90, 90, 140), 1); c.rect(20, 20, 28, 28, (255, 220, 100, 120))
    tiles.append(c.final())
    # 2 one-way platform
    c = Canvas(T, T)
    c.rect(0, 2, T, 18, P['rock'], radius=5); c.rect(0, 2, T, 7, lighten(P['rock'], 0.3), radius=3)
    c.line([(4, 18), (44, 18)], darken(P['rock'], 0.4), 1.5)
    tiles.append(c.final())
    # 3 breakable
    c = Canvas(T, T); c.rect(1, 1, 47, 47, P['rock'], darken(P['rock'], 0.5), 2, radius=5)
    c.line([(8, 10), (20, 24), (14, 38)], darken(P['rock'], 0.5), 2); c.line([(20, 24), (38, 18)], darken(P['rock'], 0.5), 2)
    c.star(24, 24, 7, 3, P['accent'], 4)
    tiles.append(c.final())
    # 4 ice
    c = Canvas(T, T); c.rect(0, 0, T, T, (170, 220, 255)); c.rect(0, 0, T, 8, (235, 250, 255))
    c.line([(6, 40), (30, 12)], (255, 255, 255, 200), 2); c.line([(22, 44), (42, 20)], (255, 255, 255, 150), 1.5)
    tiles.append(c.final())
    # 5 spikes (world flavoured)
    c = Canvas(T, T)
    sc = {1: (120, 200, 90), 2: (220, 220, 230), 3: (255, 60, 200), 4: (210, 240, 255), 5: (60, 40, 40)}[w]
    for k in range(4):
        c.poly([(k * 12, 48), (k * 12 + 12, 48), (k * 12 + 6, 18)], sc, darken(sc, 0.5), 1)
        c.line([(k * 12 + 6, 20), (k * 12 + 8, 44)], lighten(sc, 0.4), 1)
    tiles.append(c.final())
    # 6 deco grass tuft
    c = Canvas(T, T)
    for k in range(7):
        c.poly([(6 + k * 6, 48), (10 + k * 6, 48), (8 + k * 6 + (k % 3 - 1) * 4, 30 - (k % 2) * 8)], P['top'])
    tiles.append(c.final())
    # 7 fake wall (looks like fill, slightly marked)
    f = tiles[1].copy(); c = Canvas(T, T); c.img = f.resize((T * SS, T * SS)); c.d = ImageDraw.Draw(c.img)
    c.ellipse(40, 8, 1.5, 1.5, (255, 255, 255, 120)); tiles.append(c.final())
    # 8 door (timed)
    c = Canvas(T, T); c.rect(4, 0, 44, 48, (90, 70, 110), (40, 20, 50), 2); c.rect(10, 0, 38, 48, (120, 100, 150))
    for k in range(3):
        c.rect(4, 6 + k * 16, 44, 9 + k * 16, (60, 40, 70))
    c.ellipse(24, 24, 6, 6, P['accent']); tiles.append(c.final())
    # 9,10 switch off/on
    for on in (0, 1):
        c = Canvas(T, T); c.rect(10, 34, 38, 48, (80, 80, 100), radius=4)
        c.line([(24, 40), (14 + on * 20, 20)], (200, 200, 210), 4)
        c.ellipse(14 + on * 20, 20, 6, 6, (255, 80, 80) if not on else (80, 255, 120))
        tiles.append(c.final())
    # 11-14 water frames
    for k in range(4):
        c = Canvas(T, T); c.rect(0, 0, T, T, (60, 140, 230, 150))
        for x in range(0, T + 12, 12):
            c.ellipse(x + k * 3 - 6, 2, 7, 3, (200, 240, 255, 200))
        c.line([(4 + k * 6, 24), (14 + k * 6, 24)], (220, 245, 255, 110), 1.5)
        tiles.append(c.final())
    # 15-18 lava frames
    for k in range(4):
        n = noise_2d(T, T, 8, 900 + k, 3)
        arr = np.zeros((T, T, 4), dtype=np.uint8)
        arr[:, :, 0] = 255; arr[:, :, 1] = np.clip(70 + 150 * n, 0, 255); arr[:, :, 2] = np.clip(20 + 40 * n, 0, 255); arr[:, :, 3] = 255
        im = Image.fromarray(arr, 'RGBA'); c = Canvas(T, T); c.img = im.resize((T * SS, T * SS)); c.d = ImageDraw.Draw(c.img)
        for x in range(0, T + 12, 12):
            c.ellipse(x + k * 3 - 6, 3, 7, 4, (255, 230, 120))
        tiles.append(c.final())
    # 19 disappearing platform, 20 falling platform, 21 moving platform, 22 bouncy mushroom/drum, 23 spring
    c = Canvas(T, T); col = P['accent']
    c.rect(1, 2, 47, 20, mix(col, (255, 255, 255), 0.3), darken(col, 0.4), 2, radius=6)
    for k in range(3):
        c.star(10 + k * 14, 11, 4, 1.6, (255, 255, 255), 4)
    tiles.append(c.final())
    c = Canvas(T, T); c.rect(1, 2, 47, 22, (150, 110, 80), (70, 40, 30), 2, radius=3)
    c.line([(8, 8), (16, 18)], (90, 60, 40), 2); c.line([(30, 6), (38, 16)], (90, 60, 40), 2); tiles.append(c.final())
    c = Canvas(T, T); c.rect(0, 2, T, 20, lighten(P['rock'], 0.15), darken(P['rock'], 0.5), 2, radius=4)
    c.ellipse(10, 11, 3, 3, (255, 220, 120)); c.ellipse(38, 11, 3, 3, (255, 220, 120)); tiles.append(c.final())
    c = Canvas(T, T)
    if w == 2:
        c.rect(6, 22, 42, 48, (200, 60, 40), (80, 20, 10), 2, radius=3); c.ellipse(24, 22, 18, 6, (250, 235, 210))
    else:
        c.rect(18, 26, 30, 48, (240, 230, 210)); c.ellipse(24, 26, 22, 14, P['top2'] if w != 4 else (120, 200, 255))
        for k in range(3):
            c.ellipse(12 + k * 12, 22, 3, 3, (255, 255, 255))
    tiles.append(c.final())
    c = Canvas(T, T); c.rect(8, 40, 40, 48, (90, 90, 110), radius=2)
    for k in range(3):
        c.line([(12, 40 - k * 8), (36, 36 - k * 8)], (200, 200, 220), 2.5)
    c.rect(6, 12, 42, 18, (255, 80, 80), radius=3); tiles.append(c.final())
    rows = [tiles[0:8], tiles[8:16], tiles[16:24]]
    sheet(rows, T, T).save(out('tilesets', 'w%d_tiles.png' % w))

def silhouette_layer(W, H, color, seed, base, amp, kind, alpha=255, extra=None):
    img = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    n = value_noise_1d(W, 5, seed, 3)
    pts = [(x, int(base - n[x] * amp)) for x in range(W)]
    d.polygon([(0, H)] + pts + [(W - 1, H)], fill=rgba(color, alpha))
    rng = random.Random(seed)
    if kind == 'trees':
        for _ in range(26):
            x = rng.randrange(0, W); y = pts[x][1]; h = rng.randint(60, 150)
            d.rectangle([x - 4, y - h * 0.4, x + 4, y + 10], fill=rgba(color, alpha))
            for k in range(3):
                r = h * (0.35 - k * 0.07)
                d.ellipse([x - r, y - h * 0.4 - r - k * h * 0.18, x + r, y - h * 0.4 + r - k * h * 0.18], fill=rgba(color, alpha))
    elif kind == 'pines':
        for _ in range(40):
            x = rng.randrange(0, W); y = pts[x][1]; h = rng.randint(50, 130)
            d.polygon([(x - h * 0.25, y + 6), (x + h * 0.25, y + 6), (x, y - h)], fill=rgba(color, alpha))
    elif kind == 'city':
        x = 0
        while x < W:
            bw = rng.randint(40, 110); bh = rng.randint(120, 380)
            y = base - bh
            d.rectangle([x, y, x + bw, H], fill=rgba(color, alpha))
            if extra:
                for wy in range(int(y) + 10, H - 20, 18):
                    for wx in range(x + 8, x + bw - 8, 14):
                        if rng.random() < 0.35:
                            d.rectangle([wx, wy, wx + 6, wy + 8], fill=rgba(extra[rng.randrange(len(extra))], 220))
            x += bw + rng.randint(4, 20)
    elif kind == 'drums':
        for _ in range(9):
            x = rng.randrange(0, W); y = pts[x][1]; r = rng.randint(40, 90)
            d.rectangle([x - r, y - r * 1.2, x + r, y + 10], fill=rgba(color, alpha))
            d.ellipse([x - r, y - r * 1.2 - r * 0.3, x + r, y - r * 1.2 + r * 0.3], fill=rgba(lighten(color, 0.2), alpha))
    elif kind == 'ruins':
        for _ in range(12):
            x = rng.randrange(0, W); y = pts[x][1]; h = rng.randint(80, 220)
            d.rectangle([x - 14, y - h, x + 14, y + 10], fill=rgba(color, alpha))
            d.rectangle([x - 22, y - h - 12, x + 22, y - h], fill=rgba(color, alpha))
    elif kind == 'volcano':
        cx = W // 2
        d.polygon([(cx - 520, H), (cx - 120, base - amp - 120), (cx + 120, base - amp - 120), (cx + 520, H)], fill=rgba(color, alpha))
        d.polygon([(cx - 120, base - amp - 120), (cx + 120, base - amp - 120), (cx + 60, base - amp - 60), (cx - 60, base - amp - 60)], fill=(255, 120, 30, alpha))
    # make horizontally tileable: blend edges
    return img

def background(w):
    P = WORLDS[w]
    W, H = 1280, 720
    sky = vgradient(W, H, P['sky'])
    d = ImageDraw.Draw(sky)
    rng = random.Random(w)
    if w in (1, 3, 5):
        for _ in range(160 if w == 3 else 90):
            x, y = rng.randrange(W), rng.randrange(int(H * 0.6)); r = rng.choice([1, 1, 2])
            d.ellipse([x - r, y - r, x + r, y + r], fill=(255, 255, 240, rng.randint(120, 255)))
    c = Canvas(W, H); c.img = sky.resize((W * SS, H * SS)); c.d = ImageDraw.Draw(c.img)
    if w == 1:
        c.glow(980, 160, 120, (255, 230, 255), 0.7); c.ellipse(980, 160, 60, 60, (255, 245, 250))
    elif w == 2:
        c.glow(640, 520, 260, (255, 240, 160), 0.7); c.ellipse(640, 520, 130, 130, (255, 230, 140))
    elif w == 3:
        c.ellipse(300, 130, 50, 50, (255, 120, 220)); c.ellipse(320, 120, 46, 46, rgba(P['sky'][0][1]))
    elif w == 4:
        c.glow(300, 140, 100, (255, 255, 255), 0.6); c.ellipse(300, 140, 50, 50, (255, 255, 250))
        for k in range(5):
            c.rot_ellipse(640 + k * 120, 120 + math.sin(k) * 40, 260, 30, -0.2, (140, 255, 200, 50))
    elif w == 5:
        c.glow(640, 700, 400, (255, 90, 20), 0.6)
    c.final().convert('RGBA').save(out('backgrounds', 'w%d_layer0.png' % w))
    kinds = {1: ('trees', 'trees', 'trees'), 2: ('drums', 'trees', 'drums'), 3: ('city', 'city', 'city'),
             4: ('pines', 'pines', 'pines'), 5: ('volcano', 'ruins', 'ruins')}[w]
    win = [P['top'], P['top2'], (255, 230, 120)] if w == 3 else None
    l1 = silhouette_layer(W, H, P['far'], w * 7 + 1, 470, 200, kinds[0], 255, win)
    l2 = silhouette_layer(W, H, P['mid'], w * 7 + 2, 560, 150, kinds[1], 255, win)
    l3 = silhouette_layer(W, H, P['near'], w * 7 + 3, 650, 90, kinds[2], 255, win)
    # floating islands in layer 1 for world 1/2/4
    if w in (1, 2, 4):
        c = Canvas(W, H); c.img = l1.resize((W * SS, H * SS)); c.d = ImageDraw.Draw(c.img)
        for k in range(3):
            x = 200 + k * 420; y = 200 + (k % 2) * 80
            c.poly([(x - 90, y), (x + 90, y), (x + 30, y + 90), (x - 10, y + 120)], darken(P['far'], 0.15))
            c.ellipse(x, y, 92, 18, lighten(P['far'], 0.25))
            if w == 1:
                c.rect(x + 40, y, x + 52, y + 220, (200, 230, 255, 120))
        l1 = c.final()
    # glow mushrooms / neon signs / crystals / lava falls
    c = Canvas(W, H); c.img = l2.resize((W * SS, H * SS)); c.d = ImageDraw.Draw(c.img)
    rng = random.Random(w * 99)
    for _ in range(10):
        x = rng.randrange(40, W - 40); y = rng.randrange(560, 660)
        if w == 1:
            c.glow(x, y - 20, 40, P['glow'], 0.5); c.rect(x - 4, y - 20, x + 4, y + 10, (230, 220, 250)); c.ellipse(x, y - 22, 18, 10, P['glow'])
        elif w == 3:
            c.glow(x, y - 200, 50, P['top2'] if _ % 2 else P['top'], 0.4); c.rect(x - 30, y - 216, x + 30, y - 190, None, P['top2'] if _ % 2 else P['top'], 3, radius=6)
        elif w == 4:
            c.poly([(x - 12, y + 10), (x + 12, y + 10), (x, y - 50)], (200, 240, 255, 200))
        elif w == 5:
            c.glow(x, y, 40, (255, 120, 30), 0.5)
        elif w == 2:
            c.line([(x, y - 120), (x, y - 160)], (60, 20, 20), 3); c.ellipse(x - 6, y - 118, 9, 7, (60, 20, 20))
    l2 = c.final()
    l1.save(out('backgrounds', 'w%d_layer1.png' % w))
    l2.save(out('backgrounds', 'w%d_layer2.png' % w))
    l3.save(out('backgrounds', 'w%d_layer3.png' % w))

def fog():
    W, H = 1024, 256
    n = noise_2d(W, H, 60, 77, 4)
    yy = np.linspace(0, 1, H)[:, None]
    a = np.clip((n - 0.35) * 2.2, 0, 1) * np.sin(yy * math.pi) * 200
    arr = np.zeros((H, W, 4), dtype=np.uint8); arr[:, :, 0:3] = 255; arr[:, :, 3] = a.astype(np.uint8)
    im = Image.fromarray(arr, 'RGBA')
    # make tileable horizontally by crossfade
    left = im.crop((0, 0, 128, H)); right = im.crop((W - 128, 0, W, H))
    mask = Image.linear_gradient('L').rotate(90).resize((128, H))
    im.paste(Image.composite(left, right, mask), (W - 128, 0))
    im.save(out('backgrounds', 'fog.png'))

def deco(w):
    P = WORLDS[w]
    frames = []
    for k in range(4):
        c = Canvas(96, 96)
        if w == 1:
            if k < 2:
                c.rect(44, 40, 52, 96, (250, 230, 255)); c.glow(48, 40, 30, P['glow'], 0.6); c.ellipse(48, 40, 30, 16, P['glow'] if k == 0 else (255, 150, 230))
                for j in range(3):
                    c.ellipse(36 + j * 12, 36, 3, 3, (255, 255, 255))
            else:
                c.rect(44, 30, 52, 96, (120, 70, 60))
                c.ellipse(48, 30, 36, 26, (255, 140, 210)); c.ellipse(36, 22, 18, 14, (255, 180, 230))
        elif w == 2:
            if k < 2:
                c.line([(40, 90), (40, 30)], (40, 20, 20), 4); c.ellipse(32, 88, 12, 9, (40, 20, 20)); c.line([(40, 30), (60, 40)], (40, 20, 20), 4)
                c.glow(40, 70, 30, P['glow'], 0.4)
            else:
                c.rect(20, 50, 76, 96, (200, 60, 40), (90, 20, 10), 2, radius=4); c.ellipse(48, 50, 28, 8, (250, 235, 210))
        elif w == 3:
            col = P['top'] if k % 2 else P['top2']
            c.rect(46, 30, 50, 96, (80, 80, 110)); c.glow(48, 26, 30, col, 0.6); c.rect(28, 14, 68, 36, (20, 20, 40), col, 2, radius=5)
        elif w == 4:
            for j in range(3):
                c.poly([(30 + j * 16, 96), (46 + j * 16, 96), (38 + j * 16, 40 - (j % 2) * 20)], (200, 240, 255, 230), (255, 255, 255), 1)
        elif w == 5:
            if k < 2:
                c.rect(30, 30, 66, 96, (90, 60, 60), (40, 20, 20), 2); c.rect(24, 22, 72, 32, (100, 70, 70))
            else:
                c.glow(48, 70, 36, (255, 120, 30), 0.7); c.poly([(36, 96), (60, 96), (48, 50)], (255, 160, 40))
        frames.append(outline_image(c.final(), (30, 16, 40), 1))
    sheet([frames], 96, 96).save(out('objects', 'deco_w%d.png' % w))

if __name__ == '__main__':
    for w in range(1, 6):
        tile_set(w); background(w); deco(w)
    fog()
