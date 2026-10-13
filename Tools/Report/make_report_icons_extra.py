"""Placeholder icons for ammunition and food in the after-action report (only these two; the artist's icons are never touched).

    python Tools/Report/make_report_icons_extra.py
Output: Reference/UI/ReportIcons/Icon_Ammo.png and Icon_Food.png (256x256, cream outline). Replace with real art when it exists,
then run Content/Python/import_report_icons.py.
"""
import os
from PIL import Image, ImageChops, ImageDraw, ImageFilter

S = 256
OUT = os.path.join(os.path.dirname(__file__), "..", "..", "Reference", "UI", "ReportIcons")
BOX = (92, 112, 70, 255)
BOX_DARK = (56, 70, 44, 255)
IRON = (84, 90, 100, 255)
BRASS = (200, 160, 70, 255)
LEAD = (120, 122, 130, 255)
BREAD = (196, 142, 74, 255)
BREAD_DARK = (146, 96, 44, 255)
WOOD = (150, 96, 52, 255)
SACK = (190, 170, 120, 255)
OUTLINE = (24, 24, 28, 255)


def outline_cream(im):
    a = im.getchannel("A")
    ring = ImageChops.subtract(a.filter(ImageFilter.MaxFilter(7)), a).point(lambda v: 255 if v > 20 else 0).filter(ImageFilter.GaussianBlur(0.8))
    cream = Image.new("RGBA", im.size, (244, 232, 196, 255))
    cream.putalpha(ring)
    return Image.alpha_composite(Image.alpha_composite(Image.new("RGBA", im.size, (0, 0, 0, 0)), cream), im)


def ammo():
    im = Image.new("RGBA", (S * 4, S * 4), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    d.rounded_rectangle((120, 420, 900, 900), radius=40, fill=BOX, outline=OUTLINE, width=10)      # crate
    d.rectangle((120, 520, 900, 560), fill=BOX_DARK)
    d.rectangle((420, 420, 600, 900), fill=BOX_DARK)
    for i in range(5):                                                                              # cartridges standing in front
        x = 560 + i * 80
        d.rectangle((x, 220, x + 58, 440), fill=LEAD, outline=OUTLINE, width=6)
        d.polygon([(x, 220), (x + 58, 220), (x + 29, 120)], fill=LEAD, outline=OUTLINE)
        d.rectangle((x, 380, x + 58, 440), fill=BRASS)
    im = im.resize((S, S), Image.LANCZOS)
    outline_cream(im).save(os.path.join(OUT, "Icon_Ammo.png"))


def food():
    im = Image.new("RGBA", (S * 4, S * 4), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    d.polygon([(110, 540), (910, 540), (840, 900), (180, 900)], fill=WOOD, outline=OUTLINE)         # basket
    for k in range(5):
        d.line((200 + k * 150, 560, 230 + k * 140, 890), fill=(110, 70, 34, 255), width=12)
    d.ellipse((150, 330, 520, 600), fill=BREAD, outline=OUTLINE, width=10)                          # loaves
    d.arc((190, 360, 480, 560), 200, 340, fill=BREAD_DARK, width=14)
    d.ellipse((470, 300, 860, 600), fill=BREAD, outline=OUTLINE, width=10)
    d.arc((520, 330, 810, 560), 200, 340, fill=BREAD_DARK, width=14)
    d.ellipse((320, 200, 700, 470), fill=SACK, outline=OUTLINE, width=10)                           # a sack on top
    d.rectangle((470, 180, 550, 240), fill=SACK, outline=OUTLINE, width=8)
    im = im.resize((S, S), Image.LANCZOS)
    outline_cream(im).save(os.path.join(OUT, "Icon_Food.png"))


if __name__ == "__main__":
    ammo()
    food()
    print("ammo and food icons written to", os.path.normpath(OUT))
