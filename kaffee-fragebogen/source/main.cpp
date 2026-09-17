#include <nds.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fat.h>

#include "spritebutton.h"
#include "iconbutton.h"
#include "bracket.h"
#include <cup.h>
#include <bg_top.h>
#include <bg_bottom.h>
#include <coffee_americano.h>
#include <coffee_capuccino.h>
#include <coffee_filter.h>
#include <kaffeedex_americano.h>
#include <kaffeedex_capuccino.h>
#include <kaffeedex_filter.h>
#include <milk.h>
#include <sugar.h>
#include <bestellung_americano_nein_nein.h>
#include <bestellung_americano_nein_ja.h>
#include <bestellung_americano_ja_nein.h>
#include <bestellung_americano_ja_ja.h>
#include <bestellung_capuccino_nein_nein.h>
#include <bestellung_capuccino_nein_ja.h>
#include <bestellung_capuccino_ja_nein.h>
#include <bestellung_capuccino_ja_ja.h>
#include <bestellung_filter_nein_nein.h>
#include <bestellung_filter_nein_ja.h>
#include <bestellung_filter_ja_nein.h>
#include <bestellung_filter_ja_ja.h>

enum Screen {
	SCREEN_START,
	SCREEN_COFFEE_TYPE,
	SCREEN_MILK_SUGAR,
	SCREEN_DRAWING,
	SCREEN_LIBRARY,
};

enum CoffeeFocus {
	FOCUS_CARD,
	FOCUS_CONFIRM,
};

// Milch/Zucker-Zusatzfrage: alle Kaffeearten durchlaufen sie, aber
// Cappuccino hat schon Milch drin und zeigt darum keine Milch-Karte an
// (siehe milkSugarHasMilk()).
enum MilkSugarFocus {
	FOCUS_MILK,
	FOCUS_SUGAR,
	FOCUS_MS_CONFIRM,
};

static bool milkSugarHasMilk(int coffeeCardIndex) {
	return coffeeCardIndex != 1; // 1 = Cappuccino
}

static SpriteButton startButtons[] = {
	{ "Kaffee bestellen", 32, 64,  9, 0 },
	{ "Bibliothek",       32, 112, 9, 0 },
};
#define START_BUTTON_COUNT (sizeof(startButtons) / sizeof(startButtons[0]))

// Statt den Infotext zur Laufzeit zu zeichnen (Konsole + Sprite-Icon auf dem
// Hauptbildschirm), ist die komplette Infoseite pro Kaffeeart fertig als
// Bitmap gerendert (siehe gfx/kaffeedex_*.png) und wird beim Durchblaettern
// einfach als kompletter Bildspeicher-Block in den BG3-Grafikspeicher
// kopiert. Das ist deutlich robuster als das vorherige dynamische Setup.
struct KaffeedexBitmap {
	const void* data;
	unsigned int len;
};
static const KaffeedexBitmap kaffeedexBitmaps[] = {
	{ kaffeedex_americanoBitmap, kaffeedex_americanoBitmapLen },
	{ kaffeedex_capuccinoBitmap, kaffeedex_capuccinoBitmapLen },
	{ kaffeedex_filterBitmap, kaffeedex_filterBitmapLen },
};

// "Deine Bestellung"-Uebersicht (oberer Bildschirm) fuer die Milch/Zucker-
// Zusatzfrage, live aktualisiert je nachdem was gerade angeklickt ist.
// Indiziert als [coffeeCardIndex] x [wantsMilk] x [wantsSugar]. Bei
// Cappuccino (Index 1) ist wantsMilk immer false, da es dort keine
// Milch-Karte gibt (siehe milkSugarHasMilk()) -- die "milk=true"-Bilder
// werden fuer diese Kaffeeart also nie angezeigt.
static const KaffeedexBitmap bestellungBitmaps[3][2][2] = {
	{ // Americano
		{ { bestellung_americano_nein_neinBitmap, bestellung_americano_nein_neinBitmapLen },
		  { bestellung_americano_nein_jaBitmap, bestellung_americano_nein_jaBitmapLen } },
		{ { bestellung_americano_ja_neinBitmap, bestellung_americano_ja_neinBitmapLen },
		  { bestellung_americano_ja_jaBitmap, bestellung_americano_ja_jaBitmapLen } },
	},
	{ // Cappuccino
		{ { bestellung_capuccino_nein_neinBitmap, bestellung_capuccino_nein_neinBitmapLen },
		  { bestellung_capuccino_nein_jaBitmap, bestellung_capuccino_nein_jaBitmapLen } },
		{ { bestellung_capuccino_ja_neinBitmap, bestellung_capuccino_ja_neinBitmapLen },
		  { bestellung_capuccino_ja_jaBitmap, bestellung_capuccino_ja_jaBitmapLen } },
	},
	{ // Filterkaffee
		{ { bestellung_filter_nein_neinBitmap, bestellung_filter_nein_neinBitmapLen },
		  { bestellung_filter_nein_jaBitmap, bestellung_filter_nein_jaBitmapLen } },
		{ { bestellung_filter_ja_neinBitmap, bestellung_filter_ja_neinBitmapLen },
		  { bestellung_filter_ja_jaBitmap, bestellung_filter_ja_jaBitmapLen } },
	},
};

static void showBestellungBitmap(int bgTop, int coffeeCardIndex, bool wantsMilk, bool wantsSugar) {
	const KaffeedexBitmap& bmp = bestellungBitmaps[coffeeCardIndex][wantsMilk][wantsSugar];
	dmaCopy(bmp.data, bgGetGfxPtr(bgTop), bmp.len);
}

// { frame, iconGfx, iconPaletteBank, x, y, oamId }
// frame/iconGfx werden erst zur Laufzeit gesetzt (siehe main()).
static IconButton coffeeButtons[] = {
	{ nullptr, nullptr, 11, 4,   16, 0 },
	{ nullptr, nullptr, 12, 88,  16, 0 },
	{ nullptr, nullptr, 13, 172, 16, 0 },
};
#define COFFEE_BUTTON_COUNT (sizeof(coffeeButtons) / sizeof(coffeeButtons[0]))

static SpriteButton confirmButton = { "bestaetigen", 64, 136, 6, 0 };

// Weisse Icon-Karten fuer die Milch/Zucker-Zusatzfrage, im selben Stil wie
// die Kaffeeart-Auswahl (siehe bestellung-unten.png). Reihenfolge der Felder:
// { frame, iconGfx, iconPaletteBank, x, y, oamId }.
static IconButton milkCard  = { nullptr, nullptr, 9, 40, 24, 0 };
static IconButton sugarCard = { nullptr, nullptr, 8, 136, 24, 0 };
static SpriteButton msConfirmButton = { "bestaetigen", 64, 152, 6, 0 };

// Mal-Screen: Leinwand links (0,0)-(128,128), Buttons rechts daneben.
enum DrawFocus {
	DRAW_FOCUS_CLEAR,
	DRAW_FOCUS_DONE,
};
static SpriteButton drawClearButton = { "loeschen",  30, 145, 3, 0 };
static SpriteButton drawDoneButton  = { "absenden", 140, 145, 3, 0 };

#define MILK_SUGAR_CARD_WIDTH_COLS (ICON_BUTTON_WIDTH_PX / 8) // 10
#define SUGAR_CARD_X_WITH_MILK 136
#define SUGAR_CARD_X_CENTERED  88 // wenn keine Milch-Karte da ist (Cappuccino)
#define MILK_SUGAR_LABEL_ROW 15

// Zentriert ein Label (in Konsolenspalten) unter einer Karte an Pixel-x.
static int labelColFor(int cardX, int labelLen) {
	return cardX / 8 + (MILK_SUGAR_CARD_WIDTH_COLS - labelLen) / 2;
}

static void drawMilkSugarLabels(bool withMilk = true) {
	if (withMilk) {
		iprintf("\x1b[%d;%dHMilch", MILK_SUGAR_LABEL_ROW, labelColFor(milkCard.x, 5));
	}
	iprintf("\x1b[%d;%dHZucker", MILK_SUGAR_LABEL_ROW, labelColFor(sugarCard.x, 6));
}

static void clearMilkSugarLabels() {
	iprintf("\x1b[%d;%dH      ", MILK_SUGAR_LABEL_ROW, labelColFor(milkCard.x, 5));
	iprintf("\x1b[%d;%dH      ", MILK_SUGAR_LABEL_ROW, labelColFor(sugarCard.x, 6));
}

// Mal-Screen: Zeichnungen werden dauerhaft auf der SD-Karte gespeichert
// (siehe fatInitDefault() in main()) und in der Bibliothek wieder
// angezeigt. Die Leinwand ist eine eigene 8bpp-Bitmap-Hintergrundebene
// (BG3, Sub-Engine), die sich eine freie 16KB-Luecke am Ende von Bank C mit
// Konsolentext/Kachel-Hintergrund teilt (siehe Kommentar bei bgInitSub in
// main()). Darum ist die Leinwand mit 128x128px (statt bildschirmfuellenden
// 256x192px) bewusst kleiner.
#define DRAW_CANVAS_PX 128
#define DRAW_CANVAS_BYTES (DRAW_CANVAS_PX * DRAW_CANVAS_PX)
// Die Leinwand sitzt nicht mehr in der Bildschirm-Ecke, sondern per
// bgSetScale()/bgSetScroll() verkleinert und verschoben dargestellt --
// horizontal zentriert, oben mit kleinem Rand, unten mehr Platz fuer
// Buttons/Label. CANVAS_DISPLAY_PX ist die tatsaechliche Bildschirmgroesse
// (kleiner als die 128px-Pufferaufloesung); CANVAS_SCALE ist der 24.8-Fixed-
// Point-Skalierungsfaktor fuer bgSetScale() (>256 = verkleinert dargestellt).
#define CANVAS_DISPLAY_PX 96
#define CANVAS_SCALE ((DRAW_CANVAS_PX * 256) / CANVAS_DISPLAY_PX)
#define CANVAS_OFFSET_X ((256 - CANVAS_DISPLAY_PX) / 2)
#define CANVAS_OFFSET_Y 10
#define DRAW_DIR "/zeichnungen"
#define DRAW_MAX_COUNT 999
// Eigene Palettenindizes (Bank 7 ist fuer Konsolentext reserviert, siehe
// oben) -- Papier hell, Stift dunkel.
#define DRAW_PAPER_INDEX 220
#define DRAW_PEN_INDEX   221

// Wird von main() nach fatInitDefault() gesetzt. Schlaegt die FAT-
// Initialisierung fehl (kein/inkompatibler DLDI-Treiber), duerfen
// countDrawings()/saveDrawing()/loadDrawing() gar nicht erst fopen()/mkdir()
// aufrufen -- auf manchen Flashcard-Setups haengt sich ein Datei-Zugriff
// ueber einen kaputten Treiber komplett auf, statt sauber fehlzuschlagen.
static bool fatReady = false;

static char drawPathBuf[32];
static const char* drawPathFor(int index) {
	sprintf(drawPathBuf, DRAW_DIR "/z%03d.bin", index);
	return drawPathBuf;
}

// Zaehlt vorhandene Zeichnungen (fortlaufend nummeriert, keine Luecken).
static int countDrawings() {
	if (!fatReady) return 0;
	int n = 0;
	while (n < DRAW_MAX_COUNT) {
		FILE* f = fopen(drawPathFor(n + 1), "rb");
		if (!f) break;
		fclose(f);
		n++;
	}
	return n;
}

static bool saveDrawing(int index, const u8* canvas) {
	if (!fatReady) return false;
	mkdir(DRAW_DIR, 0777);
	FILE* f = fopen(drawPathFor(index), "wb");
	if (!f) return false;
	fwrite(canvas, 1, DRAW_CANVAS_BYTES, f);
	fclose(f);
	return true;
}

static bool loadDrawing(int index, u8* canvas) {
	if (!fatReady) return false;
	FILE* f = fopen(drawPathFor(index), "rb");
	if (!f) return false;
	fread(canvas, 1, DRAW_CANVAS_BYTES, f);
	fclose(f);
	return true;
}

static void clearCanvas(u8* canvas) {
	for (int i = 0; i < DRAW_CANVAS_BYTES; i++) canvas[i] = DRAW_PAPER_INDEX;
}

// Die eigentliche Zeichnung lebt in normalem RAM, nicht direkt im VRAM-
// Zeiger von bgGetGfxPtr(). Direkte, einzelne Schreibzugriffe von der
// Touch-Verarbeitung aus mitten im Haupt-Loop in dieses VRAM (das sich
// eine Bank mit Konsolentext/Kachel-Hintergrund teilt) erwiesen sich als
// unzuverlaessig -- auf melonDS UND auf echter DSi-Hardware kamen manche
// Schreibzugriffe nie im sichtbaren Bild an, nachgewiesen per direktem
// Auslesen nach dem Schreiben. Ein Referenzprojekt (OveNotesDS) loest das
// genauso: in RAM zeichnen, dann den kompletten Puffer per DMA (dmaCopy)
// auf einmal ins VRAM kopieren -- DMA-Transfers sind zuverlaessig, einzelne
// verstreute CPU-Schreibzugriffe mitten im Frame offenbar nicht.
static u8 canvasRAM[DRAW_CANVAS_BYTES];

static void syncCanvasToVRAM(u8* vramCanvas) {
	dmaCopy(canvasRAM, vramCanvas, DRAW_CANVAS_BYTES);
}

// Zeichnet einen runden Klecks statt eines 1px-Punkts -- jetzt unbedenklich,
// da wir in canvasRAM (normales RAM) schreiben, nicht mehr direkt ins VRAM.
// BRUSH_RADIUS_SQ ist der Radius zum Quadrat (Kreis-Test ohne sqrt): 1
// ergibt eine kleine Raute/Plus-Form (5 Pixel, ohne die Eckpixel des
// 3x3-Quadrats) -- duenner und runder als ein volles Quadrat.
#define BRUSH_RADIUS_SQ 1
static void plotBrush(u8* canvas, int x, int y) {
	for (int dy = -1; dy <= 1; dy++) {
		int py = y + dy;
		if (py < 0 || py >= DRAW_CANVAS_PX) continue;
		for (int dx = -1; dx <= 1; dx++) {
			if (dx * dx + dy * dy > BRUSH_RADIUS_SQ) continue;
			int px = x + dx;
			if (px < 0 || px >= DRAW_CANVAS_PX) continue;
			canvas[py * DRAW_CANVAS_PX + px] = DRAW_PEN_INDEX;
		}
	}
}

// Bresenham-Linie zwischen zwei Punkten, damit schnelle Wischbewegungen
// einen durchgehenden Strich statt einzelner Punkte ergeben.
static void plotLine(u8* canvas, int x0, int y0, int x1, int y1) {
	int dx = x1 > x0 ? x1 - x0 : x0 - x1;
	int sx = x0 < x1 ? 1 : -1;
	int dy = y1 > y0 ? y0 - y1 : y1 - y0;
	int sy = y0 < y1 ? 1 : -1;
	int err = dx + dy;
	for (;;) {
		plotBrush(canvas, x0, y0);
		if (x0 == x1 && y0 == y1) break;
		int e2 = 2 * err;
		if (e2 >= dy) { err += dy; x0 += sx; }
		if (e2 <= dx) { err += dx; y0 += sy; }
	}
}


// MODE_5_2D erlaubt BG0/BG1 als normale Text-Hintergruende UND BG2/BG3 als
// Bitmap gleichzeitig -- anders als vorher angenommen muss die Sub-Engine
// also gar nicht zwischen einem "Text"- und einem "Bitmap"-Modus umgeschaltet
// werden. Ein einziger Modus mit allen vier Layern aktiv reicht, Konsole
// (BG0), Kachel-Hintergrund (BG1) und Mal-Leinwand (BG3) leben dauerhaft
// nebeneinander.
#define SUB_MODE_ALL (MODE_5_2D | DISPLAY_BG0_ACTIVE | DISPLAY_BG1_ACTIVE | DISPLAY_BG3_ACTIVE | DISPLAY_SPR_ACTIVE | DISPLAY_SPR_1D_LAYOUT)

int main(void) {
	// Zeichnungen werden dauerhaft auf der SD-Karte gespeichert (siehe
	// saveDrawing()/loadDrawing()). Schlaegt fatInitDefault() fehl (z.B. kein
	// DLDI-Treiber), bleibt Speichern/Laden wirkungslos (fatReady-Guards
	// oben), statt fopen()/mkdir() auf einem kaputten Treiber zu riskieren.
	fatReady = fatInitDefault();

	// Oberer Bildschirm: nur ein 16bpp Bitmap-Hintergrund (BG3). Sowohl der
	// Start-Screen (bg_top) als auch die Kaffeedex-Infoseiten (kaffeedex_*)
	// sind komplett vorgerenderte Bilder in derselben Aufloesung -- zum
	// Wechseln wird einfach der komplette Bildspeicher ueberschrieben.
	videoSetMode(MODE_5_2D | DISPLAY_BG3_ACTIVE);
	vramSetBankA(VRAM_A_MAIN_BG);
	int bgTop = bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
	dmaCopy(bg_topBitmap, bgGetGfxPtr(bgTop), bg_topBitmapLen);

	consoleDemoInit();

	// Sub-Engine: einmalig in den kombinierten Modus schalten (siehe
	// SUB_MODE_ALL oben), bevor irgendein BG/OAM auf ihr eingerichtet wird.
	videoSetModeSub(SUB_MODE_ALL);
	vramSetBankD(VRAM_D_SUB_SPRITE);
	oamInit(&oamSub, SpriteMapping_1D_128, false);

	// Mal-Leinwand: 16KB-Bitmap, untergebracht in einer freien Luecke ganz am
	// Ende von Bank C (mapBase=7 -> Byte-Offset 112KB). Bank H/I waeren zwar
	// als zusaetzliche Sub-BG-Baenke verfuegbar, liegen aber laut libnds im
	// selben Adress-"Slot 0" wie Bank C und ueberlappen sich damit -- die
	// Konsole/der Kachel-Hintergrund (Bank C, Tiles ab Byte 0 bzw. 64KB,
	// Map bei 32KB bzw. mapBase 30) belegen davon nur bis rund 107KB, die
	// letzten 16KB (112KB-128KB) sind frei.
	int bgDraw = bgInitSub(3, BgType_Bmp8, BgSize_B8_128x128, 7, 0);
	bgSetPriority(bgDraw, 0);
	bgSetCenter(bgDraw, 0, 0);
	bgSetScale(bgDraw, CANVAS_SCALE, CANVAS_SCALE);
	bgSetScroll(bgDraw, -CANVAS_OFFSET_X, -CANVAS_OFFSET_Y);
	// BG3 ist in SUB_MODE_ALL dauerhaft aktiv (siehe Kommentar oben) -- sie
	// soll aber nur auf SCREEN_DRAWING/SCREEN_LIBRARY sichtbar sein, darum
	// hier erstmal ausblenden und bei jedem Bildschirmwechsel explizit ein-
	// bzw. ausblenden (bgShow()/bgHide()), statt wie vorher den ganzen Modus
	// umzuschalten.
	bgHide(bgDraw);
	u8* drawCanvas = (u8*)bgGetGfxPtr(bgDraw);
	clearCanvas(canvasRAM);
	syncCanvasToVRAM(drawCanvas);

	// Hintergrundbild fuer den Touchscreen (BG1, hinter Konsolentext BG0 und
	// den Sprites). tileBase/mapBase sind bewusst so gewaehlt, dass sie
	// nicht mit dem Bereich kollidieren, den consoleDemoInit fuer die
	// Text-Konsole in VRAM_C belegt (Tiles ab Byte 0, Map bei mapBase 30).
	int bgBottom = bgInitSub(1, BgType_Text8bpp, BgSize_T_256x256, 16, 4);
	dmaCopy(bg_bottomTiles, bgGetGfxPtr(bgBottom), bg_bottomTilesLen);
	dmaCopy(bg_bottomMap, bgGetMapPtr(bgBottom), bg_bottomMapLen);
	dmaCopy(bg_bottomPal, BG_PALETTE_SUB, bg_bottomPalLen);
	bgSetPriority(bgBottom, 3);

	// bg_bottom nutzt nur die Indizes 0-95 (siehe grit-Ausgabe) -- die
	// dmaCopy oben ueberschreibt aber die GESAMTE 256-Farben-Palette, darum
	// muessen Papier-/Stift-Farbe der Mal-Leinwand erst danach gesetzt
	// werden, sonst werden sie sofort wieder auf Schwarz zurueckgesetzt.
	BG_PALETTE_SUB[DRAW_PAPER_INDEX] = RGB8(248, 248, 248);
	BG_PALETTE_SUB[DRAW_PEN_INDEX] = RGB8(60, 30, 20);

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

	// Milch/Zucker-Zusatzfrage (nur fuer Espresso/Filterkaffee, siehe
	// coffeeNeedsMilkSugarQuestion()). Verwendet denselben Kartenrahmen wie
	// die Kaffeeart-Auswahl (iconFrameGfx, Bank 10). Eigene Icon-Paletten 9
	// und 8, da 15 (Buttons), 14 (Auswahl-Klammer) und 11-13 (Kaffee-Icons)
	// schon belegt sind.
	milkCard.frame = &iconFrameGfx;
	sugarCard.frame = &iconFrameGfx;
	milkCard.iconGfx = loadIconGfx(milkTiles, milkTilesLen, milkPal, milkCard.iconPaletteBank);
	sugarCard.iconGfx = loadIconGfx(sugarTiles, sugarTilesLen, sugarPal, sugarCard.iconPaletteBank);
	placeIconButton(milkCard, oamId);
	oamId += 7;
	hideIconButton(milkCard);
	placeIconButton(sugarCard, oamId);
	oamId += 7;
	hideIconButton(sugarCard);

	oamId = placeSpriteButton(msConfirmButton, buttonGfx, oamId);
	hideSpriteButton(msConfirmButton, buttonGfx);

	oamId = placeSpriteButton(drawClearButton, buttonGfx, oamId);
	hideSpriteButton(drawClearButton, buttonGfx);
	oamId = placeSpriteButton(drawDoneButton, buttonGfx, oamId);
	hideSpriteButton(drawDoneButton, buttonGfx);

	Screen screen = SCREEN_START;
	int pressedIndex = -1;
	int selectedIndex = 0;
	CoffeeFocus coffeeFocus = FOCUS_CARD;
	int coffeeCardIndex = 0;
	int lastCoffeeInfoIndex = -1; // erzwingt einmaliges Zeichnen beim ersten Betreten des Screens

	MilkSugarFocus msFocus = FOCUS_MILK;
	bool wantsMilk = false;
	bool wantsSugar = false;
	bool msHasMilk = true; // false bei Cappuccino, siehe milkSugarHasMilk()

	DrawFocus drawFocus = DRAW_FOCUS_DONE;
	int libraryIndex = 0;   // 1-basiert, wie die Dateinamen
	int libraryCount = 0;
	int lastDrawX = -1, lastDrawY = -1; // fuer Linien zwischen Touch-Frames

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
				lastCoffeeInfoIndex = -1; // erzwingt Neuzeichnen des oberen Bildschirms
				pressedIndex = -1;
			} else if (confirmed && selectedIndex == 1) {
				// "Bibliothek" -> gespeicherte Zeichnungen durchblaettern.
				for (size_t i = 0; i < START_BUTTON_COUNT; i++) hideSpriteButton(startButtons[i], buttonGfx);
				oamSet(&oamSub, 0, 0, 0, 0, 0, SpriteSize_64x32, SpriteColorFormat_256Color,
					cupGfx, -1, false, true, false, false, false);

				dmaCopy(bg_topBitmap, bgGetGfxPtr(bgTop), bg_topBitmapLen);
				bgShow(bgDraw);
				consoleClear();
				libraryCount = countDrawings();
				libraryIndex = libraryCount > 0 ? 1 : 0;
				if (libraryIndex > 0) {
					loadDrawing(libraryIndex, canvasRAM);
					syncCanvasToVRAM(drawCanvas);
					iprintf("\x1b[15;9HZeichnung %d/%d", libraryIndex, libraryCount);
				} else {
					clearCanvas(canvasRAM);
					syncCanvasToVRAM(drawCanvas);
					iprintf("\x1b[15;8HNoch keine Zeichnung.");
				}

				screen = SCREEN_LIBRARY;
				pressedIndex = -1;
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

			if (coffeeCardIndex != lastCoffeeInfoIndex) {
				dmaCopy(kaffeedexBitmaps[coffeeCardIndex].data, bgGetGfxPtr(bgTop), kaffeedexBitmaps[coffeeCardIndex].len);
				lastCoffeeInfoIndex = coffeeCardIndex;
			}

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
			int confirmedIndex = pressedIndex; // vor dem Reset unten sichern

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

			if (confirmed && confirmedIndex == (int)COFFEE_BUTTON_COUNT) {
				// Zusatzfrage nach Milch/Zucker (Cappuccino ohne Milch-Karte,
				// siehe milkSugarHasMilk()).
				for (size_t i = 0; i < COFFEE_BUTTON_COUNT; i++) hideIconButton(coffeeButtons[i]);
				hideSpriteButton(confirmButton, buttonGfx);

				msHasMilk = milkSugarHasMilk(coffeeCardIndex);
				wantsMilk = false;
				wantsSugar = false;
				msFocus = msHasMilk ? FOCUS_MILK : FOCUS_SUGAR;
				pressedIndex = -1;
				sugarCard.x = msHasMilk ? SUGAR_CARD_X_WITH_MILK : SUGAR_CARD_X_CENTERED;

				if (msHasMilk) placeIconButton(milkCard, milkCard.oamId);
				drawMilkSugarLabels(msHasMilk);
				placeIconButton(sugarCard, sugarCard.oamId);
				placeSpriteButton(msConfirmButton, buttonGfx, msConfirmButton.oamBaseId);
				showBestellungBitmap(bgTop, coffeeCardIndex, wantsMilk, wantsSugar);

				screen = SCREEN_MILK_SUGAR;
			}

			if (keysDown() & KEY_B) {
				// Zurueck zum Start-Bildschirm.
				for (size_t i = 0; i < COFFEE_BUTTON_COUNT; i++) hideIconButton(coffeeButtons[i]);
				hideSpriteButton(confirmButton, buttonGfx);

				dmaCopy(bg_topBitmap, bgGetGfxPtr(bgTop), bg_topBitmapLen);

				screen = SCREEN_START;
				selectedIndex = 0;
				pressedIndex = -1;

				for (size_t i = 0; i < START_BUTTON_COUNT; i++) placeSpriteButton(startButtons[i], buttonGfx, startButtons[i].oamBaseId);
			}
		} else if (screen == SCREEN_MILK_SUGAR) {
			// Milch/Zucker liegen nebeneinander -> LINKS/RECHTS wechselt
			// zwischen den beiden Karten, HOCH/RUNTER springt zur/von der
			// Bestaetigen-Zeile (wie bei der Kaffeeart-Auswahl).
			if (keysDown() & KEY_LEFT && msFocus != FOCUS_MS_CONFIRM && msHasMilk) {
				msFocus = FOCUS_MILK;
			}
			if (keysDown() & KEY_RIGHT && msFocus != FOCUS_MS_CONFIRM && msHasMilk) {
				msFocus = FOCUS_SUGAR;
			}
			if (keysDown() & KEY_DOWN) msFocus = FOCUS_MS_CONFIRM;
			if (keysDown() & KEY_UP) msFocus = msHasMilk ? FOCUS_MILK : FOCUS_SUGAR;

			if (keysDown() & KEY_A) {
				pressedIndex = (int)msFocus;
				if (msFocus == FOCUS_MILK && msHasMilk) {
					wantsMilk = !wantsMilk;
					showBestellungBitmap(bgTop, coffeeCardIndex, wantsMilk, wantsSugar);
				} else if (msFocus == FOCUS_SUGAR) {
					wantsSugar = !wantsSugar;
					showBestellungBitmap(bgTop, coffeeCardIndex, wantsMilk, wantsSugar);
				}
			}

			if (keysDown() & KEY_TOUCH) {
				touchPosition touch;
				touchRead(&touch);

				if (msHasMilk && isTouchInIconButton(milkCard, touch)) {
					msFocus = FOCUS_MILK;
					pressedIndex = FOCUS_MILK;
					wantsMilk = !wantsMilk;
					showBestellungBitmap(bgTop, coffeeCardIndex, wantsMilk, wantsSugar);
				} else if (isTouchInIconButton(sugarCard, touch)) {
					msFocus = FOCUS_SUGAR;
					pressedIndex = FOCUS_SUGAR;
					wantsSugar = !wantsSugar;
					showBestellungBitmap(bgTop, coffeeCardIndex, wantsMilk, wantsSugar);
				} else if (isTouchInSpriteButton(msConfirmButton, touch)) {
					msFocus = FOCUS_MS_CONFIRM;
					pressedIndex = FOCUS_MS_CONFIRM;
				}
			}

			bool confirmed = (keysUp() & (KEY_A | KEY_TOUCH)) && pressedIndex == FOCUS_MS_CONFIRM;

			if (keysUp() & (KEY_A | KEY_TOUCH)) {
				pressedIndex = -1;
			}

			if (msHasMilk) updateIconButtonPress(milkCard, pressedIndex == FOCUS_MILK);
			updateIconButtonPress(sugarCard, pressedIndex == FOCUS_SUGAR);
			updateSpriteButtonPress(msConfirmButton, buttonGfx, pressedIndex == FOCUS_MS_CONFIRM);

			if (msFocus == FOCUS_MILK) {
				updateSelectionBracketAt(milkCard.x, milkCard.y, ICON_BUTTON_WIDTH_PX, ICON_BUTTON_HEIGHT_PX, bracketGfx, bracketOamId);
			} else if (msFocus == FOCUS_SUGAR) {
				updateSelectionBracketAt(sugarCard.x, sugarCard.y, ICON_BUTTON_WIDTH_PX, ICON_BUTTON_HEIGHT_PX, bracketGfx, bracketOamId);
			} else {
				updateSelectionBracketAt(msConfirmButton.x, msConfirmButton.y,
					spriteButtonWidth(msConfirmButton), SPRITE_BUTTON_HEIGHT_PX, bracketGfx, bracketOamId);
			}

			if (confirmed) {
				// Bestellung abgeschlossen -> Mal-Screen fuer die Bibliothek.
				hideIconButton(milkCard);
				hideIconButton(sugarCard);
				hideSpriteButton(msConfirmButton, buttonGfx);
				clearMilkSugarLabels();

				dmaCopy(bg_topBitmap, bgGetGfxPtr(bgTop), bg_topBitmapLen);
				bgShow(bgDraw);
				consoleClear();
				clearCanvas(canvasRAM);
				syncCanvasToVRAM(drawCanvas);
				iprintf("\x1b[15;4HNachricht an die Barista:");
				drawFocus = DRAW_FOCUS_DONE;
				placeSpriteButton(drawClearButton, buttonGfx, drawClearButton.oamBaseId);
				placeSpriteButton(drawDoneButton, buttonGfx, drawDoneButton.oamBaseId);

				screen = SCREEN_DRAWING;
				pressedIndex = -1;
			}

			if (keysDown() & KEY_B) {
				// Zurueck zur Kaffeeart-Auswahl.
				hideIconButton(milkCard);
				hideIconButton(sugarCard);
				hideSpriteButton(msConfirmButton, buttonGfx);
				clearMilkSugarLabels();

				dmaCopy(kaffeedexBitmaps[coffeeCardIndex].data, bgGetGfxPtr(bgTop), kaffeedexBitmaps[coffeeCardIndex].len);

				screen = SCREEN_COFFEE_TYPE;
				coffeeFocus = FOCUS_CONFIRM;
				pressedIndex = -1;

				for (size_t i = 0; i < COFFEE_BUTTON_COUNT; i++) placeIconButton(coffeeButtons[i], coffeeButtons[i].oamId);
				placeSpriteButton(confirmButton, buttonGfx, confirmButton.oamBaseId);
			}
		} else if (screen == SCREEN_DRAWING) {
			// Touch innerhalb der Leinwand -> malen (auch waehrend gehalten,
			// nicht nur beim ersten Antippen). Wird in canvasRAM (normales
			// RAM) geschrieben, nicht direkt ins VRAM -- siehe Kommentar bei
			// canvasRAM oben. Touch-Koordinaten sind Bildschirm-Pixel, die
			// Leinwand liegt aber per bgSetScale()/bgSetScroll() verkleinert
			// bei CANVAS_OFFSET_X/Y -- erst den Versatz abziehen, dann mit
			// CANVAS_SCALE zurueck auf Puffer-Pixel hochrechnen (24.8-Fixed-
			// Point, daher >>8). plotBrush()/plotLine() pruefen Grenzen
			// selbst pro Pixel, ein Touch ausserhalb der Leinwand wird also
			// einfach ignoriert statt zu wrappen.
			if (keysDown() & KEY_TOUCH) {
				// Neuer Strich beginnt -- nicht mit dem Ende des vorherigen
				// verbinden.
				lastDrawX = -1;
				lastDrawY = -1;
			}
			if (keysHeld() & KEY_TOUCH) {
				touchPosition touch;
				touchRead(&touch);
				int tx = (((int)touch.px - CANVAS_OFFSET_X) * CANVAS_SCALE) >> 8;
				int ty = (((int)touch.py - CANVAS_OFFSET_Y) * CANVAS_SCALE) >> 8;
				if (lastDrawX < 0) {
					plotBrush(canvasRAM, tx, ty);
				} else {
					plotLine(canvasRAM, lastDrawX, lastDrawY, tx, ty);
				}
				lastDrawX = tx;
				lastDrawY = ty;
				syncCanvasToVRAM(drawCanvas);
			}

			if (keysDown() & KEY_UP) drawFocus = DRAW_FOCUS_CLEAR;
			if (keysDown() & KEY_DOWN) drawFocus = DRAW_FOCUS_DONE;

			if (keysDown() & KEY_A) {
				pressedIndex = (int)drawFocus;
			}

			if (keysDown() & KEY_TOUCH) {
				touchPosition touch;
				touchRead(&touch);
				if (isTouchInSpriteButton(drawClearButton, touch)) {
					drawFocus = DRAW_FOCUS_CLEAR;
					pressedIndex = DRAW_FOCUS_CLEAR;
				} else if (isTouchInSpriteButton(drawDoneButton, touch)) {
					drawFocus = DRAW_FOCUS_DONE;
					pressedIndex = DRAW_FOCUS_DONE;
				}
			}

			bool confirmed = (keysUp() & (KEY_A | KEY_TOUCH)) && pressedIndex >= 0;
			int confirmedFocus = pressedIndex; // vor dem Reset unten sichern

			if (keysUp() & (KEY_A | KEY_TOUCH)) {
				pressedIndex = -1;
			}

			updateSpriteButtonPress(drawClearButton, buttonGfx, pressedIndex == DRAW_FOCUS_CLEAR);
			updateSpriteButtonPress(drawDoneButton, buttonGfx, pressedIndex == DRAW_FOCUS_DONE);

			if (drawFocus == DRAW_FOCUS_CLEAR) {
				updateSelectionBracketAt(drawClearButton.x, drawClearButton.y,
					spriteButtonWidth(drawClearButton), SPRITE_BUTTON_HEIGHT_PX, bracketGfx, bracketOamId);
			} else {
				updateSelectionBracketAt(drawDoneButton.x, drawDoneButton.y,
					spriteButtonWidth(drawDoneButton), SPRITE_BUTTON_HEIGHT_PX, bracketGfx, bracketOamId);
			}

			if (confirmed && confirmedFocus == DRAW_FOCUS_CLEAR) {
				clearCanvas(canvasRAM);
				syncCanvasToVRAM(drawCanvas);
			} else if (confirmed && confirmedFocus == DRAW_FOCUS_DONE) {
				// Auf der SD-Karte speichern und zurueck zum Start.
				saveDrawing(countDrawings() + 1, canvasRAM);

				hideSpriteButton(drawClearButton, buttonGfx);
				hideSpriteButton(drawDoneButton, buttonGfx);
				bgHide(bgDraw);
				consoleClear();

				dmaCopy(bg_topBitmap, bgGetGfxPtr(bgTop), bg_topBitmapLen);

				screen = SCREEN_START;
				selectedIndex = 0;
				pressedIndex = -1;
				for (size_t i = 0; i < START_BUTTON_COUNT; i++) placeSpriteButton(startButtons[i], buttonGfx, startButtons[i].oamBaseId);
			}
		} else if (screen == SCREEN_LIBRARY) {
			if (libraryCount > 0) {
				if (keysDown() & KEY_LEFT) {
					libraryIndex = (libraryIndex - 2 + libraryCount) % libraryCount + 1;
					loadDrawing(libraryIndex, canvasRAM);
					syncCanvasToVRAM(drawCanvas);
					iprintf("\x1b[15;9HZeichnung %d/%d ", libraryIndex, libraryCount);
				}
				if (keysDown() & KEY_RIGHT) {
					libraryIndex = libraryIndex % libraryCount + 1;
					loadDrawing(libraryIndex, canvasRAM);
					syncCanvasToVRAM(drawCanvas);
					iprintf("\x1b[15;9HZeichnung %d/%d ", libraryIndex, libraryCount);
				}
			}

			if (keysDown() & KEY_B) {
				bgHide(bgDraw);
				consoleClear();

				dmaCopy(bg_topBitmap, bgGetGfxPtr(bgTop), bg_topBitmapLen);

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
