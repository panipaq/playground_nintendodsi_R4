#pragma once

#include <nds.h>

#ifdef __cplusplus
extern "C" {
#endif

// Startet den PXI-Server-Thread, der Kamera-Kommandos vom ARM9 entgegennimmt
// und ueber den I2C-Bus an die Kamerasensoren weiterreicht (siehe
// i2c_handler.c). Muss einmal beim ARM7-Start aufgerufen werden.
void i2cCameraServerStart(void);

#ifdef __cplusplus
}
#endif
