// Rohe I2C-Bus-Ansteuerung fuer die Aptina-Kamerasensoren (16bit-Register-
// Adressen). Angelehnt an libnds' eigene i2c.twl.c, aber fuer u16-Register
// erweitert. Siehe https://github.com/devkitPro/calico/blob/master/source/nds/arm7/i2c.twl.c

#include "aptina_i2c.h"

MK_INLINE bool i2cGetResult() {
	i2cWaitBusy();
	return (REG_I2CCNT >> 4) & 0x01;
}

static u8 aptGetData(u8 flags) {
	REG_I2CCNT = 0xC0 | flags;
	i2cWaitBusy();
	return REG_I2CDATA;
}

static u8 aptSetData(u8 data, u8 flags) {
	REG_I2CDATA = data;
	REG_I2CCNT  = 0xC0 | flags;
	return i2cGetResult();
}

static u8 aptSelectDevice(u8 device, u8 flags) {
	i2cWaitBusy();
	REG_I2CDATA = device;
	REG_I2CCNT  = 0xC0 | flags;
	return i2cGetResult();
}

static u8 aptSelectRegister(u8 reg, u8 flags) {
	REG_I2CDATA = reg;
	REG_I2CCNT  = 0xC0 | flags;
	return i2cGetResult();
}

u8 aptWriteRegister(u8 device, u16 reg, u16 data) {
	for (int i = 0; i < 8; i++) {
		if (aptSelectDevice(device, I2C_START) && aptSelectRegister(reg >> 8, I2C_NONE) &&
			aptSelectRegister(reg & 0xFF, I2C_NONE)) {
			if (aptSetData(data >> 8, I2C_NONE) && aptSetData(data & 0xFF, I2C_STOP)) return 1;
		}
		REG_I2CCNT = 0xC5;
	}
	return 0;
}

u16 aptReadRegister(u8 device, u16 reg) {
	for (int i = 0; i < 8; i++) {
		if (aptSelectDevice(device, I2C_START) && aptSelectRegister(reg >> 8, I2C_NONE) &&
			aptSelectRegister(reg & 0xFF, I2C_STOP)) {
			if (aptSelectDevice(device | 1, I2C_START)) {
				return (aptGetData(I2C_READ | I2C_ACK) << 8) | aptGetData(I2C_STOP | I2C_READ);
			}
		}
		REG_I2CCNT = 0xC5;
	}
	return 0xFFFF;
}
