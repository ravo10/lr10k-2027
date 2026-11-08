"""Draws the LR 10K splash screen, main/LloydLR10KTensile/images/bootbackground_start.png: 3200 x 1800 (16:9).

The app stretches it to the window, so keep 16:9. Needs Pillow and the Inter fonts (OFL; on Ubuntu: fonts-inter).

    python3 tools/splash/make_splash.py main/LloydLR10KTensile/images/bootbackground_start.png preview.png
"""
import math, sys
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
