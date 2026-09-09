#include "spritebutton.h"

#include <stdio.h>
#include <string.h>

#include <btn_left.h>
#include <btn_mid.h>
#include <btn_right.h>
#include <bracket_tl.h>
#include <bracket_tr.h>
#include <bracket_bl.h>
#include <bracket_br.h>

#define CAP_WIDTH_PX 16
#define CONTENT_HEIGHT_PX 27 // sichtbarer Bereich der Grafik, Rest bis 32px ist transparent
#define CELL_PX 8
// Nur die Grafik bewegt sich beim Druecken (dezent, 2px). Der Text muesste
// sonst um eine ganze Zeile (8px) springen, das wirkt bei so einer kurzen
// Bewegung eher abrupt als wie ein Druck-Effekt.
#define PRESS_OFFSET_PX 2

// Buttons nutzen 4bpp (16 Farben) mit einer eigenen Palettenbank, damit sie
// sich die globale Sprite-Palette (256 Eintraege, 8bpp) nicht mit anderen
// Sprites wie der Kaffeetasse teilen und deren Farben ueberschreiben.
#define BUTTON_PALETTE_BANK 15

// Eigene Bank fuer die Klammer-Grafik, getrennt von den Buttons (Bank 15)
// und der Kaffeetasse (8bpp, Indizes 0-75).
#define BRACKET_PALETTE_BANK 14
#define BRACKET_SIZE_PX 16
#define BRACKET_INSET_PX 4 // wie weit die Klammern ueber die Button-Kante hinausragen

SpriteButtonGfx loadSpriteButtonGfx() {
	SpriteButtonGfx gfx;
	gfx.left  = oamAllocateGfx(&oamSub, SpriteSize_16x32, SpriteColorFormat_16Color);
	gfx.mid   = oamAllocateGfx(&oamSub, SpriteSize_16x32, SpriteColorFormat_16Color);
	gfx.right = oamAllocateGfx(&oamSub, SpriteSize_16x32, SpriteColorFormat_16Color);

	dmaCopy(btn_leftTiles, gfx.left, btn_leftTilesLen);
	dmaCopy(btn_midTiles, gfx.mid, btn_midTilesLen);
	dmaCopy(btn_rightTiles, gfx.right, btn_rightTilesLen);

	// Alle drei Teile teilen sich exakt dieselbe 16-Farben-Palette.
	dmaCopy(btn_leftPal, SPRITE_PALETTE_SUB + BUTTON_PALETTE_BANK * 16, 16 * sizeof(u16));

	return gfx;
}

int spriteButtonWidth(const SpriteButton& b) {
	return CAP_WIDTH_PX * 2 + b.middleTiles * CAP_WIDTH_PX;
}

static void setButtonOamPositions(const SpriteButton& b, const SpriteButtonGfx& gfx, int yOffset) {
	int x = b.x;
	int y = b.y + yOffset;
	int oamId = b.oamBaseId;

	oamSet(&oamSub, oamId++, x, y, 3, BUTTON_PALETTE_BANK, SpriteSize_16x32, SpriteColorFormat_16Color,
		gfx.left, -1, false, false, false, false, false);
	x += CAP_WIDTH_PX;

	for (int i = 0; i < b.middleTiles; i++) {
		oamSet(&oamSub, oamId++, x, y, 3, BUTTON_PALETTE_BANK, SpriteSize_16x32, SpriteColorFormat_16Color,
			gfx.mid, -1, false, false, false, false, false);
		x += CAP_WIDTH_PX;
	}

	oamSet(&oamSub, oamId++, x, y, 3, BUTTON_PALETTE_BANK, SpriteSize_16x32, SpriteColorFormat_16Color,
		gfx.right, -1, false, false, false, false, false);
}

static int labelRow(const SpriteButton& b) {
	return b.y / CELL_PX + (CONTENT_HEIGHT_PX / CELL_PX) / 2;
}

static int labelCol(const SpriteButton& b) {
	int widthCells = spriteButtonWidth(b) / CELL_PX;
	int labelLen = strlen(b.label);
	int pad = (widthCells - labelLen) / 2;
	if (pad < 0) pad = 0;
	return b.x / CELL_PX + pad;
}

// "\x1b[30;1m" = Bank 8 (Druck-Effekt, unbenutzt->schwarz), "\x1b[37m" = Bank 7
// (per main.cpp mit #ffca86 belegt). Bank 0 ("39"/Standard) vermeiden wir hier
// bewusst, die kollidiert mit der Palette des Hintergrundbilds (BG1).
static void drawLabel(const SpriteButton& b, bool pressed) {
	iprintf("\x1b[%d;%dH\x1b[%sm%s\x1b[39m",
		labelRow(b), labelCol(b), pressed ? "30;1" : "37", b.label);
}

int placeSpriteButton(SpriteButton& b, const SpriteButtonGfx& gfx, int oamId) {
	b.oamBaseId = oamId;
	int nextOamId = oamId + 2 + b.middleTiles;

	setButtonOamPositions(b, gfx, 0);
	drawLabel(b, false);

	return nextOamId;
}

void updateSpriteButtonPress(const SpriteButton& b, const SpriteButtonGfx& gfx, bool pressed) {
	setButtonOamPositions(b, gfx, pressed ? PRESS_OFFSET_PX : 0);
	drawLabel(b, pressed);
}

bool isTouchInSpriteButton(const SpriteButton& b, const touchPosition& touch) {
	int width = spriteButtonWidth(b);

	return touch.px >= b.x && touch.px < b.x + width &&
	       touch.py >= b.y && touch.py < b.y + CONTENT_HEIGHT_PX;
}

BracketGfx loadBracketGfx() {
	BracketGfx gfx;
	gfx.topLeft     = oamAllocateGfx(&oamSub, SpriteSize_16x16, SpriteColorFormat_16Color);
	gfx.topRight    = oamAllocateGfx(&oamSub, SpriteSize_16x16, SpriteColorFormat_16Color);
	gfx.bottomLeft  = oamAllocateGfx(&oamSub, SpriteSize_16x16, SpriteColorFormat_16Color);
	gfx.bottomRight = oamAllocateGfx(&oamSub, SpriteSize_16x16, SpriteColorFormat_16Color);

	dmaCopy(bracket_tlTiles, gfx.topLeft, bracket_tlTilesLen);
	dmaCopy(bracket_trTiles, gfx.topRight, bracket_trTilesLen);
	dmaCopy(bracket_blTiles, gfx.bottomLeft, bracket_blTilesLen);
	dmaCopy(bracket_brTiles, gfx.bottomRight, bracket_brTilesLen);

	// Alle vier Ecken teilen sich dieselbe Palette.
	dmaCopy(bracket_tlPal, SPRITE_PALETTE_SUB + BRACKET_PALETTE_BANK * 16, 16 * sizeof(u16));

	return gfx;
}

void updateSelectionBracket(const SpriteButton& b, const BracketGfx& gfx, int oamBaseId) {
	int width = spriteButtonWidth(b);

	int left   = b.x - BRACKET_INSET_PX;
	int right  = b.x + width + BRACKET_INSET_PX - BRACKET_SIZE_PX;
	int top    = b.y - BRACKET_INSET_PX;
	int bottom = b.y + CONTENT_HEIGHT_PX + BRACKET_INSET_PX - BRACKET_SIZE_PX;

	// Prioritaet 1: vor der Button-Grafik (Prioritaet 3), damit die Klammer
	// sichtbar ueber dem Button-Rand liegt.
	oamSet(&oamSub, oamBaseId + 0, left, top, 1, BRACKET_PALETTE_BANK, SpriteSize_16x16, SpriteColorFormat_16Color,
		gfx.topLeft, -1, false, false, false, false, false);
	oamSet(&oamSub, oamBaseId + 1, right, top, 1, BRACKET_PALETTE_BANK, SpriteSize_16x16, SpriteColorFormat_16Color,
		gfx.topRight, -1, false, false, false, false, false);
	oamSet(&oamSub, oamBaseId + 2, left, bottom, 1, BRACKET_PALETTE_BANK, SpriteSize_16x16, SpriteColorFormat_16Color,
		gfx.bottomLeft, -1, false, false, false, false, false);
	oamSet(&oamSub, oamBaseId + 3, right, bottom, 1, BRACKET_PALETTE_BANK, SpriteSize_16x16, SpriteColorFormat_16Color,
		gfx.bottomRight, -1, false, false, false, false, false);
}
