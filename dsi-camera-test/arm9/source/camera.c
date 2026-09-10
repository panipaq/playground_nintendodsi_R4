// ARM9-seitige Kamerasteuerung: initialisiert die Kamera-Hardware, spricht
// über PXI (Kanal PxiChannel_Camera) mit dem I2C-Treiber auf dem ARM7, und
// startet/stoppt den DMA-Transfer der Bilddaten in den Grafikspeicher.
//
// Register-Layout laut https://problemkaputt.de/gbatek-dsi-cameras.htm

#include "camera.h"

#include <calico/nds/pxi.h>
#include <calico/nds/scfg.h>
#include <nds.h>

static Camera activeCamera    = CAM_NONE;
static CaptureMode activeMode = CAPTURE_MODE_PREVIEW;

static u32 lastInitStatus = 0;

u32 cameraLastInitStatus(void) { return lastInitStatus; }

bool cameraInit(void) {
	REG_SCFG_CLK |= SCFG_CLK_CAM_IFACE; // Kamera-Schnittstellentakt an
	REG_CAM_MCNT = 0;
	swiDelay(0x1E);
	REG_SCFG_CLK |= SCFG_CLK_CAM_EXT; // externer Kameratakt an
	swiDelay(0x1E);
	REG_CAM_MCNT = BIT(1) | BIT(5); // Modul-Reset + I2C-Zugriff freischalten
	swiDelay(0x2008);
	REG_SCFG_CLK &= ~SCFG_CLK_CAM_EXT;

	REG_CAM_CNT &= ~BIT(15);          // Parameteraenderung erlauben
	REG_CAM_CNT |= BIT(5);            // Daten-FIFO leeren
	REG_CAM_CNT = (REG_CAM_CNT & ~0x0300) | 0x0200;
	REG_CAM_CNT |= BIT(10);
	REG_CAM_CNT |= BIT(11); // IRQ an

	REG_SCFG_CLK |= SCFG_CLK_CAM_EXT;
	swiDelay(0x14);

	// Sensor-Initialisierungssequenz laeuft auf dem ARM7 (I2C-Bus).
	u32 val = pxiSendAndReceive(PxiChannel_Camera, CAM_INIT);
	lastInitStatus = val;

	REG_SCFG_CLK &= ~SCFG_CLK_CAM_EXT;
	REG_SCFG_CLK |= SCFG_CLK_CAM_EXT;
	swiDelay(0x14);

	return val == 0x2280;
}

bool cameraActivate(Camera cam) {
	if (activeCamera != CAM_NONE) cameraDeactivate(activeCamera);

	u32 command = (cam == CAM_INNER) ? CAM0_ACTIVATE : CAM1_ACTIVATE;
	if (pxiSendAndReceive(PxiChannel_Camera, command) == command) {
		activeCamera = cam;
		return true;
	}
	return false;
}

bool cameraDeactivate(Camera cam) {
	u32 command = (cam == CAM_INNER) ? CAM0_DEACTIVATE : CAM1_DEACTIVATE;
	if (pxiSendAndReceive(PxiChannel_Camera, command) == command) {
		activeCamera = CAM_NONE;
		return true;
	}
	return false;
}

void cameraTransferStart(u16* dst, CaptureMode mode) {
	bool preview = mode == CAPTURE_MODE_PREVIEW;

	if (mode != activeMode) {
		pxiSendAndReceive(PxiChannel_Camera, preview ? CAM_SET_MODE_PREVIEW : CAM_SET_MODE_CAPTURE);
		activeMode = mode;
	}

	if (REG_CAM_CNT & BIT(15)) cameraTransferStop();

	if (preview) {
		REG_CAM_CNT |= 0x2003; // YUV->RGB555 an, 4 Scanlines/DMA-Block
	} else {
		REG_CAM_CNT &= ~0x2003; // rohes YUV422, 1 Scanline/DMA-Block
	}
	REG_CAM_CNT |= BIT(5);  // Daten-FIFO leeren
	REG_CAM_CNT |= BIT(15); // Transfer starten

	REG_NDMA1SAD  = (u32)&REG_CAM_DAT;
	REG_NDMA1DAD  = (u32)dst;
	REG_NDMA1TCNT = (preview ? 256 * 192 : 640 * 480) / 2; // Gesamtlaenge in Words
	REG_NDMA1WCNT = preview ? 512 : 320;                   // Blockgroesse in Words
	REG_NDMA1BCNT = 2;
	REG_NDMA1CNT  = 0x8B044000; // NDMA fuer Kamera-Timing starten
}

void cameraTransferStop(void) { REG_CAM_CNT &= ~BIT(15); }

bool cameraTransferActive(void) { return REG_NDMA1CNT & BIT(31); }
