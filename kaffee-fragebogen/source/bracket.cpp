#include "bracket.h"

#include <bracket_tl.h>
#include <bracket_tr.h>
#include <bracket_bl.h>
#include <bracket_br.h>

// Eigene Bank fuer die Klammer-Grafik, getrennt von anderen Sprites.
#define BRACKET_PALETTE_BANK 14
#define BRACKET_SIZE_PX 16
#define BRACKET_INSET_PX 4 // wie weit die Klammern ueber die Kante hinausragen

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

void updateSelectionBracketAt(int x, int y, int width, int height, const BracketGfx& gfx, int oamBaseId) {
	int left   = x - BRACKET_INSET_PX;
	int right  = x + width + BRACKET_INSET_PX - BRACKET_SIZE_PX;
	int top    = y - BRACKET_INSET_PX;
	int bottom = y + height + BRACKET_INSET_PX - BRACKET_SIZE_PX;

	// Prioritaet 1: vor den Buttons (Prioritaet 3/2), damit die Klammer
	// sichtbar ueber dem Rand liegt.
	oamSet(&oamSub, oamBaseId + 0, left, top, 1, BRACKET_PALETTE_BANK, SpriteSize_16x16, SpriteColorFormat_16Color,
		gfx.topLeft, -1, false, false, false, false, false);
	oamSet(&oamSub, oamBaseId + 1, right, top, 1, BRACKET_PALETTE_BANK, SpriteSize_16x16, SpriteColorFormat_16Color,
		gfx.topRight, -1, false, false, false, false, false);
	oamSet(&oamSub, oamBaseId + 2, left, bottom, 1, BRACKET_PALETTE_BANK, SpriteSize_16x16, SpriteColorFormat_16Color,
		gfx.bottomLeft, -1, false, false, false, false, false);
	oamSet(&oamSub, oamBaseId + 3, right, bottom, 1, BRACKET_PALETTE_BANK, SpriteSize_16x16, SpriteColorFormat_16Color,
		gfx.bottomRight, -1, false, false, false, false, false);
}
