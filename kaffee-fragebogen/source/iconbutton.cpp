#include "iconbutton.h"

#include <icon_frame_top_l.h>
#include <icon_frame_top_r.h>
#include <icon_frame_mid_l.h>
#include <icon_frame_mid_r.h>
#include <icon_frame_bottom_l.h>
#include <icon_frame_bottom_r.h>

#define PRESS_OFFSET_PX 2

static u16* loadPiece(const void* tiles, unsigned int tilesLen, SpriteSize size) {
	u16* gfx = oamAllocateGfx(&oamSub, size, SpriteColorFormat_16Color);
	dmaCopy(tiles, gfx, tilesLen);
	return gfx;
}

IconButtonFrameGfx loadIconFrameGfx(int paletteBank) {
	IconButtonFrameGfx frame;
	frame.paletteBank = paletteBank;

	frame.topL    = loadPiece(icon_frame_top_lTiles, icon_frame_top_lTilesLen, SpriteSize_64x32);
	frame.topR    = loadPiece(icon_frame_top_rTiles, icon_frame_top_rTilesLen, SpriteSize_16x32);
	frame.midL    = loadPiece(icon_frame_mid_lTiles, icon_frame_mid_lTilesLen, SpriteSize_64x32);
	frame.midR    = loadPiece(icon_frame_mid_rTiles, icon_frame_mid_rTilesLen, SpriteSize_16x32);
	frame.bottomL = loadPiece(icon_frame_bottom_lTiles, icon_frame_bottom_lTilesLen, SpriteSize_64x32);
	frame.bottomR = loadPiece(icon_frame_bottom_rTiles, icon_frame_bottom_rTilesLen, SpriteSize_16x32);

	// grit fuellt die Palette immer auf 256 Eintraege auf, auch bei 4bpp
	// (16 Farben). Nur die ersten 16 Eintraege in die eigene Bank kopieren,
	// sonst ueberschreibt das die nachfolgenden Paletten-Banken mit Nullen.
	// Alle sechs Rahmen-Teile teilen sich dieselbe Palette.
	dmaCopy(icon_frame_top_lPal, SPRITE_PALETTE_SUB + paletteBank * 16, 16 * sizeof(u16));

	return frame;
}

u16* loadIconGfx(const void* tiles, unsigned int tilesLen, const void* pal, int paletteBank) {
	u16* gfx = oamAllocateGfx(&oamSub, SpriteSize_64x64, SpriteColorFormat_16Color);
	dmaCopy(tiles, gfx, tilesLen);
	dmaCopy(pal, SPRITE_PALETTE_SUB + paletteBank * 16, 16 * sizeof(u16));
	return gfx;
}

static void setIconButtonOamPositions(const IconButton& b, int yOffset, bool hide) {
	int x = b.x;
	int y = b.y + yOffset;
	int bank = b.frame->paletteBank;
	int mainColX = x;
	int sideColX = x + ICON_BUTTON_MAIN_COL_WIDTH_PX;

	oamSet(&oamSub, b.oamId + 0, mainColX, y, 3, bank, SpriteSize_64x32, SpriteColorFormat_16Color,
		b.frame->topL, -1, false, hide, false, false, false);
	oamSet(&oamSub, b.oamId + 1, sideColX, y, 3, bank, SpriteSize_16x32, SpriteColorFormat_16Color,
		b.frame->topR, -1, false, hide, false, false, false);

	oamSet(&oamSub, b.oamId + 2, mainColX, y + ICON_BUTTON_ROW_HEIGHT_PX, 3, bank, SpriteSize_64x32, SpriteColorFormat_16Color,
		b.frame->midL, -1, false, hide, false, false, false);
	oamSet(&oamSub, b.oamId + 3, sideColX, y + ICON_BUTTON_ROW_HEIGHT_PX, 3, bank, SpriteSize_16x32, SpriteColorFormat_16Color,
		b.frame->midR, -1, false, hide, false, false, false);

	oamSet(&oamSub, b.oamId + 4, mainColX, y + 2 * ICON_BUTTON_ROW_HEIGHT_PX, 3, bank, SpriteSize_64x32, SpriteColorFormat_16Color,
		b.frame->bottomL, -1, false, hide, false, false, false);
	oamSet(&oamSub, b.oamId + 5, sideColX, y + 2 * ICON_BUTTON_ROW_HEIGHT_PX, 3, bank, SpriteSize_16x32, SpriteColorFormat_16Color,
		b.frame->bottomR, -1, false, hide, false, false, false);

	// Das Icon-Sprite ist 64x64, die Karte aber 80x96 -- daher hier ueber die
	// gesamte Kartenflaeche zentrieren, statt nur oben/links auszurichten.
	int iconX = x + (ICON_BUTTON_WIDTH_PX - 64) / 2;
	int iconY = y + (ICON_BUTTON_HEIGHT_PX - 64) / 2;
	oamSet(&oamSub, b.oamId + 6, iconX, iconY, 2, b.iconPaletteBank, SpriteSize_64x64, SpriteColorFormat_16Color,
		b.iconGfx, -1, false, hide, false, false, false);
}

void placeIconButton(IconButton& b, int oamId) {
	b.oamId = oamId;
	setIconButtonOamPositions(b, 0, false);
}

void updateIconButtonPress(const IconButton& b, bool pressed) {
	setIconButtonOamPositions(b, pressed ? PRESS_OFFSET_PX : 0, false);
}

void hideIconButton(const IconButton& b) {
	setIconButtonOamPositions(b, 0, true);
}

bool isTouchInIconButton(const IconButton& b, const touchPosition& touch) {
	return touch.px >= b.x && touch.px < b.x + ICON_BUTTON_WIDTH_PX &&
	       touch.py >= b.y && touch.py < b.y + ICON_BUTTON_HEIGHT_PX;
}
