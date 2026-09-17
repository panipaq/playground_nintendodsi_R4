#!/usr/bin/env python3
"""
Baut die "Deine Bestellung"-Uebersichtsbilder (oberer Bildschirm) fuer jede
Kombination aus Kaffeeart + Milch-Ja/Nein + Zucker-Ja/Nein. Siehe
bestellung.png (Skizze/Mockup) fuer das Ziel-Layout.

Ausfuehren:
    python3 tools/gen_bestellung.py

Erzeugt gfx/bestellung_<coffee>_<milk>_<sugar>.png fuer jede Kombination in
tools/kaffeedex_content.json (nur fuer Kaffeearten, bei denen die
Milch/Zucker-Frage ueberhaupt gestellt wird -- siehe
coffeeNeedsMilkSugarQuestion() in main.cpp).
"""

import json
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parent.parent
CONTENT_PATH = ROOT / "tools" / "kaffeedex_content.json"
BASE_TEMPLATE_PATH = ROOT / "sprites" / "bg-overview.png"  # fertige Karte + Kachel-Hintergrund
OUT_DIR = ROOT / "gfx"

FONT_PATH = "/System/Library/Fonts/Supplemental/Andale Mono.ttf"
AA_THRESHOLD = 70

DARK = (60, 30, 20, 255)
GRAY = (190, 180, 172, 255)  # fuer nicht ausgewaehlte Optionen (Milch/Zucker)

# Alle Kaffeearten durchlaufen die Milch/Zucker-Zusatzfrage -- Cappuccino hat
# aber schon Milch drin, darum wird dort keine Milch-Zeile angezeigt (siehe
# HAS_MILK) und im Spiel keine Milch-Karte gezeigt (siehe main.cpp).
MILK_SUGAR_DRINKS = ["americano", "capuccino", "filter"]
HAS_MILK = {"americano": True, "capuccino": False, "filter": True}

COFFEE_ICON_XY = (24, 70)
COFFEE_ICON_SCALE = 2

TITLE_XY_CENTER = (128, 16)
TITLE_SIZE = 13

NAME_XY = (118, 62)
NAME_SIZE = 13

MILK_ICON_XY = (118, 92)
MILK_ICON_SCALE = 1
MILK_LABEL_XY = (148, 95)

SUGAR_ICON_XY = (118, 134)
SUGAR_ICON_SCALE = 1
SUGAR_LABEL_XY = (148, 129)

LABEL_SIZE = 11


def hard_text_mask(canvas_size, text, font_size):
	font = ImageFont.truetype(FONT_PATH, font_size)
	tmp = Image.new("L", canvas_size, 0)
	d = ImageDraw.Draw(tmp)
	d.text((0, 0), text, fill=255, font=font)
	return tmp.point(lambda p: 255 if p > AA_THRESHOLD else 0), font


def paste_text(base, xy, text, font_size, color, canvas_size=(240, 24), center_x=None):
	mask, font = hard_text_mask(canvas_size, text, font_size)
	x, y = xy
	if center_x is not None:
		x = center_x - int(font.getlength(text)) // 2
	solid = Image.new("RGBA", canvas_size, color)
	base.paste(solid, (x, y), mask)


def load_icon_rgba(sprite_path, scale):
	im = Image.open(ROOT / sprite_path).convert("RGBA")
	bbox = im.getbbox()
	crop = im.crop(bbox)
	return crop.resize((crop.width * scale, crop.height * scale), Image.NEAREST)


def paste_icon(base, sprite_path, xy, scale=2, grayed=False):
	icon = load_icon_rgba(sprite_path, scale)
	if grayed:
		gray = Image.new("RGBA", icon.size, GRAY)
		icon = Image.composite(gray, Image.new("RGBA", icon.size, (0, 0, 0, 0)), icon.split()[3])
	base.paste(icon, xy, icon)


def build(key, entry, wants_milk, wants_sugar, out_path):
	base = Image.open(BASE_TEMPLATE_PATH).convert("RGBA")

	paste_text(base, (0, TITLE_XY_CENTER[1]), "Deine Bestellung", TITLE_SIZE, DARK, center_x=TITLE_XY_CENTER[0])

	paste_icon(base, f"sprites/{key}.png", COFFEE_ICON_XY, scale=COFFEE_ICON_SCALE)
	paste_text(base, NAME_XY, entry["name"], NAME_SIZE, DARK)

	if HAS_MILK[key]:
		paste_icon(base, "sprites/milk.png", MILK_ICON_XY, scale=MILK_ICON_SCALE, grayed=not wants_milk)
		paste_text(base, MILK_LABEL_XY, "Milch", LABEL_SIZE, DARK if wants_milk else GRAY)
		sugar_icon_xy, sugar_label_xy = SUGAR_ICON_XY, SUGAR_LABEL_XY
	else:
		# Keine Milch-Zeile -- Zucker ruecklt an deren Position hoch, statt
		# eine leere Luecke stehen zu lassen.
		sugar_icon_xy, sugar_label_xy = MILK_ICON_XY, MILK_LABEL_XY

	paste_icon(base, "sprites/sugar.png", sugar_icon_xy, scale=SUGAR_ICON_SCALE, grayed=not wants_sugar)
	paste_text(base, sugar_label_xy, "Zucker", LABEL_SIZE, DARK if wants_sugar else GRAY)

	base.save(out_path)
	print(f"geschrieben: {out_path}")


def main():
	content = json.loads(CONTENT_PATH.read_text())
	drinks = content["drinks"]
	for key in MILK_SUGAR_DRINKS:
		entry = drinks[key]
		for wants_milk in (False, True):
			for wants_sugar in (False, True):
				suffix = f"{'ja' if wants_milk else 'nein'}_{'ja' if wants_sugar else 'nein'}"
				build(key, entry, wants_milk, wants_sugar, OUT_DIR / f"bestellung_{key}_{suffix}.png")


if __name__ == "__main__":
	main()
