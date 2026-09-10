// Eigener ARM7-Kern: das Standard-Setup (Keypad/RTC/Power/Touch), erweitert
// um einen zusaetzlichen Thread, der den I2C-Bus fuer die Kamerasensoren
// bedient (siehe i2c_handler.c). Der Standard-ARM7 von devkitPro (der sonst
// automatisch verlinkt wird) hat keinen Zugriff auf die Kamera-I2C-Adressen
// -- dafuer brauchen wir diesen eigenen ARM7-Build.

#include "i2c_handler.h"

#include <nds.h>

int main(void) {
	envReadNvramSettings();
	keypadStartExtServer();

	lcdSetIrqMask(DISPSTAT_IE_ALL, DISPSTAT_IE_VBLANK);
	irqEnable(IRQ_VBLANK);

	rtcInit();
	rtcSyncTime();

	pmInit();
	blkInit();

	touchInit();
	touchStartServer(80, MAIN_THREAD_PRIO);

	i2cCameraServerStart();

	while (pmMainLoop()) {
		threadWaitForVBlank();
	}

	return 0;
}
