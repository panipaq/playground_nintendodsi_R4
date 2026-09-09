#include "coffeeinfo.h"

#include <stdio.h>
#include <string.h>

#define ICON_COL_WIDTH 8 // Icon-Sprite ist 64px = 8 Textzellen breit/hoch
#define INFO_COL 9        // Spalte, ab der Name/Kategorie/Badges/Stats beginnen
#define TOTAL_COLS 32

void initCoffeeInfoScreen() {
	// Zusaetzlich zur Bitmap-Hintergrundgrafik (BG3, VRAM_A) eine Text-Konsole
	// (BG0) und Sprites auf dem Hauptbildschirm aktivieren. BG_PALETTE (Haupt-
	// Engine) wird von der Bitmap-Grafik nicht benutzt (die liegt direkt als
	// 16bpp-Farbwerte vor), darum gibt es hier -- anders als beim Touchscreen
	// -- keine Palettenkollision zu beachten.
	videoSetMode(MODE_5_2D | DISPLAY_BG3_ACTIVE | DISPLAY_BG0_ACTIVE | DISPLAY_SPR_ACTIVE | DISPLAY_SPR_1D_LAYOUT);
	vramSetBankB(VRAM_B_MAIN_SPRITE);
	vramSetBankE(VRAM_E_MAIN_BG);

	consoleInit(NULL, 0, BgType_Text4bpp, BgSize_T_256x256, 30, 0, true, true);
	oamInit(&oamMain, SpriteMapping_1D_128, false);
}

u16* loadCoffeeInfoIconGfx(const void* tiles, unsigned int tilesLen, const void* pal, int paletteBank) {
	u16* gfx = oamAllocateGfx(&oamMain, SpriteSize_64x64, SpriteColorFormat_16Color);
	dmaCopy(tiles, gfx, tilesLen);
	dmaCopy(pal, SPRITE_PALETTE + paletteBank * 16, 16 * sizeof(u16));
	return gfx;
}

static void clearRow(int row) {
	iprintf("\x1b[%d;0H", row);
	for (int i = 0; i < TOTAL_COLS; i++) iprintf(" ");
}

void updateCoffeeInfo(const CoffeeInfo& info, u16* iconGfx, int iconPaletteBank) {
	oamSet(&oamMain, 0, 0, 0, 0, iconPaletteBank, SpriteSize_64x64, SpriteColorFormat_16Color,
		iconGfx, -1, false, false, false, false, false);
	oamUpdate(&oamMain);

	for (int row = 0; row < 16; row++) clearRow(row);

	iprintf("\x1b[0;%dH%s", INFO_COL, info.name);
	iprintf("\x1b[2;%dH%s", INFO_COL, info.category);
	iprintf("\x1b[4;%dH[%s] [%s]", INFO_COL, info.badge1, info.badge2);

	iprintf("\x1b[6;%dHKoffein:  %3d mg", INFO_COL, info.caffeineMg);
	iprintf("\x1b[7;%dH", INFO_COL);
	for (int i = INFO_COL; i < TOTAL_COLS; i++) iprintf(".");
	iprintf("\x1b[8;%dHMenge:    %3d ml", INFO_COL, info.volumeMl);

	// Beschreibungsbox (volle Breite) unterhalb von Icon und Stats.
	int boxTop = ICON_COL_WIDTH + 2;
	iprintf("\x1b[%d;0H+", boxTop);
	for (int i = 0; i < TOTAL_COLS - 2; i++) iprintf("-");
	iprintf("+");

	for (int i = 0; i < 4; i++) {
		iprintf("\x1b[%d;0H|", boxTop + 1 + i);
		const char* line = info.descLines[i];
		int len = line ? strlen(line) : 0;
		iprintf(" %s", line ? line : "");
		for (int pad = len; pad < TOTAL_COLS - 3; pad++) iprintf(" ");
		iprintf("|");
	}

	iprintf("\x1b[%d;0H+", boxTop + 5);
	for (int i = 0; i < TOTAL_COLS - 2; i++) iprintf("-");
	iprintf("+");
}
