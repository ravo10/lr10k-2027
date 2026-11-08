"""Draws docs/themes/layout_annotated.png: theme_USN.png with the parts of the main window outlined and numbered
(README, "The main window"). Run from the repository folder after updating the theme pictures (tools/theme_preview.cpp):

    python3 tools/annotate_layout.py

Needs Pillow (pip install pillow). The boxes match the mock-up's default layout at 1600 x 950; move them if it changes.
"""
from PIL import Image, ImageDraw, ImageFont

SRC = "docs/themes/theme_USN.png"
DST = "docs/themes/layout_annotated.png"
FONT = "main/LloydLR10KTensile/fonts/Inter_18pt-Medium.ttf"
ACCENT = (255, 98, 2)  # USN orange, stands out on the purple

# number, box (left, top, right, bottom), badge centre
REGIONS = [
    (1, (2, 2, 1597, 28), (440, 15)),         # Menu bar
    (2, (4, 36, 156, 66), (175, 51)),         # Test tabs
    (3, (8, 72, 412, 942), (392, 182)),       # Side panel
    (4, (420, 72, 1247, 722), (470, 105)),    # Graph
    (5, (420, 726, 1247, 942), (1210, 760)),  # Console
    (6, (1252, 72, 1594, 942), (1560, 118)),  # Settings / Calculator
    (7, (1478, 2, 1597, 28), (1450, 15)),     # Machine status
]

image = Image.open(SRC).convert("RGBA")
overlay = Image.new("RGBA", image.size, (0, 0, 0, 0))
draw = ImageDraw.Draw(overlay)
font = ImageFont.truetype(FONT, 22)
for _, box, _ in REGIONS:
    draw.rectangle(box, outline=ACCENT + (255,), width=3)
for number, _, (cx, cy) in REGIONS:
    r = 15
    draw.ellipse((cx - r, cy - r, cx + r, cy + r), fill=ACCENT + (255,), outline=(255, 255, 255, 255), width=2)
    draw.text((cx, cy), str(number), font=font, fill=(255, 255, 255, 255), anchor="mm")
Image.alpha_composite(image, overlay).convert("RGB").save(DST, optimize=True)
print("wrote", DST)
