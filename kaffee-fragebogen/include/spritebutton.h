#pragma once

#include <nds.h>

// Ein Sprite-Button besteht aus drei Sprites nebeneinander: linke Kappe,
// N mal ein tileable Mittelteil, rechte Kappe. So laesst sich jede
// Button-Breite aus denselben drei Grafikteilen zusammensetzen.
struct SpriteButtonGfx {
	u16* left;
	u16* mid;
	u16* right;
};

// Laedt Tile-Daten und Palette der Button-Grafik einmalig in den
// Sprite-Speicher des Touchscreens (sub display).
SpriteButtonGfx loadSpriteButtonGfx();

struct SpriteButton {
	const char* label;
	int x;           // Pixel-X der linken Kante
	int y;           // Pixel-Y der oberen Kante
	int middleTiles; // Anzahl der 16px breiten Mittelteile zwischen den Kappen

	int oamBaseId;   // wird von placeSpriteButton gesetzt, fuer updateSpriteButtonPress
};

// Gesamtbreite des Buttons in Pixeln (2 Kappen + Mittelteile).
int spriteButtonWidth(const SpriteButton& button);

// Platziert die OAM-Eintraege des Buttons ab oamId (merkt sich oamBaseId im
// Button) und zeichnet das Label einmalig zentriert per Konsolentext.
// Gibt den naechsten freien oamId zurueck.
int placeSpriteButton(SpriteButton& button, const SpriteButtonGfx& gfx, int oamId);

// Verschiebt die Button-Grafik (nicht das Label) um ein paar Pixel nach
// unten, solange pressed true ist, fuer einen "eingedrueckt"-Effekt.
// Muss jeden Frame aufgerufen werden, damit die Aenderung sichtbar bleibt.
void updateSpriteButtonPress(const SpriteButton& button, const SpriteButtonGfx& gfx, bool pressed);

// Prueft, ob eine Touch-Position (in Pixeln) innerhalb der Button-Flaeche liegt.
bool isTouchInSpriteButton(const SpriteButton& button, const touchPosition& touch);

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

// Platziert die vier Eckstuecke der Klammer um den angegebenen Button.
// oamBaseId braucht 4 zusammenhaengende, exklusiv reservierte OAM-IDs.
// Muss jeden Frame aufgerufen werden, um die Klammer auf dem aktuell
// ausgewaehlten Button zu halten.
void updateSelectionBracket(const SpriteButton& button, const BracketGfx& gfx, int oamBaseId);
