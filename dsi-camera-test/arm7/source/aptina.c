// Initialisierungs-/Steuersequenz fuer die Aptina MT9V113 Kamerasensoren der
// DSi. Die exakten Registerwerte sind reverse-engineered und stammen von
// GBATEK (https://problemkaputt.de/gbatek.htm#dsiaptinacamerainitialization)
// -- das sind keine offiziell dokumentierten Werte, sondern von der
// Homebrew-Community durch Beobachtung des originalen Kamera-Firmwarecodes
// ermittelt. Selbst herleiten waere hier nicht seriös moeglich.

#include "aptina.h"
#include "aptina_i2c.h"

static u8 currentDevice = I2C_CAM0;

// Statt wie im Referenzcode endlos zu warten, brechen wir nach ~10s ab und
// merken uns, dass ein Timeout aufgetreten ist -- so haengt sich der Thread
// nicht komplett auf, wenn der Sensor nicht wie erwartet antwortet (z.B.
// weil die Hardware in DS- statt echtem DSi-Modus laeuft), und wir bekommen
// stattdessen eine auswertbare Fehlermeldung auf dem ARM9 zurueck.
#define APT_WAIT_TIMEOUT_FRAMES 600
static bool aptinaTimedOut = false;

static void aptWaitClr(u8 device, u16 reg, u16 mask) {
	for (int i = 0; i < APT_WAIT_TIMEOUT_FRAMES; i++) {
		if (!(aptReadRegister(device, reg) & mask)) return;
		swiWaitForVBlank();
	}
	aptinaTimedOut = true;
}

static void aptWaitSet(u8 device, u16 reg, u16 mask) {
	for (int i = 0; i < APT_WAIT_TIMEOUT_FRAMES; i++) {
		if ((aptReadRegister(device, reg) & mask) == mask) return;
		swiWaitForVBlank();
	}
	aptinaTimedOut = true;
}

static void aptClr(u8 device, u16 reg, u16 mask) {
	u16 temp = aptReadRegister(device, reg);
	aptWriteRegister(device, reg, temp & ~mask);
}

static void aptSet(u8 device, u16 reg, u16 mask) {
	u16 temp = aptReadRegister(device, reg);
	aptWriteRegister(device, reg, temp | mask);
}

static u16 aptReadMcu(u8 device, u16 reg) {
	aptWriteRegister(device, 0x098C, reg);
	return aptReadRegister(device, 0x0990);
}

static void aptWriteMcu(u8 device, u16 reg, u16 data) {
	aptWriteRegister(device, 0x098C, reg);
	aptWriteRegister(device, 0x0990, data);
}

static void aptWaitMcuClr(u8 device, u16 reg, u16 mask) {
	for (int i = 0; i < APT_WAIT_TIMEOUT_FRAMES; i++) {
		if (!(aptReadMcu(device, reg) & mask)) return;
		swiWaitForVBlank();
	}
	aptinaTimedOut = true;
}

static void aptSetMcu(u8 device, u16 reg, u16 mask) {
	u16 temp = aptReadMcu(device, reg);
	aptWriteMcu(device, reg, temp | mask);
}

// Gibt 0 bei Erfolg zurueck, sonst die Nummer des Schritts, bei dem zuerst
// ein Timeout auftrat (siehe APT_WAIT_TIMEOUT_FRAMES) -- so laesst sich am
// zurueckgegebenen Wert erkennen, wie weit die Initialisierung gekommen ist.
int aptinaInit(u8 device) {
	aptinaTimedOut = false;

	aptWriteRegister(device, 0x001A, 0x0003); // RESET_AND_MISC_CONTROL (reset)
	aptWriteRegister(device, 0x001A, 0x0000); // RESET_AND_MISC_CONTROL (release reset)
	aptWriteRegister(device, 0x0018, 0x4028); // STANDBY_CONTROL (wakeup)
	aptWriteRegister(device, 0x001E, 0x0201); // PAD_SLEW
	aptWriteRegister(device, 0x0016, 0x42DF); // CLOCKS_CONTROL
	aptWaitClr(device, 0x0018, 0x4000);       // wait for WakeupDone
	if (aptinaTimedOut) return 1;
	aptWaitSet(device, 0x301A, 0x0004);       // wait for WakeupDone
	if (aptinaTimedOut) return 2;
	aptWriteMcu(device, 0x02F0, 0x0000);
	aptWriteMcu(device, 0x02F2, 0x0210);
	aptWriteMcu(device, 0x02F4, 0x001A);
	aptWriteMcu(device, 0x2145, 0x02F4);
	aptWriteMcu(device, 0xA134, 0x0001);
	aptSetMcu(device, 0xA115, 0x0002); // SEQ_CAP_MODE (bit1=video)
	aptWriteMcu(device, 0x2755, 0x0002); // MODE_OUTPUT_FORMAT_A (YUV)
	aptWriteMcu(device, 0x2757, 0x0002); // MODE_OUTPUT_FORMAT_B (YUV)
	aptWriteRegister(device, 0x0014, 0x2145); // PLL_CONTROL
	aptWriteRegister(device, 0x0010, 0x0111); // PLL_DIVIDERS
	aptWriteRegister(device, 0x0012, 0x0000); // PLL_P_DIVIDERS
	aptWriteRegister(device, 0x0014, 0x244B); // PLL_CONTROL
	aptWriteRegister(device, 0x0014, 0x304B); // PLL_CONTROL
	aptWaitSet(device, 0x0014, 0x8000);       // wait for PLL Lock
	if (aptinaTimedOut) return 3;
	aptClr(device, 0x0014, 0x0001);           // disable PLL Bypass
	aptWriteMcu(device, 0x2703, 0x0100); // MODE_OUTPUT_WIDTH_A  = 256
	aptWriteMcu(device, 0x2705, 0x00C0); // MODE_OUTPUT_HEIGHT_A = 192
	aptWriteMcu(device, 0x2707, 0x0280); // MODE_OUTPUT_WIDTH_B  = 640
	aptWriteMcu(device, 0x2709, 0x01E0); // MODE_OUTPUT_HEIGHT_B = 480
	aptWriteMcu(device, 0x2715, 0x0001);
	aptWriteMcu(device, 0x2719, 0x001A);
	aptWriteMcu(device, 0x271B, 0x006B);
	aptWriteMcu(device, 0x271D, 0x006B);
	aptWriteMcu(device, 0x271F, 0x02C0);
	aptWriteMcu(device, 0x2721, 0x034B);
	aptWriteMcu(device, 0xA20B, 0x0000); // AE_MIN_INDEX
	aptWriteMcu(device, 0xA20C, 0x0006); // AE_MAX_INDEX
	aptWriteMcu(device, 0x272B, 0x0001);
	aptWriteMcu(device, 0x272F, 0x001A);
	aptWriteMcu(device, 0x2731, 0x006B);
	aptWriteMcu(device, 0x2733, 0x006B);
	aptWriteMcu(device, 0x2735, 0x02C0);
	aptWriteMcu(device, 0x2737, 0x034B);
	aptSet(device, 0x3210, 0x0008); // COLOR_PIPELINE_CONTROL
	aptWriteMcu(device, 0xA208, 0x0000);
	aptWriteMcu(device, 0xA24C, 0x0020); // AE_TARGETBUFFERSPEED
	aptWriteMcu(device, 0xA24F, 0x0070); // AE_BASETARGET
	if (device == I2C_CAM0) {
		aptWriteMcu(device, 0x2717, 0x0024); // MODE_SENSOR_READ_MODE_A (x-flip)
		aptWriteMcu(device, 0x272D, 0x0024); // MODE_SENSOR_READ_MODE_B (x-flip)
	} else {
		aptWriteMcu(device, 0x2717, 0x0025);
		aptWriteMcu(device, 0x272D, 0x0025);
	}
	if (device == I2C_CAM0) {
		aptWriteMcu(device, 0xA202, 0x0022); // AE_WINDOW_POS
		aptWriteMcu(device, 0xA203, 0x00BB); // AE_WINDOW_SIZE
	} else {
		aptWriteMcu(device, 0xA202, 0x0000);
		aptWriteMcu(device, 0xA203, 0x00FF);
	}
	aptSet(device, 0x0016, 0x0020);
	aptWriteMcu(device, 0xA115, 0x0072); // SEQ_CAP_MODE
	aptWriteMcu(device, 0xA11F, 0x0001); // SEQ_PREVIEW_1_AWB
	if (device == I2C_CAM0) {
		aptWriteRegister(device, 0x326C, 0x0900); // APERTURE_PARAMETERS
		aptWriteMcu(device, 0xAB22, 0x0001);       // HG_LL_APCORR1
	} else {
		aptWriteRegister(device, 0x326C, 0x1000);
		aptWriteMcu(device, 0xAB22, 0x0002);
	}
	aptWriteMcu(device, 0xA103, 0x0006);   // SEQ_CMD (RefreshMode)
	aptWaitMcuClr(device, 0xA103, 0x000F);
	if (aptinaTimedOut) return 4;
	aptWriteMcu(device, 0xA103, 0x0005);   // SEQ_CMD (Refresh)
	aptWaitMcuClr(device, 0xA103, 0x000F);
	if (aptinaTimedOut) return 5;

	return 0;
}

bool aptinaActivate(u8 device) {
	aptinaTimedOut = false;

	aptClr(device, 0x0018, 0x0001);     // STANDBY_CONTROL (wakeup)
	aptWaitClr(device, 0x0018, 0x4000); // wait for WakeupDone
	aptWaitSet(device, 0x301A, 0x0004); // wait for WakeupDone
	aptWriteRegister(device, 0x3012, 0x0010); // COARSE_INTEGRATION_TIME (Helligkeit)
	aptSet(device, 0x001A, 0x0200);           // RESET_AND_MISC_CONTROL (Parallel-Ausgang an)
	if (device == I2C_CAM1) i2cWriteRegister(0x4A, 0x31, 0x01); // BPTWL: Kamera-LED an

	currentDevice = device;
	return !aptinaTimedOut;
}

bool aptinaDeactivate(u8 device) {
	aptinaTimedOut = false;

	aptClr(device, 0x001A, 0x0200);     // Parallel-Ausgang aus
	aptSet(device, 0x0018, 0x0001);     // STANDBY_CONTROL (Standby)
	aptWaitSet(device, 0x0018, 0x4000); // wait for StandbyDone
	aptWaitClr(device, 0x301A, 0x0004); // wait for StandbyDone
	if (device == I2C_CAM1) i2cWriteRegister(0x4A, 0x31, 0x00); // BPTWL: Kamera-LED aus

	currentDevice = I2C_CAM0;
	return !aptinaTimedOut;
}

void aptinaSetMode(CaptureMode mode) {
	aptWriteMcu(currentDevice, 0xA103, mode);
	aptWaitMcuClr(currentDevice, 0xA103, 0xFFFF);
}
