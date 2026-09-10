#pragma once

#include <nds.h>
#include <nds/arm7/i2c.h>

#ifdef __cplusplus
extern "C" {
#endif

// Die Aptina-Kamerasensoren benutzen 16bit-Register-Adressen -- der normale
// i2cWriteRegister()/i2cReadRegister() aus libnds kann nur 8bit-Adressen
// (z.B. fuer den Power-Management-Chip). Darum hier eigene, rohe
// I2C-Bus-Steuerung direkt ueber REG_I2CDATA/REG_I2CCNT.
enum i2cFlags { I2C_NONE = 0x00, I2C_STOP = 0x01, I2C_START = 0x02, I2C_ACK = 0x10, I2C_READ = 0x20 };

u8 aptWriteRegister(u8 device, u16 reg, u16 data);
u16 aptReadRegister(u8 device, u16 reg);

#ifdef __cplusplus
}
#endif
