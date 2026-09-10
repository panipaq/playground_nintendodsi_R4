// Empfaengt Kamera-Kommandos vom ARM9 ueber den reservierten PXI-Kanal
// PxiChannel_Camera und fuehrt sie ueber den I2C-Bus aus (aptina.c). Laeuft
// als eigener Thread, damit der Haupt-ARM7-Loop (Touch/Sound/etc.) nicht
// blockiert wird, waehrend auf I2C-Antworten gewartet wird.

#include "i2c_handler.h"

#include "aptina.h"
#include "aptina_i2c.h"
#include "pxi_vars.h"

#include <nds.h>
#include <nds/arm7/i2c.h>

static Thread s_i2cPxiThread;
alignas(8) static u8 s_i2cPxiThreadStack[1024];

static int i2cPxiThreadMain(void* arg) {
	(void)arg;

	Mailbox mb;
	u32 mb_slots[4];
	mailboxPrepare(&mb, mb_slots, sizeof(mb_slots) / sizeof(mb_slots[0]));
	pxiSetMailbox(PxiChannel_Camera, &mb);

	for (;;) {
		u32 msg = mailboxRecv(&mb);
		u32 retval = 0;

		switch (msg) {
			case CAM_INIT: {
				// Bei Timeout: oberes Nibble zeigt an, welche Kamera betroffen
				// war (1=CAM0, 2=CAM1), unteres Nibble die Schrittnummer aus
				// aptinaInit() -- so sehen wir auf dem ARM9 genau, wo es haengt,
				// statt nur "hängt bei Initialisiere".
				int step0 = aptinaInit(I2C_CAM0);
				if (step0 != 0) {
					retval = 0x1000 | step0;
					break;
				}
				int step1 = aptinaInit(I2C_CAM1);
				if (step1 != 0) {
					retval = 0x2000 | step1;
					break;
				}
				retval = aptReadRegister(I2C_CAM0, 0);
				break;
			}
			case CAM0_ACTIVATE:
				retval = aptinaActivate(I2C_CAM0) ? CAM0_ACTIVATE : 0xBAD0;
				break;
			case CAM0_DEACTIVATE:
				retval = aptinaDeactivate(I2C_CAM0) ? CAM0_DEACTIVATE : 0xBAD0;
				break;
			case CAM1_ACTIVATE:
				retval = aptinaActivate(I2C_CAM1) ? CAM1_ACTIVATE : 0xBAD1;
				break;
			case CAM1_DEACTIVATE:
				retval = aptinaDeactivate(I2C_CAM1) ? CAM1_DEACTIVATE : 0xBAD1;
				break;
			case CAM_SET_MODE_PREVIEW:
				aptinaSetMode(CAPTURE_MODE_PREVIEW);
				retval = CAPTURE_MODE_PREVIEW;
				break;
			case CAM_SET_MODE_CAPTURE:
				aptinaSetMode(CAPTURE_MODE_CAPTURE);
				retval = CAPTURE_MODE_CAPTURE;
				break;
			default:
				break;
		}

		pxiReply(PxiChannel_Camera, retval);
	}

	return 0;
}

void i2cCameraServerStart(void) {
	threadPrepare(&s_i2cPxiThread, i2cPxiThreadMain, NULL,
		&s_i2cPxiThreadStack[sizeof(s_i2cPxiThreadStack)], MAIN_THREAD_PRIO - 1);
	threadStart(&s_i2cPxiThread);
}
