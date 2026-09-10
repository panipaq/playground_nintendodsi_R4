#!/usr/bin/env python3
"""
Baut die drei Kaffeedex-Bilder (Pokedex-Stil, oberer Bildschirm) aus dem
leeren Template `sprites/kaffeedex.png` + Text/Icon fuer jede Kaffeeart.

Texte aendern: tools/kaffeedex_content.json bearbeiten, dann dieses Skript
neu ausfuehren:

    python3 tools/gen_kaffeedex.py

Erzeugt gfx/kaffeedex_<name>.png fuer jeden Eintrag in der JSON-Datei.
Danach `make` im Projekt neu ausfuehren, damit grit die neuen PNGs
einbindet.
"""

import json
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parent.parent
TEMPLATE_PATH = ROOT / "sprites" / "kaffeedex.png"
CONTENT_PATH = ROOT / "tools" / "kaffeedex_content.json"
OUT_DIR = ROOT / "gfx"

FONT_PATH = "/System/Library/Fonts/Supplemental/Andale Mono.ttf"
AA_THRESHOLD = 70  # niedrig halten, sonst verschwinden duenne Striche (i-Punkte etc.)

DARK = (60, 30, 20, 255)
ORANGE_TEXT = (255, 250, 240, 255)

# Layout-Koordinaten fuer sprites/kaffeedex.png (256x193). Bei einem neuen
# Template-Bild hier die Werte anpassen (siehe Analyse im Chat-Verlauf).
ICON_XY = (20, 27)  # oben links, im Rasterbereich links von Box 2
NAME_XY = (112, 13)
NAME_SIZE = 13
CATEGORY_XY = (112, 30)
BADGE1_XY = (110, 54)
BADGE2_RIGHT_EDGE = 245
BADGE_Y = 54
STATS_X = 150
KOFFEIN_Y = 82
MENGE_Y = 98
DESC_XY = (18, 121)
DESC_LINE_HEIGHT = 15
BODY_SIZE = 9


def hard_text_mask(canvas_size, text, font_size):
    font = ImageFont.truetype(FONT_PATH, font_size)
    tmp = Image.new("L", canvas_size, 0)
    d = ImageDraw.Draw(tmp)
    d.text((0, 0), text, fill=255, font=font)
    return tmp.point(lambda p: 255 if p > AA_THRESHOLD else 0), font


def paste_text(base, xy, text, font_size, color, canvas_size=(240, 24), right_edge=None):
    mask, font = hard_text_mask(canvas_size, text, font_size)
    x, y = xy
    if right_edge is not None:
        x = right_edge - int(font.getlength(text))
    solid = Image.new("RGBA", canvas_size, color)
    base.paste(solid, (x, y), mask)


def build(entry, out_path, labels):
    base = Image.open(TEMPLATE_PATH).convert("RGBA")

    icon = Image.open(ROOT / entry["icon"]).convert("RGBA")
    ipx = icon.load()
    for y in range(icon.height):
        for x in range(icon.width):
            r, g, b, a = ipx[x, y]
            if r > 240 and g < 30 and b > 240:  # Magenta-Transparenzfarbe
                ipx[x, y] = (r, g, b, 0)
    base.paste(icon, ICON_XY, icon)

    paste_text(base, NAME_XY, entry["name"], NAME_SIZE, ORANGE_TEXT)
    paste_text(base, CATEGORY_XY, entry["category"], BODY_SIZE, DARK)

    paste_text(base, BADGE1_XY, f"[{entry['badge1']}]", BODY_SIZE, DARK)
    paste_text(base, (0, BADGE_Y), f"[{entry['badge2']}]", BODY_SIZE, DARK, right_edge=BADGE2_RIGHT_EDGE)

    label_width = max(len(labels["caffeine"]), len(labels["volume"])) + 1  # +1 fuer ":"
    caffeine_label = f"{labels['caffeine']}:".ljust(label_width)
    volume_label = f"{labels['volume']}:".ljust(label_width)
    paste_text(base, (STATS_X, KOFFEIN_Y), f"{caffeine_label} {entry['caffeineMg']:>3} mg", BODY_SIZE, DARK)
    paste_text(base, (STATS_X, MENGE_Y), f"{volume_label} {entry['volumeMl']:>3} ml", BODY_SIZE, DARK)

    x0, y0 = DESC_XY
    for i, line in enumerate(entry["descLines"]):
        if line:
            paste_text(base, (x0, y0 + i * DESC_LINE_HEIGHT), line, BODY_SIZE, DARK)

    base.save(out_path)
    print(f"geschrieben: {out_path}")


def main():
    content = json.loads(CONTENT_PATH.read_text())
    labels = content["labels"]
    for key, entry in content["drinks"].items():
        build(entry, OUT_DIR / f"kaffeedex_{key}.png", labels)


if __name__ == "__main__":
    main()
