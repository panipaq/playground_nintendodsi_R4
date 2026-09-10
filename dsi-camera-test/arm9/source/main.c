// Minimaler Kamera-Test: zeigt das Live-Bild einer Kamera als Bitmap auf dem
// oberen Bildschirm. A wechselt zwischen Aussen-/Innenkamera, START beendet
// (und schaltet die Kamera-LED wieder aus).
//
// Bewusst ohne FAT/PNG-Speichern -- das ist erstmal nur der Beweis, dass
// Kamera-Init + Live-Vorschau auf echter DSi-Hardware ueberhaupt geht.

#include "camera.h"

#include <calico/nds/pxi.h>
#include <calico/nds/system.h>
#include <nds.h>
#include <stdio.h>

int main(void) {
	consoleDemoInit();
	vramSetBankA(VRAM_A_MAIN_BG);
	videoSetMode(MODE_5_2D | DISPLAY_BG3_ACTIVE);
	int bgTop = bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);

	iprintf("DSi Kamera-Test\n\n");

	iprintf("TWL-Modus (echtes DSi): %s\n", systemIsTwlMode() ? "JA" : "NEIN (DS-Modus!)");
	swiWaitForVBlank();

	iprintf("Warte auf ARM7-Kamera-Thread...\n");
	swiWaitForVBlank();
	pxiWaitRemote(PxiChannel_Camera); // warten bis ARM7-Thread bereit ist
	iprintf("ARM7-Thread bereit.\n");

	iprintf("Sende CAM_INIT (kann bis zu ~100s\ndauern, falls Sensor nicht\nantwortet)...\n");
	swiWaitForVBlank();
	bool ok = cameraInit();
	iprintf("CAM_INIT Antwort: 0x%04lX\n", cameraLastInitStatus());
	iprintf("CAM_MCNT danach: 0x%04X\n", REG_CAM_MCNT);
	iprintf("CAM_CNT danach:  0x%04X\n", REG_CAM_CNT);

	if (!ok) {
		iprintf("Kamera-Init fehlgeschlagen!\n");
		iprintf("(0x1xx/0x2xx = Timeout bei\nKamera 0/1, Schritt xx)\n");
		while (pmMainLoop()) swiWaitForVBlank();
		return 0;
	}

	Camera camera = CAM_OUTER;
	bool activated = cameraActivate(camera);
	iprintf("Kamera aktiviert: %s\n", activated ? "ja" : "NEIN");

	iprintf("\nA = Kamera wechseln\nSTART = beenden\n");

	while (pmMainLoop()) {
		swiWaitForVBlank();

		if (!cameraTransferActive()) {
			cameraTransferStart(bgGetGfxPtr(bgTop), CAPTURE_MODE_PREVIEW);
		}

		scanKeys();
		u16 pressed = keysDown();

		if (pressed & KEY_A) {
			while (cameraTransferActive()) swiWaitForVBlank();
			cameraTransferStop();

			camera = (camera == CAM_INNER) ? CAM_OUTER : CAM_INNER;
			cameraActivate(camera);
		} else if (pressed & KEY_START) {
			cameraDeactivate(camera);
			break;
		}
	}

	return 0;
}
