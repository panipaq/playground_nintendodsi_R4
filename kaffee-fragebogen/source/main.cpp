#include <nds.h>
#include <stdio.h>

#include "spritebutton.h"
#include <cup.h>
#include <bg_top.h>
#include <bg_bottom.h>

static SpriteButton buttons[] = {
	{ "Kaffee bestellen", 32, 64,  9, 0 },
	{ "Bibliothek",       32, 112, 9, 0 },
};
#define BUTTON_COUNT (sizeof(buttons) / sizeof(buttons[0]))

int main(void) {
	// Oberer Bildschirm: reines Hintergrundbild (16bpp Bitmap, BG3).
	videoSetMode(MODE_5_2D | DISPLAY_BG3_ACTIVE);
	vramSetBankA(VRAM_A_MAIN_BG);
	int bgTop = bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
	dmaCopy(bg_topBitmap, bgGetGfxPtr(bgTop), bg_topBitmapLen);

	consoleDemoInit();

	// Sprites auf dem Touchscreen (sub display) zusaetzlich zur Text-Konsole aktivieren.
	videoSetModeSub(MODE_0_2D | DISPLAY_BG0_ACTIVE | DISPLAY_BG1_ACTIVE | DISPLAY_SPR_ACTIVE | DISPLAY_SPR_1D_LAYOUT);
	vramSetBankD(VRAM_D_SUB_SPRITE);
	oamInit(&oamSub, SpriteMapping_1D_128, false);

	// Hintergrundbild fuer den Touchscreen (BG1, hinter Konsolentext BG0 und
	// den Sprites). tileBase/mapBase sind bewusst so gewaehlt, dass sie
	// nicht mit dem Bereich kollidieren, den consoleDemoInit fuer die
	// Text-Konsole in VRAM_C belegt (Tiles ab Byte 0, Map bei mapBase 30).
	int bgBottom = bgInitSub(1, BgType_Text8bpp, BgSize_T_256x256, 16, 4);
	dmaCopy(bg_bottomTiles, bgGetGfxPtr(bgBottom), bg_bottomTilesLen);
	dmaCopy(bg_bottomMap, bgGetMapPtr(bgBottom), bg_bottomMapLen);
	dmaCopy(bg_bottomPal, BG_PALETTE_SUB, bg_bottomPalLen);
	bgSetPriority(bgBottom, 3);

	// Der Konsolentext (4bpp) teilt sich BG_PALETTE_SUB mit dem 8bpp-
	// Hintergrundbild. bg_bottom nutzt nur die Indizes 0-95 (siehe grit-
	// Ausgabe), darum ist Palettenbank 7 (Indizes 112-127) sicher frei fuer
	// eigene Textfarben. ANSI-Code "37" waehlt genau diese Bank aus.
	BG_PALETTE_SUB[7 * 16 + 15] = RGB8(0xFF, 0xCA, 0x86);

	u16* cupGfx = oamAllocateGfx(&oamSub, SpriteSize_64x32, SpriteColorFormat_256Color);
	dmaCopy(cupTiles, cupGfx, cupTilesLen);
	dmaCopy(cupPal, SPRITE_PALETTE_SUB, cupPalLen);

	SpriteButtonGfx buttonGfx = loadSpriteButtonGfx();

	int oamId = 1; // 0 ist fuer die Kaffeetasse reserviert
	for (size_t i = 0; i < BUTTON_COUNT; i++) {
		oamId = placeSpriteButton(buttons[i], buttonGfx, oamId);
	}

	int pressedIndex = -1;
	int selectedIndex = 0;

	while (pmMainLoop()) {
		swiWaitForVBlank();
		scanKeys();

		if (keysDown() & KEY_START) break;

		if (keysDown() & KEY_UP) {
			selectedIndex = (selectedIndex - 1 + BUTTON_COUNT) % BUTTON_COUNT;
		}
		if (keysDown() & KEY_DOWN) {
			selectedIndex = (selectedIndex + 1) % BUTTON_COUNT;
		}

		if (keysDown() & KEY_A) {
			pressedIndex = selectedIndex;
			iprintf("\x1b[20;0HGedrueckt: %-20s", buttons[selectedIndex].label);
		}
		if (keysUp() & KEY_A) {
			pressedIndex = -1;
		}

		if (keysDown() & KEY_TOUCH) {
			touchPosition touch;
			touchRead(&touch);

			for (size_t i = 0; i < BUTTON_COUNT; i++) {
				if (isTouchInSpriteButton(buttons[i], touch)) {
					selectedIndex = i;
					pressedIndex = i;
					iprintf("\x1b[20;0HGedrueckt: %-20s", buttons[i].label);
					break;
				}
			}
		}

		if (keysUp() & KEY_TOUCH) {
			pressedIndex = -1;
		}

		for (size_t i = 0; i < BUTTON_COUNT; i++) {
			updateSpriteButtonPress(buttons[i], buttonGfx, (int)i == pressedIndex);
			drawSelectionMarker(buttons[i], (int)i == selectedIndex);
		}

		oamSet(&oamSub, 0, 192, 24, 0, 0, SpriteSize_64x32, SpriteColorFormat_256Color,
			cupGfx, -1, false, false, false, false, false);
		oamUpdate(&oamSub);
	}

	return 0;
}
