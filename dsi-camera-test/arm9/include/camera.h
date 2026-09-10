#pragma once

#include "pxi_vars.h"

#include <nds/ndstypes.h>

#ifdef __cplusplus
extern "C" {
#endif

// Kamera-Steuerregister (nicht in libnds/calico vordefiniert, siehe
// https://problemkaputt.de/gbatek-dsi-cameras.htm).
#define REG_CAM_MCNT (*(vu16*)0x04004200)
#define REG_CAM_CNT  (*(vu16*)0x04004202)
#define REG_CAM_DAT  (*(vu16*)0x04004204)

// NDMA-Kanal 1, ueber den die Kameradaten direkt in den Grafikspeicher
// kopiert werden (siehe cameraTransferStart()).
#define REG_NDMA1SAD  (*(vu32*)0x04004120)
#define REG_NDMA1DAD  (*(vu32*)0x04004124)
#define REG_NDMA1TCNT (*(vu32*)0x04004128)
#define REG_NDMA1WCNT (*(vu32*)0x0400412C)
#define REG_NDMA1BCNT (*(vu32*)0x04004130)
#define REG_NDMA1CNT  (*(vu32*)0x04004138)

typedef enum {
	CAM_NONE,
	CAM_INNER, // Frontkamera (Selfie)
	CAM_OUTER, // Kamera auf der Rueckseite
} Camera;

// Initialisiert die Kamera-Hardware und beide Sensoren (ueber ARM7/I2C).
// Muss einmal aufgerufen werden, bevor cameraActivate() benutzt wird.
bool cameraInit(void);

// Rohwert der letzten CAM_INIT-Antwort vom ARM7, fuer Diagnose:
// 0x2280       = Erfolg
// 0x1SS / 0x2SS = Timeout bei Kamera 0/1, Schritt SS (siehe aptina.c)
u32 cameraLastInitStatus(void);

// Aktiviert eine Kamera; die jeweils andere wird automatisch deaktiviert.
bool cameraActivate(Camera cam);

// Deaktiviert die angegebene Kamera (schaltet auch die Status-LED aus).
bool cameraDeactivate(Camera cam);

// Startet die Uebertragung eines Bilds per DMA in den angegebenen Speicher.
// Im Preview-Modus (256x192) liefert die Hardware direkt fertiges RGB555,
// im Capture-Modus (640x480) rohes YUV422 (muss man selbst konvertieren).
void cameraTransferStart(u16* dst, CaptureMode mode);

// Bricht eine laufende Uebertragung ab.
void cameraTransferStop(void);

// Prueft, ob gerade eine Uebertragung laeuft.
bool cameraTransferActive(void);

#ifdef __cplusplus
}
#endif
