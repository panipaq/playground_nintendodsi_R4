#pragma once

#include <nds.h>

// Die vier Eckstuecke einer Klammer-Auswahlanzeige (aus UiCozyFree.png).
struct BracketGfx {
	u16* topLeft;
	u16* topRight;
	u16* bottomLeft;
	u16* bottomRight;
};

// Laedt Tile-Daten und Palette der Klammer-Grafik einmalig in den
// Sprite-Speicher des Touchscreens (sub display).
BracketGfx loadBracketGfx();

// Platziert die vier Eckstuecke der Klammer um ein Rechteck (x,y = obere
// linke Ecke, width/height = Ausdehnung). oamBaseId braucht 4
// zusammenhaengende, exklusiv reservierte OAM-IDs. Muss jeden Frame
// aufgerufen werden, um die Klammer an ihrer Position zu halten.
void updateSelectionBracketAt(int x, int y, int width, int height, const BracketGfx& gfx, int oamBaseId);
