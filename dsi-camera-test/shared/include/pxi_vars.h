#pragma once

// Gemeinsame Kommandos/Konstanten fuer die PXI-Kommunikation zwischen ARM7
// (spricht die Kamera-I2C-Bus an) und ARM9 (steuert Capture/DMA). Der
// PXI-Kanal PxiChannel_Camera (8) ist in calico offiziell fuer genau diesen
// Zweck reserviert, aber (Stand jetzt) nicht mit einem fertigen Treiber
// befuellt -- wir implementieren ihn hier selbst.

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	CAM_INIT,
	CAM0_ACTIVATE,
	CAM0_DEACTIVATE,
	CAM1_ACTIVATE,
	CAM1_DEACTIVATE,
	CAM_SET_MODE_PREVIEW,
	CAM_SET_MODE_CAPTURE,
} PxiCommand;

typedef enum {
	CAPTURE_MODE_PREVIEW = 1, // 256x192, direkt als RGB555 (Hardware-Konvertierung)
	CAPTURE_MODE_CAPTURE = 2, // 640x480, roh als YUV422
} CaptureMode;

#ifdef __cplusplus
}
#endif
