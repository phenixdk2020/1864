"""Tegner seks små farvede ikoner til rapporten efter slaget (kanon, morter, gevær, hest, vogn, fane).

    python Tools/Report/make_report_icons.py
Output: Reference/UI/ReportIcons/Icon_<Navn>.png (farver på gennemsigtig, 128x128) og Icons_preview.png.
Importeres med Content/Python/import_report_icons.py.
"""
import math
import os
from PIL import Image, ImageDraw

S = 128
OUT = os.path.join(os.path.dirname(__file__), "..", "..", "Reference", "UI", "ReportIcons")
IRON = (86, 94, 106, 255)
IRON_LIGHT = (140, 150, 164, 255)
BRONZE = (176, 128, 56, 255)
WOOD = (150, 96, 52, 255)
WOOD_DARK = (104, 64, 34, 255)
RED = (200, 36, 52, 255)
WHITE = (245, 245, 245, 255)
GREEN = (62, 110, 70, 255)
SKIN = (160, 108, 62, 255)
OUTLINE = (24, 24, 28, 255)


def canvas():
    im = Image.new("RGBA", (S * 4, S * 4), (0, 0, 0, 0))
    return im, ImageDraw.Draw(im)


def save(im, name):
    os.makedirs(OUT, exist_ok=True)
    im.resize((S, S), Image.LANCZOS).save(os.path.join(OUT, "Icon_%s.png" % name))


def wheel(d, cx, cy, r, rim=WOOD_DARK, spoke=WOOD, hub=IRON, spokes=8):
    d.ellipse((cx - r, cy - r, cx + r, cy + r), fill=rim)
    d.ellipse((cx - r * 0.78, cy - r * 0.78, cx + r * 0.78, cy + r * 0.78), fill=(0, 0, 0, 0))
    for i in range(spokes):
        a = i * math.pi * 2 / spokes
        d.line((cx, cy, cx + math.cos(a) * r * 0.85, cy + math.sin(a) * r * 0.85), fill=spoke, width=int(r * 0.14))
    d.ellipse((cx - r * 0.24, cy - r * 0.24, cx + r * 0.24, cy + r * 0.24), fill=hub)


def cannon():
    im, d = canvas()
    d.polygon([(70, 250), (470, 130), (490, 190), (90, 320)], fill=BRONZE, outline=OUTLINE)    # barrel
    d.polygon([(250, 215), (300, 200), (310, 235), (260, 250)], fill=(214, 168, 88, 255))      # highlight band
    d.ellipse((440, 110, 510, 210), fill=BRONZE, outline=OUTLINE)                              # muzzle swell
    d.ellipse((470, 140, 495, 180), fill=OUTLINE)                                              # bore
    d.polygon([(70, 250), (140, 330), (60, 360), (20, 300)], fill=WOOD_DARK)                   # trail
    wheel(d, 230, 340, 120)
    save(im, "Cannon")


def mortar():
    im, d = canvas()
    d.polygon([(190, 90), (330, 40), (400, 230), (260, 290)], fill=IRON, outline=OUTLINE)      # short fat barrel
    d.ellipse((180, 55, 350, 125), fill=OUTLINE)                                               # mouth
    d.ellipse((200, 70, 330, 115), fill=(40, 40, 46, 255))
    d.rounded_rectangle((90, 320, 430, 420), radius=30, fill=WOOD, outline=OUTLINE)            # base plate
    d.rectangle((120, 345, 400, 360), fill=WOOD_DARK)
    save(im, "Mortar")


def rifle():
    im, d = canvas()
    d.polygon([(20, 330), (60, 270), (190, 300), (190, 380), (60, 400)], fill=WOOD, outline=OUTLINE)   # stock
    d.rectangle((170, 295, 440, 345), fill=IRON, outline=OUTLINE)                                      # barrel
    d.rectangle((170, 295, 440, 312), fill=IRON_LIGHT)
    d.polygon([(440, 300), (505, 215), (520, 232), (460, 345)], fill=IRON_LIGHT, outline=OUTLINE)      # bayonet
    d.rectangle((250, 345, 280, 410), fill=IRON)                                                       # trigger guard
    d.rectangle((300, 265, 330, 295), fill=IRON)                                                       # sight
    save(im, "Rifle")


def horse():
    im, d = canvas()
    d.polygon([(110, 440), (150, 300), (130, 200), (190, 90), (260, 60), (330, 90), (420, 200),
               (470, 290), (440, 330), (380, 300), (330, 330), (310, 440), (240, 440), (230, 340),
               (200, 440)], fill=SKIN, outline=OUTLINE)                                    # head and neck
    d.polygon([(260, 60), (275, 10), (305, 70)], fill=SKIN, outline=OUTLINE)               # ear
    d.polygon([(130, 200), (190, 90), (215, 130), (170, 260)], fill=WOOD_DARK)             # mane
    d.ellipse((312, 140, 342, 172), fill=OUTLINE)                                           # eye
    d.ellipse((425, 290, 450, 315), fill=WOOD_DARK)                                         # nostril
    save(im, "Horse")


def wagon():
    im, d = canvas()
    d.rounded_rectangle((60, 120, 450, 280), radius=18, fill=WOOD, outline=OUTLINE)        # bed
    d.rectangle((90, 70, 250, 130), fill=GREEN, outline=OUTLINE)                           # crates
    d.rectangle((270, 90, 410, 130), fill=(110, 80, 40, 255), outline=OUTLINE)
    d.rectangle((450, 245, 500, 265), fill=WOOD_DARK)                                       # shaft
    wheel(d, 160, 350, 85, rim=RED, spoke=WOOD_DARK)
    wheel(d, 370, 350, 85, rim=RED, spoke=WOOD_DARK)
    save(im, "Wagon")


def flag():
    im, d = canvas()
    d.rectangle((90, 40, 118, 470), fill=WOOD_DARK, outline=OUTLINE)                       # pole
    d.polygon([(118, 60), (440, 60), (400, 160), (440, 260), (118, 260)], fill=RED, outline=OUTLINE)
    d.rectangle((200, 60, 235, 260), fill=WHITE)                                            # the Dannebrog cross
    d.rectangle((118, 140, 440, 175), fill=WHITE)
    d.ellipse((84, 20, 124, 60), fill=BRONZE)
    save(im, "Flag")


if __name__ == "__main__":
    for f in (cannon, mortar, rifle, horse, wagon, flag):
        f()
    names = ["Cannon", "Mortar", "Rifle", "Horse", "Wagon", "Flag"]
    sheet = Image.new("RGBA", (S * 6 + 70, S + 20), (36, 46, 62, 255))
    for i, n in enumerate(names):
        ic = Image.open(os.path.join(OUT, "Icon_%s.png" % n))
        sheet.paste(ic, (10 + i * (S + 10), 10), ic)
    sheet = sheet.resize((sheet.width * 2, sheet.height * 2), Image.LANCZOS)
    sheet.save(os.path.join(OUT, "Icons_preview.png"))
    print("ikoner skrevet til", os.path.normpath(OUT))
