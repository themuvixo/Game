"""Shared drawing helpers for the procedural asset generator (original art, CC0)."""
import math, os, random
import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageChops

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', 'assets'))
SS = 3  # supersampling factor

def out(*parts):
    p = os.path.join(ROOT, *parts)
    os.makedirs(os.path.dirname(p), exist_ok=True)
    return p

def rgba(c, a=255):
    if len(c) == 4:
        return tuple(c)
    return (c[0], c[1], c[2], a)

def mix(a, b, t):
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(len(a)))

def lighten(c, t):
    return mix(c[:3], (255, 255, 255), t)

def darken(c, t):
    return mix(c[:3], (0, 0, 0), t)

class Canvas:
    """Supersampled RGBA canvas; coordinates are given in final pixels."""
    def __init__(self, w, h):
        self.w, self.h = w, h
        self.img = Image.new('RGBA', (w * SS, h * SS), (0, 0, 0, 0))
        self.d = ImageDraw.Draw(self.img)

    def s(self, v):
        return v * SS

    def ellipse(self, cx, cy, rx, ry, fill, outline=None, width=0):
        box = [self.s(cx - rx), self.s(cy - ry), self.s(cx + rx), self.s(cy + ry)]
        self.d.ellipse(box, fill=rgba(fill) if fill else None,
                       outline=rgba(outline) if outline else None, width=int(self.s(width)))

    def rot_ellipse(self, cx, cy, rx, ry, ang, fill, outline=None, width=0):
        pts = []
        for i in range(48):
            t = 2 * math.pi * i / 48
            x, y = rx * math.cos(t), ry * math.sin(t)
            ca, sa = math.cos(ang), math.sin(ang)
            pts.append((cx + x * ca - y * sa, cy + x * sa + y * ca))
        self.poly(pts, fill, outline, width)

    def poly(self, pts, fill, outline=None, width=0):
        sp = [(self.s(x), self.s(y)) for x, y in pts]
        self.d.polygon(sp, fill=rgba(fill) if fill else None)
        if outline and width > 0:
            self.d.line(sp + [sp[0]], fill=rgba(outline), width=int(self.s(width)), joint='curve')

    def line(self, pts, color, width):
        sp = [(self.s(x), self.s(y)) for x, y in pts]
        self.d.line(sp, fill=rgba(color), width=max(1, int(self.s(width))), joint='curve')

    def rect(self, x0, y0, x1, y1, fill, outline=None, width=0, radius=0):
        box = [self.s(x0), self.s(y0), self.s(x1), self.s(y1)]
        if radius > 0:
            self.d.rounded_rectangle(box, radius=self.s(radius), fill=rgba(fill) if fill else None,
                                     outline=rgba(outline) if outline else None, width=int(self.s(width)))
        else:
            self.d.rectangle(box, fill=rgba(fill) if fill else None,
                             outline=rgba(outline) if outline else None, width=int(self.s(width)))

    def arc(self, cx, cy, rx, ry, a0, a1, color, width):
        box = [self.s(cx - rx), self.s(cy - ry), self.s(cx + rx), self.s(cy + ry)]
        self.d.arc(box, a0, a1, fill=rgba(color), width=max(1, int(self.s(width))))

    def star(self, cx, cy, r_out, r_in, fill, points=5, rot=-math.pi / 2, outline=None, width=0):
        pts = []
        for i in range(points * 2):
            r = r_out if i % 2 == 0 else r_in
            a = rot + math.pi * i / points
            pts.append((cx + r * math.cos(a), cy + r * math.sin(a)))
        self.poly(pts, fill, outline, width)

    def glow(self, cx, cy, r, color, strength=1.0):
        g = Image.new('RGBA', self.img.size, (0, 0, 0, 0))
        gd = ImageDraw.Draw(g)
        steps = 12
        for i in range(steps, 0, -1):
            rr = r * i / steps
            a = int(255 * strength * (1 - i / steps) ** 1.6 * 0.9) + 8
            gd.ellipse([self.s(cx - rr), self.s(cy - rr), self.s(cx + rr), self.s(cy + rr)],
                       fill=(color[0], color[1], color[2], min(255, a)))
        g = g.filter(ImageFilter.GaussianBlur(self.s(r) * 0.15))
        self.img = Image.alpha_composite(self.img, g)
        self.d = ImageDraw.Draw(self.img)

    def final(self):
        return self.img.resize((self.w, self.h), Image.LANCZOS)

def outline_image(img, color=(30, 18, 40), thickness=2):
    """Adds a dark cartoon outline around opaque pixels."""
    a = img.split()[3]
    grown = a.filter(ImageFilter.MaxFilter(thickness * 2 + 1))
    base = Image.new('RGBA', img.size, rgba(color, 0))
    base.putalpha(grown)
    solid = Image.new('RGBA', img.size, rgba(color))
    solid.putalpha(grown)
    return Image.alpha_composite(solid, img)

def sheet(frames_rows, fw, fh):
    rows = len(frames_rows)
    cols = max(len(r) for r in frames_rows)
    s = Image.new('RGBA', (cols * fw, rows * fh), (0, 0, 0, 0))
    for y, row in enumerate(frames_rows):
        for x, f in enumerate(row):
            s.paste(f, (x * fw, y * fh), f)
    return s

def value_noise_1d(n, octaves, seed, base_freq=4):
    rng = np.random.default_rng(seed)
    res = np.zeros(n)
    amp, freq, tot = 1.0, base_freq, 0
    for _ in range(octaves):
        pts = rng.random(freq + 1)
        pts[-1] = pts[0]  # tileable
        xs = np.linspace(0, freq, n, endpoint=False)
        i = xs.astype(int)
        f = xs - i
        f = f * f * (3 - 2 * f)
        res += amp * (pts[i] * (1 - f) + pts[(i + 1)] * f)
        tot += amp
        amp *= 0.5
        freq *= 2
    return res / tot

def noise_2d(w, h, scale, seed, octaves=4):
    rng = np.random.default_rng(seed)
    res = np.zeros((h, w))
    amp, tot = 1.0, 0
    for o in range(octaves):
        gw, gh = max(2, int(w / scale) + 2), max(2, int(h / scale) + 2)
        grid = rng.random((gh, gw))
        im = Image.fromarray((grid * 255).astype(np.uint8)).resize((w, h), Image.BICUBIC)
        res += amp * np.asarray(im, dtype=np.float32) / 255.0
        tot += amp
        amp *= 0.5
        scale = max(1, scale / 2)
    return res / tot

def vgradient(w, h, stops):
    """stops: list of (t, (r,g,b))"""
    arr = np.zeros((h, w, 4), dtype=np.uint8)
    for y in range(h):
        t = y / max(1, h - 1)
        for i in range(len(stops) - 1):
            if stops[i][0] <= t <= stops[i + 1][0]:
                lt = (t - stops[i][0]) / max(1e-6, stops[i + 1][0] - stops[i][0])
                c = mix(stops[i][1], stops[i + 1][1], lt)
                break
        else:
            c = stops[-1][1]
        arr[y, :, 0:3] = c
        arr[y, :, 3] = 255
    return Image.fromarray(arr, 'RGBA')
