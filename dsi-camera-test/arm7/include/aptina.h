#pragma once

#include "pxi_vars.h"

#include <nds.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialisiert einen Kamerasensor (Registersequenz laut GBATEK:
// https://problemkaputt.de/gbatek.htm#dsiaptinacamerainitialization).
// Rueckgabe 0 = Erfolg, sonst Nummer des Schritts, an dem eine I2C-Antwort
// vom Sensor ausblieb (Timeout nach ca. 10s statt endlosem Haengen).
int aptinaInit(u8 device);

// Weckt den Sensor auf und schaltet den Pixel-Datenausgang frei.
// Rueckgabe false = Timeout beim Warten auf WakeupDone.
bool aptinaActivate(u8 device);

// Versetzt den Sensor in Standby und schaltet den Datenausgang ab.
// Rueckgabe false = Timeout beim Warten auf StandbyDone.
bool aptinaDeactivate(u8 device);

// Wechselt zwischen Vorschau- (256x192) und Vollaufloesungs-Modus (640x480).
void aptinaSetMode(CaptureMode mode);

#ifdef __cplusplus
}
#endif
