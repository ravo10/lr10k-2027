"""Draws the LR 10K splash screen, main/LloydLR10KTensile/images/bootbackground_start.png: 3200 x 1800 (16:9).

The app stretches it to the window, so keep 16:9. Needs Pillow and the Inter fonts (OFL; on Ubuntu: fonts-inter).

    python3 tools/splash/make_splash.py main/LloydLR10KTensile/images/bootbackground_start.png preview.png
"""
import math, random, sys
from PIL import Image, ImageDraw, ImageFilter, ImageFont

W, H = 3200, 1800
F = '/usr/share/fonts/opentype/inter/'
def font(name, size): return ImageFont.truetype(F + name, size)

# Background: deep navy with a soft blue glow top right
img = Image.new('RGB', (W, H), (9, 14, 30))
glow = Image.new('L', (W, H), 0)
gd = ImageDraw.Draw(glow)
gd.ellipse((W * 0.45, -H * 0.55, W * 1.35, H * 0.85), fill=255)
glow = glow.filter(ImageFilter.GaussianBlur(320))
img = Image.composite(Image.new('RGB', (W, H), (24, 52, 120)), img, glow)
glow2 = Image.new('L', (W, H), 0)
ImageDraw.Draw(glow2).ellipse((-W * 0.3, H * 0.6, W * 0.5, H * 1.6), fill=150)
glow2 = glow2.filter(ImageFilter.GaussianBlur(300))
img = Image.composite(Image.new('RGB', (W, H), (40, 20, 90)), img, glow2)

layer = Image.new('RGBA', (W, H), (0, 0, 0, 0))
d = ImageDraw.Draw(layer)

# ---- Stars: a sparse faint field, and the Pleiades (M45) at the upper right, with a static twinkle
rnd = random.Random(45)
for _ in range(170):
    sx, sy = rnd.uniform(0, W), rnd.uniform(0, H * 0.62)
    if sx < 1900 and 330 < sy:   # Keep the text area calm
        continue
    r = rnd.choice((1.6, 2.0, 2.4, 3.0))
    a = rnd.randint(20, 60)
    d.ellipse((sx - r, sy - r, sx + r, sy + r), fill=(220, 230, 255, a))

# The nine named stars: (name, RA in hours, Dec in degrees, magnitude). North up, east left, as seen in the sky.
PLEIADES = [
    ('Alcyone', 3 + 47 / 60 + 29 / 3600, 24 + 6 / 60, 2.87), ('Atlas', 3 + 49 / 60 + 9 / 3600, 24 + 3 / 60, 3.62),
    ('Electra', 3 + 44 / 60 + 52 / 3600, 24 + 7 / 60, 3.70), ('Maia', 3 + 45 / 60 + 50 / 3600, 24 + 22 / 60, 3.86),
    ('Merope', 3 + 46 / 60 + 20 / 3600, 23 + 57 / 60, 4.17), ('Taygeta', 3 + 45 / 60 + 12 / 3600, 24 + 28 / 60, 4.29),
    ('Pleione', 3 + 49 / 60 + 11 / 3600, 24 + 8 / 60, 5.05), ('Celaeno', 3 + 44 / 60 + 48 / 3600, 24 + 17 / 60, 5.45),
    ('Asterope', 3 + 45 / 60 + 55 / 3600, 24 + 33 / 60, 5.76),
]
ra0 = sum(st[1] for st in PLEIADES) / len(PLEIADES)
de0 = sum(st[2] for st in PLEIADES) / len(PLEIADES)
cx, cy, scale = 2690, 250, 640   # Centre on the splash and pixels per degree
pos = []
for name, ra, de, mag in PLEIADES:
    x = cx - (ra - ra0) * 15 * math.cos(math.radians(de0)) * scale
    y = cy - (de - de0) * scale
    pos.append((x, y, mag))

# A faint blue haze (the reflection nebula)
haze = Image.new('L', (W, H), 0)
hd = ImageDraw.Draw(haze)
for x, y, mag in pos:
    rr = 150 - mag * 14
    hd.ellipse((x - rr, y - rr, x + rr, y + rr), fill=40)
haze = haze.filter(ImageFilter.GaussianBlur(70))
haze_rgba = Image.new('RGBA', (W, H), (110, 150, 255, 0))
haze_rgba.putalpha(haze)
layer = Image.alpha_composite(layer, haze_rgba)

# Each star: a soft glow, a bright core and, for the brighter ones, four thin spikes (the "twinkle")
glow_l = Image.new('RGBA', (W, H), (0, 0, 0, 0))
gl = ImageDraw.Draw(glow_l)
for x, y, mag in pos:
    g = 52 - mag * 6
    gl.ellipse((x - g, y - g, x + g, y + g), fill=(170, 200, 255, 85))
glow_l = glow_l.filter(ImageFilter.GaussianBlur(14))
layer = Image.alpha_composite(layer, glow_l)
# Spikes on their own layer (drawing them straight onto the stars would cut dark lines through the cores)
spikes = Image.new('RGBA', (W, H), (0, 0, 0, 0))
sp = ImageDraw.Draw(spikes)
for x, y, mag in pos:
    if mag >= 4.4:
        continue
    length = 120 - mag * 20
    for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
        steps = 40
        for i in range(steps):
            t0, t1 = i / steps, (i + 1) / steps
            a = int(130 * (1 - t0) ** 1.8)
            w = 4 if t0 < 0.25 else (3 if t0 < 0.55 else 2)
            sp.line((x + dx * length * t0, y + dy * length * t0, x + dx * length * t1, y + dy * length * t1),
                    fill=(230, 238, 255, a), width=w)
spikes = spikes.filter(ImageFilter.GaussianBlur(0.8))
layer = Image.alpha_composite(layer, spikes)
cores = Image.new('RGBA', (W, H), (0, 0, 0, 0))
cd = ImageDraw.Draw(cores)
for x, y, mag in pos:
    core = 11 - mag * 1.35
    cd.ellipse((x - core, y - core, x + core, y + core), fill=(235, 242, 255, int(250 - mag * 22)))
layer = Image.alpha_composite(layer, cores)
d = ImageDraw.Draw(layer)

# A faint graph on the right: grid and a stress-strain curve (elastic, yield, UTS, necking, fracture)
gx0, gy0, gx1, gy1 = 1960, 1010, 2970, 1430
for i in range(9):
    x = gx0 + (gx1 - gx0) * i / 8
    d.line((x, gy0, x, gy1), fill=(255, 255, 255, 14), width=2)
for i in range(5):
    y = gy0 + (gy1 - gy0) * i / 4
    d.line((gx0, y, gx1, y), fill=(255, 255, 255, 14), width=2)
d.line((gx0, gy1, gx1, gy1), fill=(255, 255, 255, 60), width=3)
d.line((gx0, gy0, gx0, gy1), fill=(255, 255, 255, 60), width=3)

def curve(t):  # t: 0..1 strain -> 0..1 stress
    if t < 0.12: return t / 0.12 * 0.78
    if t < 0.62: return 0.78 + 0.22 * math.sin((t - 0.12) / 0.5 * math.pi / 2)
    return 1.0 - 0.9 * ((t - 0.62) / 0.38) ** 2.2 * 0.35
pts = []
for i in range(401):
    t = i / 400 * 0.97
    pts.append((gx0 + t * (gx1 - gx0), gy1 - curve(t) * (gy1 - gy0) * 0.86))
glowc = Image.new('RGBA', (W, H), (0, 0, 0, 0))
ImageDraw.Draw(glowc).line(pts, fill=(59, 130, 246, 170), width=26, joint='curve')
glowc = glowc.filter(ImageFilter.GaussianBlur(22))
layer = Image.alpha_composite(layer, glowc)
d = ImageDraw.Draw(layer)
d.line(pts, fill=(96, 165, 250, 255), width=9, joint='curve')
# Crucial points: yield, UTS, fracture
for t, col in ((0.12, (217, 119, 6)), (0.62, (8, 145, 178)), (0.97, (220, 38, 38))):
    x, y = gx0 + t * (gx1 - gx0), gy1 - curve(t) * (gy1 - gy0) * 0.86
    d.ellipse((x - 22, y - 22, x + 22, y + 22), fill=col + (255,), outline=(255, 255, 255, 255), width=6)

# Text, left aligned
x = 230
d.text((x, 380), 'USN KONGSBERG  ·  MACHINE ENGINEERING', font=font('Inter-SemiBold.otf', 52), fill=(147, 197, 253, 255))
d.rectangle((x, 480, x + 150, 492), fill=(59, 130, 246, 255))
d.text((x, 545), 'LR 10K Tensile Machine', font=font('InterDisplay-Bold.otf', 196), fill=(255, 255, 255, 255))
d.text((x, 775), 'Control Software', font=font('InterDisplay-Light.otf', 196), fill=(226, 232, 240, 255))
d.text((x, 1060), 'Tension, compression and 3-point bending tests,', font=font('Inter-Regular.otf', 60), fill=(203, 213, 225, 255))
d.text((x, 1140), 'live graphs, optical extensometer and Excel reports.', font=font('Inter-Regular.otf', 60), fill=(203, 213, 225, 255))

# Footer
d.line((x, 1520, W - 230, 1520), fill=(255, 255, 255, 40), width=2)
d.text((x, 1565), 'Developed by Rasmus Ullestad Relling', font=font('Inter-Medium.otf', 54), fill=(241, 245, 249, 255))
right = 'Open source  ·  2023–2026'
fr = font('Inter-Regular.otf', 50)
d.text((W - 230 - d.textlength(right, font=fr), 1570), right, font=fr, fill=(148, 163, 184, 255))

img = Image.alpha_composite(img.convert('RGBA'), layer).convert('RGB')
img.save(sys.argv[1], optimize=True)
img.resize((1200, 675), Image.LANCZOS).save(sys.argv[2])
