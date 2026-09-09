#pragma once

#include <nds.h>

#define ICON_BUTTON_WIDTH_PX 80
#define ICON_BUTTON_HEIGHT_PX 96
#define ICON_BUTTON_ROW_HEIGHT_PX 32     // Hoehe jeder der 3 Rahmen-Reihen
#define ICON_BUTTON_MAIN_COL_WIDTH_PX 64 // Breite der linken Hauptspalte je Reihe

// Ein breiter, hochkanter Button, zusammengesetzt aus 6 Rahmen-Sprites (3
// Reihen a 32px Hoehe, je Reihe eine 64px-Hauptspalte + eine 16px-Spalte,
// da ein einzelnes Sprite max. 64px pro Seite gross sein darf) und einem
// Icon-Sprite obendrauf. Die Rahmen-Sprites werden einmal geladen und hier
// nur referenziert, damit sie sich fuer jeden Button wiederverwenden lassen.
struct IconButtonFrameGfx {
	u16* topL;
	u16* topR;
	u16* midL;
	u16* midR;
	u16* bottomL;
	u16* bottomR;
	int paletteBank;
};

struct IconButton {
	const IconButtonFrameGfx* frame;

	u16* iconGfx;
	int iconPaletteBank;

	int x;
	int y;

	int oamId; // 6 Rahmen-Slots (oamId..oamId+5), Icon = oamId+6 (von placeIconButton gesetzt)
};

// Laedt die (fuer alle Buttons gemeinsamen) Rahmen-Teile in die angegebene Bank.
IconButtonFrameGfx loadIconFrameGfx(int paletteBank);

// Laedt Tile-Daten und Palette eines einzelnen Icons in eine eigene
// 16-Farben-Bank und gibt den allokierten Grafikspeicher zurueck.
u16* loadIconGfx(const void* tiles, unsigned int tilesLen, const void* pal, int paletteBank);

// Platziert Rahmen (oamId..oamId+5) und Icon (oamId+6) einmalig.
void placeIconButton(IconButton& button, int oamId);

// Verschiebt Rahmen und Icon gemeinsam um ein paar Pixel nach unten, solange
// pressed true ist, fuer denselben "eingedrueckt"-Effekt wie bei
// SpriteButton. Muss jeden Frame aufgerufen werden.
void updateIconButtonPress(const IconButton& button, bool pressed);

// Blendet Rahmen und Icon aus (fuer Bildschirmwechsel), OAM-Slots bleiben belegt.
void hideIconButton(const IconButton& button);

// Prueft, ob eine Touch-Position (in Pixeln) innerhalb des Buttons liegt.
bool isTouchInIconButton(const IconButton& button, const touchPosition& touch);
