#pragma once

#include <nds.h>

// Inhalt einer Kaffeeart-Infoseite (oberer Bildschirm, Pokedex-artiges Layout).
struct CoffeeInfo {
	const char* name;
	const char* category;
	const char* badge1;
	const char* badge2;
	int caffeineMg;
	int volumeMl;
	const char* descLines[4]; // bis zu 4 Zeilen, ungenutzte Zeilen = nullptr
};

// Initialisiert Text-Konsole (BG0) und Sprite-Engine des Hauptbildschirms
// (oberer Screen) fuer die Infoseite. Muss einmal aufgerufen werden, bevor
// updateCoffeeInfo() benutzt wird.
void initCoffeeInfoScreen();

// Laedt das Icon-Sprite einer Kaffeeart fuer den Hauptbildschirm (separates
// Sprite-Speicher/Palette vom Icon auf dem Touchscreen, da Haupt- und
// Sub-Engine getrennte OAM/VRAM-Bereiche haben).
u16* loadCoffeeInfoIconGfx(const void* tiles, unsigned int tilesLen, const void* pal, int paletteBank);

// Zeichnet die Infoseite fuer die angegebene Kaffeeart neu (Name, Kategorie,
// Badges, Stats, Beschreibung) und positioniert das zugehoerige Icon-Sprite.
void updateCoffeeInfo(const CoffeeInfo& info, u16* iconGfx, int iconPaletteBank);
