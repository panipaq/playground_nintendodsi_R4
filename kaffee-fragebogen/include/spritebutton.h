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

// Blendet den Button aus (fuer Bildschirmwechsel), OAM-Slots bleiben belegt.
void hideSpriteButton(const SpriteButton& button, const SpriteButtonGfx& gfx);

// Prueft, ob eine Touch-Position (in Pixeln) innerhalb der Button-Flaeche liegt.
bool isTouchInSpriteButton(const SpriteButton& button, const touchPosition& touch);

// Sichtbarer Hoehen-Bereich des Buttons in Pixeln (fuer bracket.h's
// updateSelectionBracketAt, siehe main.cpp).
#define SPRITE_BUTTON_HEIGHT_PX 27
