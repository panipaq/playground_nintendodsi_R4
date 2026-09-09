#include <nds.h>
#include <stdio.h>

#include "spritebutton.h"
#include "iconbutton.h"
#include "bracket.h"
#include <cup.h>
#include <bg_top.h>
#include <bg_bottom.h>
#include <coffee_americano.h>
#include <coffee_capuccino.h>
#include <coffee_filter.h>

enum Screen {
	SCREEN_START,
	SCREEN_COFFEE_TYPE,
};

enum CoffeeFocus {
	FOCUS_CARD,
	FOCUS_CONFIRM,
};

static SpriteButton startButtons[] = {
	{ "Kaffee bestellen", 32, 64,  9, 0 },
	{ "Bibliothek",       32, 112, 9, 0 },
};
#define START_BUTTON_COUNT (sizeof(startButtons) / sizeof(startButtons[0]))

static const char* coffeeNames[] = { "Americano", "Cappuccino", "Filterkaffee" };

// { frame, iconGfx, iconPaletteBank, x, y, oamId }
// frame/iconGfx werden erst zur Laufzeit gesetzt (siehe main()).
static IconButton coffeeButtons[] = {
	{ nullptr, nullptr, 11, 4,   16, 0 },
	{ nullptr, nullptr, 12, 88,  16, 0 },
	{ nullptr, nullptr, 13, 172, 16, 0 },
};
#define COFFEE_BUTTON_COUNT (sizeof(coffeeButtons) / sizeof(coffeeButtons[0]))

static SpriteButton confirmButton = { "bestaetigen", 64, 136, 6, 0 };

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
	BracketGfx bracketGfx = loadBracketGfx();

	int oamId = 1; // 0 ist fuer die Kaffeetasse reserviert
	for (size_t i = 0; i < START_BUTTON_COUNT; i++) {
		oamId = placeSpriteButton(startButtons[i], buttonGfx, oamId);
	}
	int bracketOamId = oamId;
	oamId += 4;

	oamId = placeSpriteButton(confirmButton, buttonGfx, oamId);
	hideSpriteButton(confirmButton, buttonGfx);

	static IconButtonFrameGfx iconFrameGfx = loadIconFrameGfx(10);
	for (size_t i = 0; i < COFFEE_BUTTON_COUNT; i++) coffeeButtons[i].frame = &iconFrameGfx;

	coffeeButtons[0].iconGfx = loadIconGfx(coffee_americanoTiles, coffee_americanoTilesLen, coffee_americanoPal, coffeeButtons[0].iconPaletteBank);
	coffeeButtons[1].iconGfx = loadIconGfx(coffee_capuccinoTiles, coffee_capuccinoTilesLen, coffee_capuccinoPal, coffeeButtons[1].iconPaletteBank);
	coffeeButtons[2].iconGfx = loadIconGfx(coffee_filterTiles, coffee_filterTilesLen, coffee_filterPal, coffeeButtons[2].iconPaletteBank);
	for (size_t i = 0; i < COFFEE_BUTTON_COUNT; i++) {
		placeIconButton(coffeeButtons[i], oamId);
		oamId += 7; // 6 Rahmen-Teile + Icon
		hideIconButton(coffeeButtons[i]);
	}

	Screen screen = SCREEN_START;
	int pressedIndex = -1;
	int selectedIndex = 0;
	CoffeeFocus coffeeFocus = FOCUS_CARD;
	int coffeeCardIndex = 0;

	while (pmMainLoop()) {
		swiWaitForVBlank();
		scanKeys();

		if (keysDown() & KEY_START) break;

		if (screen == SCREEN_START) {
			if (keysDown() & KEY_UP) {
				selectedIndex = (selectedIndex - 1 + START_BUTTON_COUNT) % START_BUTTON_COUNT;
			}
			if (keysDown() & KEY_DOWN) {
				selectedIndex = (selectedIndex + 1) % START_BUTTON_COUNT;
			}

			if (keysDown() & KEY_A) {
				pressedIndex = selectedIndex;
			}

			if (keysDown() & KEY_TOUCH) {
				touchPosition touch;
				touchRead(&touch);

				for (size_t i = 0; i < START_BUTTON_COUNT; i++) {
					if (isTouchInSpriteButton(startButtons[i], touch)) {
						selectedIndex = i;
						pressedIndex = i;
						break;
					}
				}
			}

			bool confirmed = (keysUp() & (KEY_A | KEY_TOUCH)) && pressedIndex >= 0;

			if (keysUp() & (KEY_A | KEY_TOUCH)) {
				pressedIndex = -1;
			}

			for (size_t i = 0; i < START_BUTTON_COUNT; i++) {
				updateSpriteButtonPress(startButtons[i], buttonGfx, (int)i == pressedIndex);
			}
			updateSelectionBracketAt(startButtons[selectedIndex].x, startButtons[selectedIndex].y,
				spriteButtonWidth(startButtons[selectedIndex]), SPRITE_BUTTON_HEIGHT_PX, bracketGfx, bracketOamId);

			oamSet(&oamSub, 0, 192, 24, 0, 0, SpriteSize_64x32, SpriteColorFormat_256Color,
				cupGfx, -1, false, false, false, false, false);

			if (confirmed && selectedIndex == 0) {
				// "Kaffee bestellen" -> zur Kaffeeart-Auswahl wechseln.
				for (size_t i = 0; i < START_BUTTON_COUNT; i++) hideSpriteButton(startButtons[i], buttonGfx);
				oamSet(&oamSub, 0, 0, 0, 0, 0, SpriteSize_64x32, SpriteColorFormat_256Color,
					cupGfx, -1, false, true, false, false, false);

				screen = SCREEN_COFFEE_TYPE;
				coffeeFocus = FOCUS_CARD;
				coffeeCardIndex = 0;
				pressedIndex = -1;
			} else if (confirmed && selectedIndex == 1) {
				// Bibliothek hat noch keinen eigenen Screen.
				iprintf("\x1b[20;0HBibliothek: noch nicht verfuegbar");
			}
		} else if (screen == SCREEN_COFFEE_TYPE) {
			if (keysDown() & KEY_LEFT && coffeeFocus == FOCUS_CARD) {
				coffeeCardIndex = (coffeeCardIndex - 1 + COFFEE_BUTTON_COUNT) % COFFEE_BUTTON_COUNT;
			}
			if (keysDown() & KEY_RIGHT && coffeeFocus == FOCUS_CARD) {
				coffeeCardIndex = (coffeeCardIndex + 1) % COFFEE_BUTTON_COUNT;
			}
			if (keysDown() & KEY_DOWN) coffeeFocus = FOCUS_CONFIRM;
			if (keysDown() & KEY_UP) coffeeFocus = FOCUS_CARD;

			if (keysDown() & KEY_A) {
				pressedIndex = (coffeeFocus == FOCUS_CARD) ? coffeeCardIndex : (int)COFFEE_BUTTON_COUNT;
			}

			if (keysDown() & KEY_TOUCH) {
				touchPosition touch;
				touchRead(&touch);

				for (size_t i = 0; i < COFFEE_BUTTON_COUNT; i++) {
					if (isTouchInIconButton(coffeeButtons[i], touch)) {
						coffeeCardIndex = i;
						coffeeFocus = FOCUS_CARD;
						pressedIndex = i;
						break;
					}
				}
				if (isTouchInSpriteButton(confirmButton, touch)) {
					coffeeFocus = FOCUS_CONFIRM;
					pressedIndex = COFFEE_BUTTON_COUNT;
				}
			}

			bool confirmed = (keysUp() & (KEY_A | KEY_TOUCH)) && pressedIndex >= 0;

			if (keysUp() & (KEY_A | KEY_TOUCH)) {
				pressedIndex = -1;
			}

			for (size_t i = 0; i < COFFEE_BUTTON_COUNT; i++) {
				updateIconButtonPress(coffeeButtons[i], (int)i == pressedIndex);
			}
			updateSpriteButtonPress(confirmButton, buttonGfx, pressedIndex == (int)COFFEE_BUTTON_COUNT);

			if (coffeeFocus == FOCUS_CARD) {
				updateSelectionBracketAt(coffeeButtons[coffeeCardIndex].x, coffeeButtons[coffeeCardIndex].y,
					ICON_BUTTON_WIDTH_PX, ICON_BUTTON_HEIGHT_PX, bracketGfx, bracketOamId);
			} else {
				updateSelectionBracketAt(confirmButton.x, confirmButton.y,
					spriteButtonWidth(confirmButton), SPRITE_BUTTON_HEIGHT_PX, bracketGfx, bracketOamId);
			}

			if (confirmed && pressedIndex == (int)COFFEE_BUTTON_COUNT) {
				iprintf("\x1b[20;0HAusgewaehlt: %-20s", coffeeNames[coffeeCardIndex]);
			}

			if (keysDown() & KEY_B) {
				// Zurueck zum Start-Bildschirm.
				for (size_t i = 0; i < COFFEE_BUTTON_COUNT; i++) hideIconButton(coffeeButtons[i]);
				hideSpriteButton(confirmButton, buttonGfx);

				screen = SCREEN_START;
				selectedIndex = 0;
				pressedIndex = -1;

				for (size_t i = 0; i < START_BUTTON_COUNT; i++) placeSpriteButton(startButtons[i], buttonGfx, startButtons[i].oamBaseId);
			}
		}

		oamUpdate(&oamSub);
	}

	return 0;
}
